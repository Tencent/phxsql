#!/bin/bash

docker run -d -e "IP_LIST=127.0.0.1,172.16.8.186,172.16.8.187" \
	-e "PHXSQLPROXY_PORT=500" \
	-e "AGENT_PORT=501" \
	-e "PHXBINLOGSVR_PORT=502" \
	-e "PAXOS_PORT=506" \
	-e "MYSQL_PORT=3306" \
	 phxsql

