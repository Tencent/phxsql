
import sys
import os
import binary_operator
import directory_operator

class binary_installer:
	def __init__( self, args ):
		self.m_args = args
	
	def install_all( self ):
		install_phxbinlogsvr_ret = self.install_phxbinlogsvr()
		if install_phxbinlogsvr_ret != 0:
			print 'install phxbinlogsvr failed, ret %d'%(install_phxbinlogsvr_ret)
			return install_phxbinlogsvr_ret

		install_mysql_ret = self.install_mysql()
		if install_mysql_ret != 0:
			print 'install mysql failed, ret %d'%(install_mysql_ret)
			return install_mysql_ret

		return 0

	def install_mysql( self ):
		op = binary_operator.binary_operator( self.m_args )
		if op.kill_mysql() != 0:
			print 'kill mysql failed'
			return sys._getframe().f_lineno

		do = directory_operator.directory_operator( self.m_args )
		if do.mkdir_for_mysql() != 0:
			print 'mkdir for mysql failed'
			return sys._getframe().f_lineno

		cmd = 'cd %s/percona.src; ./scripts/mysql_install_db --defaults-file=%s/etc/my.cnf --user=mysql'%( self.m_args.base_dir, self.m_args.base_dir )
		print cmd
		return os.system( cmd )

	def install_phxbinlogsvr( self ):
		op = binary_operator.binary_operator( self.m_args )
		if op.kill_phxbinlogsvr() != 0:
			print 'kill phxbinlogsvr failed'
			return sys._getframe().f_lineno

		do = directory_operator.directory_operator( self.m_args )
		if do.mkdir_for_phxbinlogsvr() != 0:
			print 'mkdir for phxbinlogsvr failed'
			return sys._getframe().f_lineno

		return 0
	


