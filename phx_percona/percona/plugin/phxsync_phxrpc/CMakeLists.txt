# Copyright (c) 2006, 2010, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

ADD_DEFINITIONS(-std=gnu++0x -DNO_USE_PAXOS_PLUGIN -DMYSQL_SERVER )
EXEC_PROGRAM(g++ ARGS -print-file-name=libstdc++.a OUTPUT_VARIABLE STDCLIBPATH)

SET(BASE_PATH ${CMAKE_SOURCE_DIR}/../)

INCLUDE_DIRECTORIES(  
	${BASE_PATH}/include
	${BASE_PATH}/third_party/phxpaxos/include
	${BASE_PATH}/third_party/phxpaxos/plugin/include/
)

SET( PHXCLIENTLIB_SRC 
		libphxbinlogsvrclient_phxrpc.a
		libphxglog.a
		libglog.a
		libphxrpc.a
		libprotobuf.a 
		pthread
		${STDCLIBPATH}
		)

SET( PHXCLIENTLIB_DIR 
		${BASE_PATH}/third_party/phxrpc/lib
		${BASE_PATH}/third_party/phxpaxos/lib
		${BASE_PATH}/third_party/glog/lib 
		${BASE_PATH}/third_party/protobuf/lib 
		${BASE_PATH}/third_party/boost/lib 
		${BASE_PATH}/.lib/extlib
		)

LINK_DIRECTORIES( ${PHXCLIENTLIB_DIR} )

SET( PHXSYNC_MASTER_PHXRPC_SOURCES  phxsync_master_phxrpc_plugin.cc phxsync_utils.cc init_with_phxbinlogsvr.cc )

MYSQL_ADD_PLUGIN( phxsync_master_phxrpc ${PHXSYNC_MASTER_PHXRPC_SOURCES})

TARGET_LINK_LIBRARIES( phxsync_master_phxrpc ${PHXCLIENTLIB_SRC} )

SET_TARGET_PROPERTIES( phxsync_master_phxrpc PROPERTIES LINK_FLAGS
	"${mysqld_link_flags} -Wl,-soname,--whole-archive")
