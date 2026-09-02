# SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(Dtk${DTK_VERSION_MAJOR}Core REQUIRED)

# generate moc_predefs.h
set(CMAKE_AUTOMOC ON)
set(AUTOMOC_COMPILER_PREDEFINES ON)

include(src.cmake)

ADD_EXECUTABLE(dde-dconfig-daemon main.cpp ${HEADERS} ${SOURCES} ${DCONFIG_DBUS_XML})

set(COMMON_LIBS
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk${DTK_VERSION_MAJOR}::Core
)

target_link_libraries(dde-dconfig-daemon PUBLIC ${COMMON_LIBS})

target_include_directories(dde-dconfig-daemon PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

## dbus xml files
set(DCONFIG_XMLS_FILES
    services/org.desktopspec.ConfigManager.xml
    services/org.desktopspec.ConfigManager.Manager.xml
)
install (FILES ${DCONFIG_XMLS_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces)

if (QT_VERSION_MAJOR EQUAL 5)
    set(DBUS_CONFIG_FILE services/qt5/org.desktopspec.ConfigManager.conf)
    set(DBUS_SERVICE_FILE services/qt5/org.desktopspec.ConfigManager.service)
    set(SYSTEMD_SERVICE_FILE services/qt5/dde-dconfig-daemon.service)
    set(POLKIT_POLICY_FILE services/qt5/org.desktopspec.ConfigManager.policy)
else()
    set(DBUS_CONFIG_FILE services/org.desktopspec.ConfigManager.conf)
    set(DBUS_SERVICE_FILE services/org.desktopspec.ConfigManager.service)
    set(SYSTEMD_SERVICE_FILE services/dde-dconfig-daemon.service)
endif()

## dbus config file
install(FILES ${DBUS_CONFIG_FILE}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d
    RENAME org.desktopspec.ConfigManager.conf)

## services files
install(FILES ${DBUS_SERVICE_FILE}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services
    RENAME org.desktopspec.ConfigManager.service)
install(FILES ${SYSTEMD_SERVICE_FILE}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system
    RENAME dde-dconfig-daemon.service)

if (POLKIT_POLICY_FILE)
    install(FILES ${POLKIT_POLICY_FILE}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions
        RENAME org.desktopspec.ConfigManager.policy)
endif()

## bin
install(TARGETS dde-dconfig-daemon DESTINATION ${CMAKE_INSTALL_BINDIR})

# debug config file
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/deepin-debug-config/dde-dconfig-daemon.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin-debug-config/deepin-debug-config.d)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/deepin-log-config/dde-dconfig-daemon.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin-log-viewer/deepin-log.conf.d)
