#!/bin/bash

set -e
set -x

echo "creating containers..."

cid1=$(docker run -d phxsql)
cid2=$(docker run -d phxsql)
cid3=$(docker run -d phxsql)

ip1=$(docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $cid1)
ip2=$(docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $cid2)
ip3=$(docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $cid3)

sleep 30

docker exec -it $cid1 phxbinlogsvr_tools_phxrpc -f InitBinlogSvrMaster -h "$ip1,$ip2,$ip3" -p 17000

sleep 10

time=$(date +%Y%m%d%H%M%S)

docker exec $cid1 mysql -u root -h $ip1 -P 54321 -e "create database if not exists test_phxsql"
docker exec $cid1 mysql -u root -h $ip1 -P 54321 test_phxsql -e "create table if not exists test_phxsql(name varchar(80))"
docker exec $cid1 mysql -u root -h $ip1 -P 54321 test_phxsql -e "insert into test_phxsql values($time)"

for port in 54321 54322; do
    for ip in $ip1 $ip2 $ip3; do
        result=$(docker exec $cid1 mysql -N -u root -h $ip -P $port test_phxsql -e "select name from test_phxsql")
        test "$result" = "$time"
    done
    sleep 3
done

echo "clean up..."

docker stop $cid1 $cid2 $cid3
docker rm -v $cid1 $cid2 $cid3
