
if [ $# -lt 1 ]; then
echo "port ip1 ip2 ip3"
exit
fi

port=$1
ip=$2

function queryl_mysql(){
	echo $2
		echo $1
		echo "$1" | sh 

		if [ $? -ne 0 ]; then
			echo "$2 fail"
				exit 1
				fi
				echo "$2 done"

}

time=$(date +%Y%m%d%H%M%S)

queryl_mysql "mysql -uroot -h$ip -P$port -e \"create database if not exists test_phxsql;\"" "create database to phxsql"

queryl_mysql "mysql -uroot -h$ip -P$port -e \"use test_phxsql; create table if not exists test_phxsql(name varchar(80));\"" "create table to phxsql"

queryl_mysql "mysql -uroot -h$ip -P$port -e \"use test_phxsql; insert into test_phxsql values($time);\"" "insert data to phxsql"

queryl_mysql "mysql -uroot -h$ip -P$port -e \"use test_phxsql; select name from test_phxsql;\"" "select data from phxsql"

for i in $@
do 
if [ "$i" == "$port" ]; then
continue
fi
queryl_mysql "mysql -uroot -h$i -P$port -e \"use test_phxsql; select name from test_phxsql;\"" "select data from phxsql from read/write port"

((read_port=$port+1))
queryl_mysql "mysql -uroot -h$i -P$read_port -e \"use test_phxsql; select name from test_phxsql;\"" "select data from phxsql from readonly port"
done
