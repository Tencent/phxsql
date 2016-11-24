#!/bin/bash

base_dir=`pwd`
make_file_tools=$base_dir/tools/create_makefile.py
check_env_tools=$base_dir/tools/check_install.py
src_dir=$base_dir/src_list

function check_env(){
	echo "[check environment status]..."
	python $check_env_tools $base_dir
	if [ $? -gt 0 ];
	then
		exit 1
	fi
	echo "[check environment status ok]"
}

function create_makefile(){
	python $make_file_tools $base_dir $1 
}

function scandir(){
	if [ $1 ];then
		echo "[creating makefile] $1"
		create_makefile $1
		for file in `ls $1`
		do
			if ([ -d $1"/"$file ] && [ "$file" != "glog" ])
				then
					scandir $1"/"$file
			fi
		done
	fi
}

function install_mysql(){
	cd percona
	rm plugin/phxsync_phxrpc -rf
	cmake . -DCMAKE_INSTALL_PREFIX=$base_dir/percona -DCMAKE_BUILD_TYPE=Release -DWITH_EMBEDDED_SERVER=OFF
	make perconaserverclient
	cd -
	cp phx_percona/percona/plugin/phxsync_phxrpc percona/plugin -r
	cp phx_percona/percona/sql/* percona/sql/ -r
	cp phx_percona/percona/plugin/semisync/semisync_master_plugin.cc  percona/plugin/semisync/semisync_master_plugin.cc
	cd percona
	cmake . -DCMAKE_INSTALL_PREFIX=$base_dir/percona -DCMAKE_BUILD_TYPE=Release -DWITH_EMBEDDED_SERVER=OFF -DWITH_PHXSYNC_MASTER_PHXRPC=1
	cd -
	echo "[install mysql] done"
}

function process(){
	create_makefile $base_dir
	res=`cat $src_dir`
	echo $res
	for file in $res
	do
		if ([ -d $base_dir"/"$file ] && [ "$file" != "glog" ])
			then
				scandir $base_dir"/"$file
		fi
	done

	install_mysql
}

function showusage(){
	echo "Configuration:"
	echo "  -h, --help              display this help and exit"
	echo "Installation directories:"
	echo "  --prefix=PREFIX         install architecture-independent files in PREFIX"
	echo "                          [.]"
	exit
}

prefix=

ARGS=`getopt -o h -l prefix:,help -- "$@"`  
eval set -- "${ARGS}" 

while true  
do  
	case "$1" in 
		--prefix)  
			if [ $2 ]; then
				sed -i -r "s#PREFIX=.*#PREFIX=$2#" makefile.mk
			fi
			shift  
			;;  
		-h|--help)
			showusage
			break
			;;
		--)  
			shift  
			break 
			;;  
	esac  
shift  
done 

check_env

if [ $? -eq 0 ]; then 
process 
fi
