
import sys
import os
import time
import name_config
from phxsql_utils import *

class binary_operator:
	def __init__( self, args ):
		self.m_args = args;
	
	def kill_all( self ):
		kill_mysql_ret = self.kill_mysql()
		if kill_mysql_ret != 0:
			print 'kill mysql failed, exit......'
			return kill_mysql_ret

		kill_phxbinlogsvr_ret = self.kill_phxbinlogsvr()
		if kill_phxbinlogsvr_ret != 0:
			print 'kill phxbinlogsvr failed, exit......'
			return kill_phxbinlogsvr_ret

		kill_phxsqlproxy_ret = self.kill_phxsqlproxy()
		if kill_phxsqlproxy_ret != 0:
			print 'kill phxsqlproxy failed, exit......'
			return kill_phxsqlproxy_ret
		return 0

	def comm_kill_binary( self, binary_name ):
		pwd = os.getcwd()
		cmd = 'ps -ef | grep %s | grep %s'%( binary_name, self.m_args.base_dir)
		ret = os.popen( cmd ).xreadlines()
		pids = str()
		for lines in ret:
			if lines.find( cmd ) != -1:
				continue
			pids += "%s "%( lines.split()[1] )
		if len(pids):
			cmd = 'kill -9 %s'%( pids )
			print cmd
			return os.system( cmd )
		return 0


	def kill_mysql( self ):
		return self.comm_kill_binary( 'mysqld' )

	def kill_phxbinlogsvr( self ):
		return self.comm_kill_binary( 'phxbinlogsvr' )

	def kill_phxsqlproxy( self ):
		return self.comm_kill_binary( 'phxsqlproxy' )

	def start_binary( self ):
		if self.kill_all() != 0:
			print 'kill all failed'
			return sys._getframe().f_lineno
		
		if self.start_mysql() != 0:
			print 'start mysql failed'
			return sys._getframe().f_lineno

		if self.start_phxbinlogsvr() != 0:
			print 'start phxbinlogsvr failed'
			return sys._getframe().f_lineno

		if self.start_phxsqlproxy() != 0:
			print 'start phxsqlproxy failed'
			return sys._getframe().f_lineno

		return 0

	def start_mysql( self ):
		cmd = 'nohup sh %s --defaults-file=%s --super_read_only &' \
				%( format_path('%s/bin/mysqld_safe'%self.m_args.base_dir), format_path('%s/etc/my.cnf'%self.m_args.base_dir) )
		return os.system(cmd)


	def start_phxbinlogsvr( self ):
		cmd = 'nohup %s &'%( format_path('%s/sbin/%s'%(self.m_args.base_dir, name_config.phxbinlogsvr) ) )
		return os.system(cmd)


	def start_phxsqlproxy( self ):
		cmd = '%s %s daemon'%( format_path('%s/sbin/%s'%(self.m_args.base_dir, name_config.phxsqlproxy)), \
			format_path('%s/etc/phxsqlproxy.conf'%self.m_args.base_dir) )
		return os.system(cmd)

	def restart_mysql( self ):
		self.kill_mysql()
		time.sleep(1)
		if self.m_args.new_process != "":
			cmd = 'cp %s %s'%( format_path(self.m_args.new_process), format_path( '%s/sbin/'%self.m_args.base_dir ) )
			os.system( cmd )
		return self.start_mysql()

	def restart_phxsqlproxy( self ):
		self.kill_phxsqlproxy()
		time.sleep(1)
		if self.m_args.new_process != "":
			cmd = 'cp %s %s'%( format_path(self.m_args.new_process), format_path( '%s/sbin/'%self.m_args.base_dir ) )
			os.system( cmd )
		return self.start_phxsqlproxy()

	def restart_phxbinlogsvr( self ):
		self.kill_phxbinlogsvr()
		time.sleep(1)
		if self.m_args.new_process != "":
			cmd = 'cp %s %s'%( format_path(self.m_args.new_process), format_path( '%s/sbin/'%self.m_args.base_dir ) )
			os.system( cmd )
		return self.start_phxbinlogsvr()



























