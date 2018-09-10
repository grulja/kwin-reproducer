project(kwin-reproducer)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.4.0")

################# set KDE specific information #################

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMPackageConfigHelpers)
include(ECMOptionalAddSubdirectory)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    DBus
    Widgets
)

find_package(KF5 REQUIRED
    Wayland
)

find_package(GBM)
set_package_properties(GBM PROPERTIES
    TYPE REQUIRED
)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0 -DQT_NO_KEYWORDS)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY)

set(kwin_reproducer_SRCS
    main.cpp
    reproducer.cpp
)

add_executable(kwin-reproducer ${kwin_reproducer_SRCS})

target_link_libraries(kwin-reproducer
    Qt5::Core
    Qt5::DBus
    Qt5::Concurrent
    Qt5::Widgets
    KF5::WaylandClient
    GBM::GBM
)

install(TARGETS kwin-reproducer DESTINATION ${KDE_INSTALL_LIBEXECDIR})
