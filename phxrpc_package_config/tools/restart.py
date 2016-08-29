
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

  
def get_ip_address(ifname):
	"""
	>>> get_ip_address('lo')
	'127.0.0.1'

	>>> get_ip_address('eth0')
	'38.113.228.130'
	"""
	ip = str()
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		ip = socket.inet_ntoa(fcntl.ioctl(
							s.fileno(),
							0x8915,  # SIOCGIFADDR
							struct.pack('256s', ifname[:15])
							)[20:24])
	except:
		ip = ""
	return ip

def ParseArguments():
	parser.add_argument( '-p', '--process_name', help="which binary you want to restart[mysql|phxbinlogsvr|phxsqlproxy]", type=str, option_strings=['mysql', 'phxsqlproxy', 'phxbinlogsvr'], required =True )
#, default="" )
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

	if args.process_name != 'mysql' and args.process_name != 'phxsqlproxy' and args.process_name != 'phxbinlogsvr':
		print 'the process_name should be mysql or phxsqlproxy or phxbinlogsvr'
		sys.exit(-1)

	op = binary_operator.binary_operator( args )

	if args.process_name == 'mysql':
		if op.restart_mysql() != 0:
			print 'start mysql failed, exit.....'
	
	if args.process_name == 'phxsqlproxy':
		if op.restart_phxsqlproxy() != 0:
			print 'start phxsqlproxy failed, exit.....'

	if args.process_name == 'phxbinlogsvr':
		if op.restart_phxbinlogsvr() != 0:
			print 'start phxbinlogsvr failed, exit.....'

