
set -e  # exit immediately on error
set -x  # display all commands

cd third_party;

if [ ! -f protobuf/bin/protoc ]; then
	if [ ! -f protobuf-cpp-3.0.0.tar.gz ]; then
		wget https://github.com/google/protobuf/releases/download/v3.0.0/protobuf-cpp-3.0.0.tar.gz
	fi	

	tar zxvf protobuf-cpp-3.0.0.tar.gz
	cd protobuf-3.0.0

	./configure CXXFLAGS=-fPIC --prefix=`pwd`/../protobuf 
	make -j2
	make install

	cd ../
fi

if [ ! -f glog/lib/libglog.a ]; then
	if [ ! -f v0.3.3.tar.gz ]; then
		wget https://github.com/google/glog/archive/v0.3.3.tar.gz
	fi	

	tar zxvf v0.3.3.tar.gz
	cd glog-0.3.3 

	./configure CXXFLAGS=-fPIC --prefix=`pwd`/../glog
	make -j2
	make install

	cd ../
fi

if [ ! -f leveldb/lib/libleveldb.a ]; then
	if [ ! -f v1.19.tar.gz ]; then
		wget https://github.com/google/leveldb/archive/v1.19.tar.gz
	fi

	rm -rf leveldb

	tar zxvf v1.19.tar.gz
	ln -s leveldb-1.19 leveldb
	cd leveldb

	make

	mkdir -p lib
	cp out-static/libleveldb.a lib/

	cd ../
fi

if [ ! -f colib/lib/libcolib.a ]; then
	if [ ! -f colib-master.zip ]; then
		wget https://github.com/tencent-wechat/libco/archive/master.zip -O colib-master.zip
	fi

	rm -rf colib

	unzip colib-master.zip
	ln -s libco-master colib

	cd colib
	make

	cd ../
fi

if [ ! -f phxpaxos/lib/libphxpaxos.a ]; then
	if [ ! -f v1.0.2.tar.gz ]; then
		wget https://github.com/tencent-wechat/phxpaxos/archive/v1.0.2.tar.gz
	fi

	rm -rf phxpaxos

	tar zxvf v1.0.2.tar.gz
	ln -s phxpaxos-1.0.2 phxpaxos

	cd phxpaxos/third_party
	rm -rf glog gmock leveldb protobuf
	ln -s ../../glog-0.3.3 glog
	ln -s ../../protobuf-3.0.0/gmock gmock
	ln -s ../../leveldb leveldb
	ln -s ../../protobuf protobuf
	cd ../

	./autoinstall.sh

	make
	make install

	cd plugin
	make
	make install
	
	cd ../../
fi

if [ ! -f phxrpc/lib/libphxrpc.a ]; then
	if [ ! -f phxrpc-master.zip ]; then
		wget https://github.com/tencent-wechat/phxrpc/archive/master.zip -O phxrpc-master.zip
	fi

	rm -rf phxrpc

	unzip phxrpc-master.zip
	ln -s phxrpc-master phxrpc

	cd phxrpc
	rm third_party -rf 
	ln -s ../../third_party third_party

	make

	cd ../
fi

cd ..

if [ ! -f percona/sql/binlog.cc ]; then
	wget https://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.31-77.0/source/tarball/percona-server-5.6.31-77.0.tar.gz

	rm percona -rf

	tar -xvf percona-server-5.6.31-77.0.tar.gz 
	mv percona-server-5.6.31-77.0 percona 
fi

./autoinstall.sh

make

make install

