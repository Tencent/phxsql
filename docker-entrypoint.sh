#!/bin/bash

set -x
cd /phxsql/tools
python2.7 install.py -i "$(hostname -I)" -p 54321 -g 6000 -y 11111 -P 17000 -a 8001 -f /data "$@"
exec tail -f /dev/null
