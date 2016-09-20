## Note
阅读代码，添加注释  

### TODO List
- [ ] 阅读 phxsqlproxy 模块代码
- [ ] 阅读 phxbinlogsvr 模块代码
- [ ] 厘清 Master 透传请求的过程
- [ ] 厘清 phxbinlogsvr 选举 master 的过程

### phxsqlproxy 模块
1. ConnectDest  
  ```
  int IORoutine::ConnectDest()
  ```
2. GetDestEndpoint  
  ```
  int MasterIORoutine::GetDestEndpoint(std::string & dest_ip, int & dest_port)
  ```
