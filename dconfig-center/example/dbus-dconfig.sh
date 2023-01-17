#!/bin/bash

# SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

date +'%F %T.%3N'
dbus-send --system --type=method_call --print-reply=literal --dest=org.desktopspec.ConfigManager / org.desktopspec.ConfigManager.setDelayReleaseTime int32:40

echo delayReleaseTime: $(dbus-send --system --type=method_call --print-reply=literal --dest=org.desktopspec.ConfigManager / org.desktopspec.ConfigManager.delayReleaseTime)

#acquireManager
DCONFIG_RESOURCE_PATH=$(dbus-send --system --type=method_call --print-reply=literal --dest=org.desktopspec.ConfigManager / org.desktopspec.ConfigManager.acquireManager string:'dconfig-example' string:'example' string:'')

#object path
echo path: $DCONFIG_RESOURCE_PATH

#monitor valueChanged
dbus-monitor --system "type='signal', interface='org.desktopspec.ConfigManager.Manager',member=valueChanged" &

#description
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.description string:'canExit' string:''
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

#name
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.name string:'canExit' string:''
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.name string:'canExit' string:'zh_CN'
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

#visibility
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.visibility string:'canExit'
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

#permissions
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.permissions string:'canExit'
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

#flags
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.flags string:'canExit'
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release


#value
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.value string:'canExit' 
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release


#setValue
PROPERTY_VALUE=$(dbus-send --system --type=method_call --print-reply=literal --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.value string:'canExit')  
if [[ "$PROPERTY_VALUE" = *true ]];then 
  dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.setValue string:'canExit' variant:boolean:false
else  
  dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.setValue string:'canExit' variant:boolean:true
fi
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

#reset
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.reset string:'canExit'
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

#value
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.value string:'canExit' 
dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

#release
# dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.release

##update
#dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager / org.desktopspec.ConfigManager.update string:'/usr/share/dsg/configs/dconfig-example/example.json'

##sync
#dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager / org.desktopspec.ConfigManager.sync string:'/usr/share/dsg/configs/dconfig-example/example.json'


# killall dbus-monitor
# kill $DBUS_MONITOR_PID

date +'%F %T.%3N'

jobs -p |xargs kill 
