#!/bin/bash

set -e

create_containers() {
    cid=$(docker run -d phxsql/phxsql:latest)
    ip=$(docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $cid)
    echo "created container $cid $ip"
    read $1 $2 <<< "$cid $ip"
}

check_processes() {
    cid=$1
    proc=$(docker top $cid)
    for binary in phxbinlogsvr phxsqlproxy mysqld_safe; do
        if [[ ! $proc =~ $binary ]]; then
            echo "$binary in $cid hasn't started"
            return 1
        fi
    done
    return 0
}

echo "creating containers..."

create_containers cid1 ip1
create_containers cid2 ip2
create_containers cid3 ip3

echo "waiting processes start..."

while true; do
    sleep 10
    if check_processes $cid1 && \
       check_processes $cid2 && \
       check_processes $cid3; then
        break
    fi
done

docker exec -it $cid1 phxbinlogsvr_tools_phxrpc -f InitBinlogSvrMaster -h "$ip1,$ip2,$ip3" -p 17000

sleep 10

time=$(date +%Y%m%d%H%M%S)

echo "create database and insert $time..."

docker exec $cid1 mysql -u root -h $ip1 -P 54321 -e "create database if not exists test_phxsql"
docker exec $cid1 mysql -u root -h $ip1 -P 54321 test_phxsql -e "create table if not exists test_phxsql(name varchar(80))"
docker exec $cid1 mysql -u root -h $ip1 -P 54321 test_phxsql -e "insert into test_phxsql values($time)"

for port in 54321 54322; do
    for ip in $ip1 $ip2 $ip3; do
        result=$(docker exec $cid1 mysql -N -u root -h $ip -P $port test_phxsql -e "select name from test_phxsql")
        echo "select from $ip $port : $result"
        test "$result" = "$time"
    done
    sleep 3
done

echo "clean up..."

docker stop $cid1 $cid2 $cid3
docker rm -v $cid1 $cid2 $cid3
