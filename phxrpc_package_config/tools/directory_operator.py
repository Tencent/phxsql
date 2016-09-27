
import sys
import os
from phxsql_utils import *


class directory_operator:
	def __init__( self, args ):
		self.m_args = args
		os.system( 'mkdir %s'%format_path( '%s/log'%self.m_args.data_dir ) )


	def mkdir_for_mysql( self ):
		mysql_data_dir = format_path( '%s/percona.workspace'%self.m_args.data_dir )
		if os.path.isdir( mysql_data_dir ):
			ret = os.system( 'rm -rf %s'%mysql_data_dir )
			if ret != 0:
				print 'delete mysql data dir [%s] failed'%mysql_data_dir
				return ret

		print mysql_data_dir
		os.system( 'mkdir %s'%mysql_data_dir )
		os.system( 'mkdir %s'%(format_path( '%s/data'%mysql_data_dir)) )
		os.system( 'mkdir %s'%(format_path( '%s/tmp'%mysql_data_dir)) )
		os.system( 'mkdir %s'%(format_path( '%s/binlog'%mysql_data_dir)) )
		os.system( 'mkdir %s'%(format_path( '%s/innodb'%mysql_data_dir)) )
		os.system( 'chown mysql:mysql -R %s'%mysql_data_dir )

		return 0

	def mkdir_for_phxbinlogsvr( self ):
		phxbinlogsvr_data_dir = format_path('%s/phxbinlogsvr'%(self.m_args.data_dir))
		if os.path.isdir( phxbinlogsvr_data_dir ):
			ret = os.system( 'rm -rf %s'%phxbinlogsvr_data_dir )
			if ret != 0:
				print 'delete phxbinlogsvr data dir [%s] failed'%phxbinlogsvr_data_dir
				return ret
		
		print phxbinlogsvr_data_dir
		os.system( 'mkdir %s'%phxbinlogsvr_data_dir )
		os.system( 'mkdir %s'%(format_path( '%s/phxbinlogsvr'%phxbinlogsvr_data_dir)) )
		os.system( 'mkdir %s'%(format_path( '%s/phxbinlogsvr/event_data'%phxbinlogsvr_data_dir)) )
		os.system( 'mkdir %s'%(format_path( '%s/phxbinlogsvr/paxoslog'%phxbinlogsvr_data_dir)) )

		return 0

