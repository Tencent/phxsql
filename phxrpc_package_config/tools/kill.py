
import time
import sys
import os
import argparse
import binary_operator
import config_generator 
import binary_installer
import socket
import fcntl
import struct

parser = argparse.ArgumentParser()
args = argparse.Namespace()

def ParseArguments():
	parser.add_argument( '-p', '--process_name', help="which binary you want to restart[mysql|phxbinlogsvr|phxsqlproxy]", type=str, option_strings=['mysql', 'phxsqlproxy', 'phxbinlogsvr'], default='all' )
	parser.add_argument( '-b', '--base_dir', help="where phxsql locate, no need to specify it", type=str, default="" )
	parser.add_argument( '-n', '--new_process', help="where new process locate, we will copy it", type=str, default="" )

	base_dir = str()
	pwd = os.getcwd()
	for directory in pwd.split('/'):
		if directory == 'tools' or directory == 'py_tools':
			break
		base_dir += (directory + "/")
	
	args = parser.parse_args()
	if args.base_dir == "":
		args.base_dir = base_dir


	print args
	return 0, args

if __name__ == '__main__':
	ret, args = ParseArguments()
	if ret == -1:
		sys.exit(-1)

	if args.process_name != 'mysql' and args.process_name != 'phxsqlproxy' and args.process_name != 'phxbinlogsvr' and args.process_name != 'all':
		print 'the process_name should be mysql or phxsqlproxy or phxbinlogsvr or all'
		sys.exit(-1)

	op = binary_operator.binary_operator( args )

	if args.process_name == 'all':
		if op.kill_all() != 0:
			print 'kill all failed, exit.....'

	if args.process_name == 'mysql':
		if op.kill_mysql() != 0:
			print 'kill mysql failed, exit.....'
	
	if args.process_name == 'phxsqlproxy':
		if op.kill_phxsqlproxy() != 0:
			print 'kill phxsqlproxy failed, exit.....'

	if args.process_name == 'phxbinlogsvr':
		if op.kill_phxbinlogsvr() != 0:
			print 'kill phxbinlogsvr failed, exit.....'

