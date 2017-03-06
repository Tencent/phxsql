#!/bin/bash

#保证在phxsql/tools目录下运行此命令

python ./install.py -i $IP_LIST \
        -p $PHXSQLPROXY_PORT \
        -g $AGENT_PORT \
        -y $MYSQL_PORT \
        -P $PHXBINLOGSVR_PORT \
        -a $PAXOS_PORT \
        -f $DATA_DIR > install.log

if [ $? != 0 ]; then
    cat install.log
    exit $?
fi
        
#只为docker能以deamon支行而不退出        
tail -f install.log
