
import sys
import os


class directory_operator:
	def __init__( self, args ):
		self.m_args = args
		os.system( 'mkdir %s/log'%self.m_args.data_dir )


	def mkdir_for_mysql( self ):
		mysql_data_dir = '%s/percona.workspace'%self.m_args.data_dir
		if os.path.isdir( mysql_data_dir ):
			ret = os.system( 'rm -rf %s'%mysql_data_dir )
			if ret != 0:
				print 'delete mysql data dir [%s] failed'%mysql_data_dir
				return ret

		print mysql_data_dir
		os.system( 'mkdir %s'%mysql_data_dir )
		os.system( 'mkdir %s/data'%mysql_data_dir )
		os.system( 'mkdir %s/tmp'%mysql_data_dir )
		os.system( 'mkdir %s/binlog'%mysql_data_dir )
		os.system( 'mkdir %s/innodb'%mysql_data_dir )
		os.system( 'chown user:mysql -R %s'%mysql_data_dir )

		return 0

	def mkdir_for_phxbinlogsvr( self ):
		phxbinlogsvr_data_dir = '%s/phxbinlogsvr'%(self.m_args.data_dir)
		if os.path.isdir( phxbinlogsvr_data_dir ):
			ret = os.system( 'rm -rf %s'%phxbinlogsvr_data_dir )
			if ret != 0:
				print 'delete phxbinlogsvr data dir [%s] failed'%phxbinlogsvr_data_dir
				return ret
		
		print phxbinlogsvr_data_dir
		os.system( 'mkdir %s'%phxbinlogsvr_data_dir )
		os.system( 'mkdir %s/phxbinlogsvr'%phxbinlogsvr_data_dir )
		os.system( 'mkdir %s/phxbinlogsvr/event_data'%phxbinlogsvr_data_dir )
		os.system( 'mkdir %s/phxbinlogsvr/paxoslog'%phxbinlogsvr_data_dir )

		return 0

