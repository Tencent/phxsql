
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
	parser.add_argument( '-m', '--module_name', help="module name", type=str, default="test")
	parser.add_argument( '-i', '--ip_list', help="IPs of group hosts, split by ','", type=str, default="" )
	parser.add_argument( '-p', '--phxsqlproxy_port', help="phxsqlproxy for mysql client to connect", type=int, default=54321)
	parser.add_argument( '-g', '--agent_port', help="agent for mysql to learn binlog", type=int, default=6001)
	parser.add_argument( '-y', '--mysql_port', help="mysql port for phxsqlproxy to connect", type=int, default=13306)
	parser.add_argument( '-P', '--phxbinlogsvr_port', help="phxbinlogsvr port", type=int, default=17000)
	parser.add_argument( '-a', '--paxos_port', help="port for paxos protocol to communicate", type=int, default=8001)
	parser.add_argument( '-s', '--skip_data', help="do not delete datas in data_dir", type=int, default=0)
	parser.add_argument( '-f', '--data_dir', help="where phxbinlogsvr and paxos log been stored", type=str, default="/data/phxsql/" )
	parser.add_argument( '-b', '--base_dir', help="where phxsql located, no need to specify it", type=str, default="" )
	parser.add_argument( '-n', '--inner_ip', help="host inner ip,we will figure it out on system config, no need to specify it", type=str, default="" )
	parser.add_argument( '-r', '--process_name', help="which binary you want to install[mysql|phxbinlogsvr|phxsqlproxy]", type=str, option_strings=['mysql',  'phxbinlogsvr', 'all'], default='all')

	base_dir = str()
	pwd = os.getcwd()
	for directory in pwd.split('/'):
		if directory == 'tools' or directory == 'install_tools':
			break
		base_dir += (directory + "/")
	
	args = parser.parse_args()
	if args.base_dir == "":
		args.base_dir = base_dir


	for eth in os.listdir('/sys/class/net/'):
		eth_ip = get_ip_address( eth )
		if eth_ip != "" and args.ip_list.find( get_ip_address( eth ) ) != -1:
			args.inner_ip = eth_ip

	if args.inner_ip == "": 
		print 'We can not figure out this host\'s ip in your given ip list, please check the iplist or assign the ip by --inner_ip, for more usage, use --help'
		return -1, []

	print args
	return 0, args

if __name__ == '__main__':
	ret, args = ParseArguments()
	if ret == -1:
		sys.exit(-1)

	if args.process_name == 'all':
		op = binary_operator.binary_operator(args)
	
		kill_binary_ret = op.kill_all()
		if kill_binary_ret != 0:
			print 'kill binarys failed, exit......'
			sys.exit(-1)
		print 'kill all binaries success....'
	
		cg = config_generator.config_generator( args )
		generate_config_ret = cg.generate_config()
		if generate_config_ret != 0:
			print 'generate config failed, exit......'
			sys.exit(-1)
		print 'generate all configs success....'
	
		installer = binary_installer.binary_installer( args )
		install_all_ret = installer.install_all()
		if install_all_ret != 0:
			print 'install failed, exit......'
			sys.exit(-1)
		print 'install all success....'
	
		time.sleep(5)
	
		start_binary_ret = op.start_binary();
		if start_binary_ret != 0:
			print 'start binary failed, exit......'
			sys.exit(-1)

	elif args.process_name == 'mysql':
		op = binary_operator.binary_operator(args)
	
		kill_binary_ret = op.kill_mysql()
		if kill_binary_ret != 0:
			print 'kill mysql failed, exit......'
			sys.exit(-1)
		print 'kill mysql success....'
	
		installer = binary_installer.binary_installer( args )
		install_all_ret = installer.install_mysql()
		if install_all_ret != 0:
			print 'install mysql failed, exit......'
			sys.exit(-1)
		print 'install mysql success....'
	
		time.sleep(5)
	
		start_binary_ret = op.start_mysql();
		if start_binary_ret != 0:
			print 'start mysql failed, exit......'
			sys.exit(-1)
	
	elif args.process_name == 'phxbinlogsvr':
		op = binary_operator.binary_operator(args)
	
		kill_binary_ret = op.kill_phxbinlogsvr()
		if kill_binary_ret != 0:
			print 'kill phxbinlogsvr failed, exit......'
			sys.exit(-1)
		print 'kill phxbinlogsvr success....'
	
		installer = binary_installer.binary_installer( args )
		install_all_ret = installer.install_phxbinlogsvr()
		if install_all_ret != 0:
			print 'install phxbinlogsvr failed, exit......'
			sys.exit(-1)
		print 'install phxbinlogsvr success....'
	
		time.sleep(5)
	
		start_binary_ret = op.start_phxbinlogsvr();
		if start_binary_ret != 0:
			print 'start binary failed, exit......'
			sys.exit(-1)
