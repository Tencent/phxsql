
import os
from build_comm import *

base_dir=""
lib_dir=""
bin_dir=""

third_party_list=["PROTOBUF", "LEVELDB", "GFLAGS", "GLOG", "PHXPAXOS", "PHXPAXOS_PLUGIN", "COLIB"]

def GetPath(key):
	makefile_def=open("makefile.mk")
	lines = makefile_def.readlines()
	makefile_def.close()

	for line in lines:
		key_value = line.split('=')
		if( str.strip(key_value[0]) == key ):
			res=str.strip(key_value[1])
			res=res.replace("$(SRC_BASE_PATH)",base_dir);
			res=res.replace("$(PHXSQL_LIB_PATH)",lib_dir);
			return res
	return ""

def GetPathPrefix(key):
	makefile_def=open("makefile.mk")
	lines = makefile_def.readlines()
	makefile_def.close()

	res_list=[]
	for line in lines:
		key_value = line.split('=')
		if( str.strip(key_value[0])[0:len(key)] == key and str.strip(key_value[0])[-4:]=="PATH"):
			res=str.strip(key_value[1])
			res=res.replace("$(SRC_BASE_PATH)",base_dir);
			res=res.replace("$(PHXSQL_LIB_PATH)",lib_dir);
			res_list.append( (str.strip(key_value[0]),res) )
	return res_list

def CheckMySql():
	if( not os.path.exists( base_dir+"/percona") ):
		print "percona directory(%s) not found" % (base_dir + "/percona")
		print "please make sure percona 5.6 has been placed on the source directory and named \"percona\","
		print "you can download percona 5.6 from https://github.com/percona/percona-server.git."
		exit(1)


def CheckBasePath():
	global lib_dir, sbin_dir
	lib_dir=GetPath("PHXSQL_LIB_PATH")

	if( not os.path.exists( lib_dir ) ):
		os.mkdir( lib_dir )

	sbin_dir=GetPath("PHXSQL_SBIN_PATH")

	if( not os.path.exists( sbin_dir ) ):
		os.mkdir( sbin_dir )

	extlib_dir=GetPath("PHXSQL_EXTLIB_PATH")
	if( not os.path.exists( extlib_dir ) ):
		os.mkdir( extlib_dir )

def Check3rdPath():
	for lib in third_party_list:
		path_list=GetPathPrefix(lib)
		for path in path_list:
			if( not os.path.exists( path[1] ) ):
				print "%s not found" % path[1]
				print "please make sure %s has been placed on the third party directory" % lib
				exit(1)

def CheckProtobufVersion():
	protobuf_bin_path = base_dir + "/third_party/protobuf/bin"
	if( not os.path.exists( protobuf_bin_path ) ):
		print "please make sure protobuf 3.0+ has been installed on the third party directory"
		print "and %s/protoc can be detected" % protobuf_bin_path
		exit(1)

	cmd = "%s/protoc --version | grep libprotoc.3" % protobuf_bin_path
	cmd_res = os.popen( cmd, 'r' )
	res=cmd_res.read()
	cmd_res.close()
	if( res == '' ):
		print "protobuf %s has beed found",res
		print "please make sure protobuf 3.0+ has been installed on the third party directory"
		exit(1)

if(__name__ == '__main__'):
	base_dir=sys.argv[1]
	CheckBasePath()
	Check3rdPath()
	CheckMySql()
	CheckProtobufVersion()
	exit(0)
