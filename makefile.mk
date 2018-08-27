
version = 0.8.5

SRC_BASE_PATH ?= .
PREFIX?=$(SRC_BASE_PATH)
PHXSQL_LIB_PATH = $(SRC_BASE_PATH)/.lib
PHXSQL_SBIN_PATH = $(SRC_BASE_PATH)/.sbin
PHXSQL_EXTLIB_PATH = $(PHXSQL_LIB_PATH)/extlib

NANOPBPATH=$(SRC_BASE_PATH)/third_party/nanopb/

PROTOBUF_INCLUDE_PATH=$(SRC_BASE_PATH)/third_party/protobuf/include
LEVELDB_INCLUDE_PATH=$(SRC_BASE_PATH)/third_party/leveldb/include
GFLAGS_INCLUDE_PATH=$(SRC_BASE_PATH)/third_party/gflags/include
GLOG_INCLUDE_PATH=$(SRC_BASE_PATH)/third_party/glog/include
PHXPAXOS_INCLUDE_PATH=$(SRC_BASE_PATH)/third_party/phxpaxos/include
PHXPAXOS_PLUGIN_PATH=$(SRC_BASE_PATH)/third_party/phxpaxos/plugin/include
MYSQL_INCLUDE_PATH=$(SRC_BASE_PATH)/percona/include
COLIB_INCLUDE_PATH=$(SRC_BASE_PATH)/third_party/colib
PHXRPC_INCLUDE_PATH=$(SRC_BASE_PATH)/third_party/phxrpc

PROTOBUF_LIB_PATH=$(SRC_BASE_PATH)/third_party/protobuf/lib
LEVELDB_LIB_PATH=$(SRC_BASE_PATH)/third_party/leveldb/lib/
GFLAGS_LIB_PATH=$(SRC_BASE_PATH)/third_party/gflags/lib
GLOG_LIB_PATH=$(SRC_BASE_PATH)/third_party/glog/lib
MYSQL_LIB_PATH=$(SRC_BASE_PATH)/percona/libmysql
PHXPAXOS_LIB_PATH=$(SRC_BASE_PATH)/third_party/phxpaxos/lib
COLIB_LIB_PATH=$(SRC_BASE_PATH)/third_party/colib/lib
PHXRPC_LIB_PATH=$(SRC_BASE_PATH)/third_party/phxrpc/lib


CXX=g++
CXXFLAGS+=-std=c++11
CPPFLAGS+=-I$(SRC_BASE_PATH) -I$(PROTOBUF_INCLUDE_PATH)  -I$(LEVELDB_INCLUDE_PATH)
CPPFLAGS+=-I$(GFLAGS_INCLUDE_PATH) -I$(GLOG_INCLUDE_PATH)
CPPFLAGS+=-Wall -g -fPIC -m64

LDFLAGS += -L$(PHXPAXOS_LIB_PATH) -L$(PHXSQL_LIB_PATH) -L$(PROTOBUF_LIB_PATH) -L$(LEVELDB_LIB_PATH)
LDFLAGS += -L$(GFLAGS_LIB_PATH) -L$(GLOG_LIB_PATH) -L$(GRPC_LIBE_PATH) -L$(OPEN_SSL_LIB_PATH) -L$(MYSQL_LIB_PATH)
LDFLAGS += -L$(COLIB_LIB_PATH)
LDFLAGS += -static-libgcc -static-libstdc++
LDFLAGS += -Wl,--no-as-needed


#=====================================================================================================

PROTOC = $(SRC_BASE_PATH)/third_party/protobuf/bin/protoc
PROTOS_PATH = .
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
NANOPB_PLUGIN_PATH=$(NANOPBPATH)/generator/protoc-gen-nanopb

vpath %.proto $(PROTOS_PATH)

.PRECIOUS: %.nano.pb.cc
%.nano.pb.cc: %.proto
	$(PROTOC) --plugin=protoc-gen-nanopb=$(NANOPB_PLUGIN_PATH) --nanopb_out=nanoproto/ $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I$(PROTOBUF_INCLUDE_PATH) -I $(PROTOS_PATH) --cpp_out=. $<

.PHONY: install
install:
	make install -C percona
	@mkdir $(PREFIX)/lib -p;\
	rm $(PREFIX)/install_package -rf; \
	mkdir $(PREFIX)/install_package -p;\
	mkdir $(PREFIX)/install_package/lib -p;\
	mkdir $(PREFIX)/install_package/sbin -p; \
	mkdir $(PREFIX)/install_package/percona.src -p;
	@echo create install_package directory to $(PREFIX)/install_package;
	@echo install install_package in $(PREFIX)/install_package;
	@echo install mysql in $(PREFIX)/install_package/sbin;
	@echo install mysql plugin in $(PREFIX)/install_package/lib;
	@cp $(SRC_BASE_PATH)/LICENSE $(PREFIX)/install_package/;
	@cp $(SRC_BASE_PATH)/README.md $(PREFIX)/install_package/;
	@cp $(SRC_BASE_PATH)/phxrpc_package_config/* $(PREFIX)/install_package/ -rf;
	@cp $(SRC_BASE_PATH)/.sbin/* $(PREFIX)/install_package/sbin/ -rf;
	@cp $(SRC_BASE_PATH)/percona/bin/mysqld $(PREFIX)/install_package/sbin/ -rf;
	@cp $(SRC_BASE_PATH)/percona/bin $(PREFIX)/install_package/percona.src/ -r;
	@cp $(SRC_BASE_PATH)/percona/lib $(PREFIX)/install_package/percona.src/ -r;
	@cp $(SRC_BASE_PATH)/percona/share $(PREFIX)/install_package/percona.src/ -r;
	@cp $(SRC_BASE_PATH)/percona/scripts $(PREFIX)/install_package/percona.src/ -r;


.PHONY:package
package:
	@echo creating package phxsql-$(version).tar.gz...
	@rm phxsql phxsql-$(version).tar.gz -rf;\
	cp $(PREFIX)/install_package phxsql -r;\
	tar -zcf phxsql-$(version).tar.gz phxsql;\
	rm  phxsql -rf;\
