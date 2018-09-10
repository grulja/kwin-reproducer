/*
 * Copyright © 2018 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "reproducer.h"

#include <QDebug>
#include <QEventLoop>
#include <QThread>
#include <QTimer>

// KWayland
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/event_queue.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/output.h>
#include <KWayland/Client/remote_access.h>

// system
#include <fcntl.h>
#include <unistd.h>

KWinReproducer::KWinReproducer(QObject *parent)
    : QObject(parent)
    , m_bindOutput(false)
    , m_registryInitialized(false)
    , m_connection(nullptr)
    , m_queue(nullptr)
    , m_registry(nullptr)
    , m_remoteAccessManager(nullptr)
{
    initDrm();
    initWayland();
}

KWinReproducer::~KWinReproducer()
{
    if (m_remoteAccessManager) {
        m_remoteAccessManager->destroy();
    }
}

void KWinReproducer::initDrm()
{
    m_drmFd = open("/dev/dri/renderD128", O_RDWR);
    m_gbmDevice = gbm_create_device(m_drmFd);

    if (!m_gbmDevice) {
        qFatal("Cannot create GBM device: %s", strerror(errno));
    }
}

void KWinReproducer::KWinReproducer::initWayland()
{
    m_thread = new QThread(this);
    m_connection = new KWayland::Client::ConnectionThread;

    connect(m_connection, &KWayland::Client::ConnectionThread::connected, this, &KWinReproducer::setupRegistry, Qt::QueuedConnection);
    connect(m_connection, &KWayland::Client::ConnectionThread::connectionDied, this, [this] {
        if (m_queue) {
            delete m_queue;
            m_queue = nullptr;
        }

        m_connection->deleteLater();
        m_connection = nullptr;

        if (m_thread) {
            m_thread->quit();
            if (!m_thread->wait(3000)) {
                m_thread->terminate();
                m_thread->wait();
            }
            delete m_thread;
            m_thread = nullptr;
        }
    });
    connect(m_connection, &KWayland::Client::ConnectionThread::failed, this, [this] {
        m_thread->quit();
        m_thread->wait();
    });

    m_thread->start();
    m_connection->moveToThread(m_thread);
    m_connection->initConnection();
}

void KWinReproducer::KWinReproducer::addOutput(quint32 name, quint32 version)
{
    if (!m_bindOutput) {
        m_bindOutput = true;
        KWayland::Client::Output *output = new KWayland::Client::Output(this);
        output->setup(m_registry->bindOutput(name, version));
    }
}

void KWinReproducer::KWinReproducer::processBuffer(const KWayland::Client::RemoteBuffer* rbuf)
{
    QScopedPointer<const KWayland::Client::RemoteBuffer> guard(rbuf);

    auto gbmHandle = rbuf->fd();
    auto width = rbuf->width();
    auto height = rbuf->height();
    auto stride = rbuf->stride();
    auto format = rbuf->format();

    qWarning() << QString("Incoming GBM fd %1, %2x%3, stride %4, fourcc 0x%5").arg(gbmHandle).arg(width).arg(height).arg(stride).arg(QString::number(format, 16));

    if (!gbm_device_is_format_supported(m_gbmDevice, format, GBM_BO_USE_SCANOUT)) {
        qCritical() << "GBM format is not supported by device!";
    }

    // import GBM buffer that was passed from KWin
    gbm_import_fd_data importInfo = {gbmHandle, width, height, stride, format};
    gbm_bo *imported = gbm_bo_import(m_gbmDevice, GBM_BO_IMPORT_FD, &importInfo, GBM_BO_USE_SCANOUT);
    if (!imported) {
        qCritical() << "Cannot import passed GBM fd:" << strerror(errno);
    }

    gbm_bo_destroy(imported);

    close(gbmHandle);
}

void KWinReproducer::KWinReproducer::setupRegistry()
{
    m_queue = new KWayland::Client::EventQueue(this);
    m_queue->setup(m_connection);

    m_registry = new KWayland::Client::Registry(this);

    connect(m_registry, &KWayland::Client::Registry::outputAnnounced, this, &KWinReproducer::addOutput);

    connect(m_registry, &KWayland::Client::Registry::interfacesAnnounced, this, [this] {
        if (m_registry->hasInterface(KWayland::Client::Registry::Interface::RemoteAccessManager)) {
            KWayland::Client::Registry::AnnouncedInterface interface = m_registry->interface(KWayland::Client::Registry::Interface::RemoteAccessManager);
            if (!interface.name && !interface.version) {
                qWarning() << "Cannot start stream because remote access interface is not initialized yet";
                return;
            }
            m_remoteAccessManager = m_registry->createRemoteAccessManager(interface.name, interface.version);
            connect(m_remoteAccessManager, &KWayland::Client::RemoteAccessManager::bufferReady, this, [this] (const void *output, const KWayland::Client::RemoteBuffer * rbuf) {
                Q_UNUSED(output);
                connect(rbuf, &KWayland::Client::RemoteBuffer::parametersObtained, this, [this, rbuf] {
                    processBuffer(rbuf);
                });
            });
        };
    });

    m_registry->create(m_connection);
    m_registry->setEventQueue(m_queue);
    m_registry->setup();
}
