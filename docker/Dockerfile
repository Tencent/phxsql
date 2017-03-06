FROM gcc:4.8

RUN echo "deb http://mirrors.163.com/ubuntu/ precise main restricted universe multiverse" > /etc/apt/sources.list
RUN echo "deb http://mirrors.163.com/ubuntu/ precise-security main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb http://mirrors.163.com/ubuntu/ precise-updates main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb http://mirrors.163.com/ubuntu/ precise-proposed main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb http://mirrors.163.com/ubuntu/ precise-backports main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://mirrors.163.com/ubuntu/ precise main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://mirrors.163.com/ubuntu/ precise-security main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://mirrors.163.com/ubuntu/ precise-updates main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://mirrors.163.com/ubuntu/ precise-proposed main restricted universe multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://mirrors.163.com/ubuntu/ precise-backports main restricted universe multiverse" >> /etc/apt/sources.list
RUN apt-get update -y

RUN apt-get install python -y 
RUN apt-get install libncurses5-dev  libreadline-dev -y 
RUN apt-get install libstdc++6 unzip cmake -y --force-yes

WORKDIR /app
RUN mkdir /data

ENV DATA_DIR /data
ENV THIRD_PATH /root/third_party

RUN mkdir -p $THIRD_PATH #/{glog,leveldb,protobuf,libco,phxpaxos,phxrpc}

#wget -c https://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.31-77.0/source/tarball/percona-server-5.6.31-77.0.tar.gz
ADD percona-server-5.6.31-77.0.tar.gz $THIRD_PATH/

# wget -c https://github.com/google/glog/archive/v0.3.3.tar.gz -o glog v0.3.3.tar.gz
ADD glog-v0.3.3.tar.gz $THIRD_PATH/glog
WORKDIR $THIRD_PATH/glog/glog-0.3.3
RUN ./configure CXXFLAGS=-fPIC --prefix=`pwd` ; make ; make install 

#wget -c https://github.com/google/leveldb/archive/v1.18.tar.gz -o leveldb-v1.18.tar.gz
ADD leveldb-v1.18.tar.gz $THIRD_PATH/leveldb
WORKDIR $THIRD_PATH/leveldb/leveldb-1.18
RUN make -j 10
RUN mkdir lib; mv libl* lib/

# wget -c https://github.com/google/protobuf/archive/v3.0.0.tar.gz -o protobuf-v3.0.0.tar.gz
ADD protobuf-v3.0.0.tar.gz $THIRD_PATH/protobuf
ADD gmock-1.7.0.zip $THIRD_PATH/protobuf/protobuf-3.0.0/gmock-1.7.0.zip
WORKDIR $THIRD_PATH/protobuf/protobuf-3.0.0
RUN unzip gmock-1.7.0.zip; mv gmock-1.7.0 gmock

#RUN mv code-gmock/gmock-1.7.0 gmock # rm -rf code-gmock gmock-1.7.0.zip
RUN ./autogen.sh
RUN ./configure CXXFLAGS=-fPIC --prefix=`pwd` ; make ; make install 

#git clone https://github.com/tencent-wechat/libco.git
ADD libco $THIRD_PATH/libco
WORKDIR $THIRD_PATH/libco
RUN make -j 2

#wget -c      https://github.com/tencent-wechat/phxpaxos/archive/v1.0.1.tar.gz -o phxpaxos-v1.0.1.tar.gz
ADD phxpaxos-v1.0.1.tar.gz $THIRD_PATH/phxpaxos
WORKDIR $THIRD_PATH/phxpaxos/phxpaxos-1.0.1/third_party
RUN rm * -fr
RUN ln -s $THIRD_PATH/glog/glog-0.3.3/ glog
RUN ln -s $THIRD_PATH/leveldb/leveldb-1.18/ leveldb
RUN ln -s $THIRD_PATH/protobuf/protobuf-3.0.0/ protobuf
RUN ln -s $THIRD_PATH/protobuf/protobuf-3.0.0/gmock gmock  

WORKDIR $THIRD_PATH/phxpaxos/phxpaxos-1.0.1
RUN ./autoinstall.sh
RUN make -j 4; make install; cd plugin; make; make install


#wget -c      https://github.com/tencent-wechat/phxrpc/archive/v0.8.0.tar.gz -o phxrpc-v0.8.0.tar.gz
ADD phxrpc-v0.8.0.tar.gz  $THIRD_PATH/phxrpc
WORKDIR $THIRD_PATH/phxrpc/phxrpc-0.8.0/third_party
RUN rm * -fr
RUN ln -s $THIRD_PATH/protobuf/protobuf-3.0.0/ protobuf
RUN cd ..; make -j 2

ADD phxsql-v0.8.0.tar.gz  /root/phxsql
WORKDIR /root/phxsql/phxsql-0.8.0/third_party
RUN rm * -fr
RUN ln -s $THIRD_PATH/glog/glog-0.3.3/ glog
RUN ln -s $THIRD_PATH/leveldb/leveldb-1.18/ leveldb
RUN ln -s $THIRD_PATH/protobuf/protobuf-3.0.0/ protobuf
RUN ln -s $THIRD_PATH/libco colib
RUN ln -s $THIRD_PATH/phxpaxos/phxpaxos-1.0.1 phxpaxos
RUN ln -s $THIRD_PATH/phxrpc/phxrpc-0.8.0 phxrpc
RUN cd ..; ln -s $THIRD_PATH/percona-server-5.6.31-77.0 percona
RUN cd ..; ./autoinstall.sh
RUN cd ..; make -j 10; make install

RUN mkdir /root/phxsql/phxsql-0.8.0/etc

ADD start.sh /root/phxsql/phxsql-0.8.0/tools/start.sh

WORKDIR /root/phxsql/phxsql-0.8.0/tools
CMD sh start.sh

# CMD python

#EXPOSE $PHXSQLPROXY_PORT
#EXPOSE $AGENT_PORT
#EXPOSE $MYSQL_PORT 
#EXPOSE $PAXOS_PORT
