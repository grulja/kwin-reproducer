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

#ifndef KWIN_REPRODUCER_H
#define KWIN_REPRODUCER_H

#include <QObject>
#include <QMap>

#include <gbm.h>

namespace KWayland {
    namespace Client {
        class ConnectionThread;
        class EventQueue;
        class OutputDevice;
        class Registry;
        class RemoteAccessManager;
        class RemoteBuffer;
        class Output;
    }
}

class KWinReproducer: public QObject
{
    Q_OBJECT
public:
    explicit KWinReproducer(QObject *parent = nullptr);
    ~KWinReproducer();

private Q_SLOTS:
    void addOutput(quint32 name, quint32 version);
    void processBuffer(const KWayland::Client::RemoteBuffer *rbuf);
    void setupRegistry();

private:
    void initDrm();
    void initWayland();

    bool m_bindOutput;
    bool m_registryInitialized;

    QThread *m_thread;

    QList<KWayland::Client::Output*> m_bindOutputs;

    KWayland::Client::ConnectionThread *m_connection;
    KWayland::Client::EventQueue *m_queue;
    KWayland::Client::Registry *m_registry;
    KWayland::Client::RemoteAccessManager *m_remoteAccessManager;

    qint32 m_drmFd = 0;
    gbm_device *m_gbmDevice = nullptr;
};

#endif // KWIN_REPRODUCER_H


