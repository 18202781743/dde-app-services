#!/bin/bash

# SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

# configure file is in the position. e.g: /usr/share/dsg/configs/dconfig-example/example.json
# global cache is in the position. e.g: /var/dsg/appdata/configs/dconfig-example/configs/example.json
# user cache is in the position. e.g: /home/userhome/.config/dsg/configs/dconfig-example/example.json


for time in {1..3}
do
    for i in {1..100}
    do
        echo $i
        cd /data/cj/repo/dde/dde-app-services/build-snipe_5_11_34/dconfig-center/example/
        ./dconfig-example &

    done

    ping 127.0.0.1 -c 2 > nul

    pkill dconfig-example

done
echo "kill all process"

jobs -p |xargs kill
