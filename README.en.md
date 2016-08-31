**PhxSQL is a high-availability and strong-consistency MySQL cluster built on Paxos and Percona.**

#PhxSQL features:
  - high resilience to nodes failure and network partition: the cluster works well when more than half of cluster nodes work and are interconnected.
  - high availability by automatic election of master node.
  - guarantee of data consistency among cluster nodes.
  - easy deployment and easy maintenance: PhxSQL, powered by in-house implementation of Paxos, has only 4 components including MySQL and doesn't depend on zookeeper or etcd for anything. PhxSql supports automated cluster membership hot reconfiguration.
  - complete compliance with MySQL and MySQL client.

