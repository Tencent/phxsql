### Note
阅读代码，添加注释

- [ ] 厘清 Master 透传请求的过程

- phxsqlproxy
  1. GetDestEndpoint
  ```
  int SlaveIORoutine::GetDestEndpoint(std::string & dest_ip, int & dest_port)
  ```
  2. GetDestEndpoint
  ```
  int MasterIORoutine::GetDestEndpoint(std::string & dest_ip, int & dest_port)
  ```