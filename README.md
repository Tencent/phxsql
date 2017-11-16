[简体中文README](https://github.com/Tencent/phxsql/blob/master/README.zh_cn.md)

**PhxSQL is a high-availability and strong-consistency MySQL cluster built on Paxos and Percona.**

Authors: Junchao Chen, Haochuan Cui, Duokai Huang, Ming Chen and Sifan Liu 

Contact us: phxteam@tencent.com

[![Build Status](https://travis-ci.org/Tencent/phxsql.svg?branch=master)](https://travis-ci.org/Tencent/phxsql)

#PhxSQL features:
  - high availability by automatic failovers: the cluster works well when more than half of cluster nodes work and are interconnected.
  - guarantee of data consistency among cluster nodes: replacing loss-less semi-sync between MySQL master and MySQL slaves with Paxos, PhxSQL ensures zero-loss binlogs between master and slaves and supports linearizable consistency, which is as strong as that of Zookeeper.
  - complete compliance with MySQL and MySQL client: PhxSQL supports up to serializable isolation level of transaction.
  - easy deployment and easy maintenance: PhxSQL, powered by in-house implementation of Paxos, has only 4 components including MySQL and doesn't depend on zookeeper or etcd for anything. PhxSql supports automated cluster membership hot reconfiguration.
  


This project includes 
* Source codes
* Third party submodules
* Pre-compiled binaries for Ubuntu 64bit system.
    
Projects on which this project depends are also published by Tencent( phxpaxos, phxrpc, libco ).
You can download or clone them with --recurse-submodule.

**phxpaxos：** [http://github.com/Tencent/phxpaxos](http://github.com/Tencent/phxpaxos "http://github.com/Tencent/phxpaxos")

**phxrpc：** [http://github.com/Tencent/phxrpc](http://github.com/Tencent/phxrpc "http://github.com/Tencent/phxrpc")

**libco：** [http://github.com/Tencent/libco](http://github.com/Tencent/libco "http://github.com/Tencent/libco")

# Compilation of PhxSQL

>If you prefer pre-compiled binaries, just skip this part.

### Structure of PhxSQL Directories
* PhxSQL
    * phxsqlproxy
    * phxbinlogsvr
    * percona
    * phx_percona
        * plugin
        * phxsync_phxrpc
        * semisync
    * third_party
        * glog
        * leveldb
        * protobuf
        * phxpaxos
        * colib
        * phxrpc
    * tools
    * phxrpc_package_config

### Introduction of Directories.

| Name | Introduction |
| ------| ------ |
| phxsqlproxy | surrogate between MySQL client and PhxSql |
| phxbinlogsvr | server for global binlog synchronization and storage, as well as management of mastership and membership |
| percona | Source code of percona5.6.31-77.0 |
| phx_percona/plugin/phxsync_phxrpc | A plugin running in MySql that intercepts MySQL binlogs and forwards them to phxbinlogsvr |
| phx_percona/plugin/semisync | A semisync compatible with our modified plugin APIs of MySQL |
| third_party/glog | GLOG library
| third_party/leveldb | LevelDB library
| third_party/protobuf | Google Protobuf 3.0+ library
| third_party/phxpaxos| PhxPaxos library
| third_party/colib| Libco library
| third_party/phxrpc | Phxrpc library


### Preparation

##### Installation of third party libs

PhxSQL needs 6 third party libs(glog, leveldb, protobuf, phxpaxos, colib, phxrpc). Please install them in phxsql/third_party directory or just link to third_party.

**NOTE: Please make sure -fPIC is added while executing configure in GLOG and Protobuf as well as specifying --prefix=/the/current/absolute/path.**

For example: `./configure CXXFLAGS=-fPIC --prefix=/home/root/phxsql/third_party/glog`.

**Then download**  [percona-server-5.6.31-77.0.tar.gz](https://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.31-77.0/source/tarball/percona-server-5.6.31-77.0.tar.gz)

Move `percona-server-5.6\_5.6.31-77.0` to PhxSQL directory, rename or link as 'percona'
**(NOTE: Only percona-server-5.6\_5.6.31-77.0 is available)**


##### Preparation of installation enviroment
1. Execute `./autoinstall.sh && make && make install`
2. Execute 'make package' to generate a tar.gz package so you can transfer to your target hosts.

**(NOTE: We put the binaries in install_package/sbin, configuration files in install_package/tools/etc_template, install scripts in install_package/tools. The 'make package' command will pack 'install_package' into 'phxsql-$version.tar.gz'. Please specify -prefix=/the/path/you/want/to/install while executing ./autoinstall.sh)**


# Deployment of PhxSQL

### Host requirements.

>PhxSQL needs to run on more than 2 hosts. We suggest N >= 3 and N is an odd number, where N means the number of hosts.

### Initialization of PhxSQL

1. Transfer phxsql.tar.gz to all of the hosts you want to install. Then do as the following steps:
    1. Execute `tar -xvf phxsq.tar.gz .`
    2. Enter phxsql/tools, Execute `python install.py --help` to get the help of installation.
    
        (For example:`python2.7 install.py -i"your_inner_ip" -p 54321 -g 6000 -y 11111 -P 17000 -a 8001 -f/tmp/data/`)

2.  After executing 'install.py' on all the hosts, Execute './phxbinlogsvr_tools_phxrpc -f InitBinlogSvrMaster -h"ip1,ip2,ip3" -p 17000' in any one hosts. 17000 should be replaced with the port on which phxbinlogsvr is listening.
3.  The cluster is active while a message shows master initialization is finished.
4.  You can execute some SQLs to check the status of cluster through `mysql -uroot -h"your_inner_ip" -P$phxsqlproxy_port` 

### Simple tests.

1. Enter phxsql/tools/
2. Execute `test_phxsql.sh phxsqlproxy_port ip1 ip2 ip3`

### Description of Configuration Files

> PhxSQL have 3 configuration files in total.

###### 1. my.cnf： The configuration of MySQL. Please modify it accroding to your own needs.
**NOTE:Modify `tools/etc_temlate/my.cnf` before installation, Modify `etc/my.cnf` after installation**

###### 2. phxbinlogsvr.conf 

|Section name |Key name |comment|
|------------ | ---------| ------|
|AgentOption | AgentPort | Port for the connection of binlogsvr and MySQL |
| | EventDataDir　|　Directory where to store the binlogsvr data |
| | MaxFileSize　| File size per data of phxbinlogsvr, the unit is B |
| | MasterLease | Lease length of master, the unit is second |
| | CheckPointTime | The data before CheckPointTime will be deleted by phxbinlogsvr, but it will not be deleted if some other PhxSQL nodes have not learned yet, the unit is minute |
| | MaxDeleteCheckPointFileNum　| The maximum number of files deleted each time by phxbinlogsvr |
| | FollowIP | Enabled if it is a follower node and will learn binlog from this `FollowIP`, this node will not vote |
| PaxosOption| PaxosLogPath| Directory where to store paxos data |
| | PaxosPort| Port for paxos to connect each other |
| | PacketMode | The maximum size of paxos log for PhxPaxos,1 means 100M, but the network timeout will be 1 minute, 0 means 50M and network timeout is 2s(changed in dynamic).| 
| | UDPMaxSize | Our default network use udp and tcp combination, a message we use udp or tcp to send decide by a threshold. Message size under UDPMaxSize we use udp to send. |
| Server | IP | IP for phxbinlogsvr to listen |
| | Port | Port for phxbinlogsvr to listen |
| |  LogFilePath | Directory to store log |
| | LogLevel | Log level of phxbinlogsvr |

###### 3. phxsqlproxy.conf 

| Section name | Key name | comment |
| ------| ------| ------|
|Server | IP | IP for phxsqlproxy to listen |
| | Port | Port for phxsqlproxy to listen |
| | LogFilePath  | Directory to store log |
| | LogLevel | Log level of phxbinlogsvr |
| | MasterEnableReadPort | Enable readonly-port in master node. If set to 0, master will forwarding readonly-port requests to one of slaves. |
| | TryBestIfBinlogsvrDead | After the local phxbinlogsvr is dead, phxsqlproxy will try to get master information from phxbinlogsvr on other machine, if this option set to 1. |

# PhxSQL Usasge

> phxsqlproxy is the surrogate of PhxSQL, all requests will be sent to phxsqlproxy and then be forwarded to MySQL.
>
### phxsqlproxy provides 2 different types of port for user. 

##### Master Port( also called Read-Write Port )

It is the port configured in `phxsqlproxy.conf`.
Every requests sent to this port will be forwarded to the master node to excute.

##### Slave Port( also called Read-Only Port )

It is (MasterPort + 1). You can also specify it by setting `SlavePort = xxxxx` in `phxsqlproxy.conf`.  
Every requests will be executed on the local MySQL. A master node will make a redirection to another slave nodes if  `MasterEnableReadPort = 0` (this will save the CPU/IO resource for write requests)


### SQL Command Execution

1. Using `mysql -u$user -h$phxsqlproxyip -P$phxsqlproxyport -p$pwd` to connect to phxsqlproxy
2. Execute SQL command.

>`$phxsqlproxyip` can be any one IP of hosts in a clusters and `$phxsqlproxyport` can be `MasterPort` or `SlavePort`.

# PhxSQL Management

PhxSQL provides a tool `phxbinlogsvr_tools_phxrpc` to help the mangerment of PhxSQL.

PhxSQL cluster needs 1 MySQL admin accounts and 1 synchronization account. The default admin account is (`root`, `""` ),  the default synchronization account is ( `replica`, `replica123` ), They can be modified( and only be modifyed ) via `phxbinlogsvr_tools_phxrpc`. **DON'T DO THIS MANUALLY.**

**Following is some commands you may used frequently.**

### `phxbinlogsvr_tools -f GetMasterInfoFromGlobal -h <host> -p <port>`

**Function:**Get the current master info from quorum nodes( IP and timeout ).

**Arguments:**
 
  - **Host:** Any one IP of clusters nodes
  - **Port:** Port which phxbinlogsvr is listening. like `17000` 
 
### `phxbinlogsvr_tools -f SetMySqlAdminInfo -h <host> -p <port> -u <admin username> -d <admin pwd> -U <new admin username> -D <new admin pwd>`

**Function:** Set the user and password of admin account.

**Arguments:**

  - **Host:** Any one IP of clusters nodes
  - **Port:** Port which phxbinlogsvr is listening. like `17000`
  - **Admin username:** Current account user( default is `root` )
  - **Admin pwd:** Current account password( default is `""` )
  - **New admin username:** New user
  - **New admin pwd:** New password

### `phxbinlogsvr_tools -f SetMySqlReplicaInfo -h <host> -p <port> -u <admin username> -d <admin pwd> -U <new replica username> -D <new replica pwd>`

**Function:** Set the user and password of synchronization account.

**Arguments：**

  - **Host:** Any one IP of clusters nodes
  - **Port:** Port which phxbinlogsvr is listening. like `17000`
  - **Admin username:** Current account user( default is `root` )
  - **Admin pwd:** Current account password( default is `""` )
  - **New replica username:**  New user
  - **New replica pwd:** New password
 
### `phxbinlogsvr_tools_phxrpc -f GetMemberList -h <host> -p <port>`

**Function:** Membership of this cluster, all IPs and Ports included.

**Arguments:**
 
  - **Host:** Any one IP of clusters nodes
  - **Port:** Port which phxbinlogsvr is listening. like `17000`

# Phxbinlogsvr Membership Managerment

### Member Deletion
Execute `phxbinlogsvr_tools_phxrpc -f RemoveMember -h<host> -p<port> -m <ip_of_nodeA>` to delete node A.
Once it is succesfully executed, A will not learn binlog after a small period.

### Member Involvement
1. Execute `phxbinlogsvr_tools -f AddMember -h<host> -p<port> -m <ip_of_nodeA>` to add node A into the membership.
2. Install PhxSQL on A.
3. A will begin to learn data after installation is finished.
4. Copy a snapshot of MySQL from any other nodes to A.
5. Kill phxbinlogsvr and access MySQL through the local port( or socket ). then execute `set global super_read_only = 0; set global read_only = 0`;
6. Dump the snapshot into MySQL.
7. A will begin to work after a while.

# Phxbinlogsvr fault Handling.

###### You can choose to reinstall PhxSQL if PhxSQL meets an unrecovery failure.

`Phxbinlogsvr` will pull the checkpoint in another node to reboot during reinstallation. It will self-kill after pulling is over(to make sure the consistency). You can reboot `phxbinlogsvr` after a message like `"All sm load state ok, start to exit"` appears. 

###### `phxbinlogsvr` will stop working if a data problem arise in MySQL. We suggest you to check the status of MySQL. 
###### You can observe logs with red `"err"` to check the abnormaly. 

# Performance Testing

### Hosts Infomation	
CPU :	Intel(R) Xeon(R) CPU E5-2420 0 @ 1.90GHz * 24

Memory : 32G

Disk : SSD Raid10 


### Ping Costs
Master -> Slave : 3 ~ 4ms

Client -> Master : 4ms


### Tools and Arguments
sysbench  --oltp-tables-count=10 --oltp-table-size=1000000 --num-threads=500 --max-requests=100000 --report-interval=1 --max-time=200

### Results


| Client Threads                                         | Clusters    |                    |             |     Test sets   |           |                 |               |
|------------------------------------------------------|-------------|------------------------|-------------|----------------------|-----------|---------------------|---------------|
|                                                      |             | insert.lua (100% write)     |             | select.lua (0% write)     |           | OLTP.lua (20% write)     |               |
|                                                      |             | QPS                | Response time(MS)        | QPS              | Response time(MS)      | QPS             | Response time(MS)          |
| 200                                                  | PhxSQL      | 5076               | 39.34/56.93 | 46334            | 4.21/5.12 | 25657           | 140.16/186.39 |
| 200                                                  | MySQL semi-sync | 4055               | 49.27/66.64 | 47528            | 4.10/5.00 | 20391           | 176.39/226.76 |
| 500                                                  | PhxSQL      | 8260               | 60.41/83.14 | 105928           | 4.58/5.81 | 46543           | 192.93/242.85 |
| 500                                                  | MySQL semi-sync | 7072               | 70.60/91.72 | 121535           | 4.17/5.08 | 33229           | 270.38/345.84 |

**NOTE:The 2 Response times means average and 95% percentile**
