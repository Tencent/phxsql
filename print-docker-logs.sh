#!/bin/bash

for cid in $(docker ps -q --filter=ancestor=phxsql); do
    ip=$(docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $cid)
    echo "logs in $cid $ip"
    docker exec $cid cat /data/percona.workspace/log.err
    docker exec $cid cat /data/log/phxbinlogsvr.ERROR
    docker exec $cid cat /data/log/phxbinlogsvr.WARNING
    docker exec $cid cat /data/log/phxbinlogsvr.INFO
    docker exec $cid cat /data/log/phxsqlproxy.ERROR
    docker exec $cid cat /data/log/phxsqlproxy.WARNING
    docker exec $cid cat /data/log/phxsqlproxy.INFO
done
