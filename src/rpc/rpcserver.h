

#pragma once

#include <vector>
#include <string>
#include <map>


#include "thpool.h"

namespace simprpc{

class RPCConnection;
class RPCMethod;
class RPCServer;

// This class will handle connection management and promise access/modify connection
// safely in parallel
class ConnectionManager{
public:
  ConnectionManager(size_t sz = 7): _bucket_sz(sz), _locks(sz), _recorders(sz){ }
  ~ConnectionManager();

  // Access fucntions
  // return cresponding connection class with given socket file descriptor
  RPCConnection* find(int sockfd);
  
  // register a new connection
  bool add(int sockfd, RPCServer* ps);

  // close a connection
  bool close(int sockfd);

  // close all connection and shutdown the manager
  void shutdown();

private:
  size_t _bucket_sz;
  std::vector<std::mutex> _locks;
  std::vector<std::map<int, RPCConnection*> > _recorders;

  size_t _getBuckId(int val) const;
};


class RPCServer{
public:
  // RPCServer(size_t thpoll_sz)std::string
  // RPCServer(const char* ip, int port);
  RPCServer() = delete;
  RPCServer(const char* ip, int port, size_t thpoll_sz=10, size_t bucksz=7);
  ~RPCServer();

  void start();

  bool registMethod(RPCMethod* method);

  RPCMethod* getMethod(const std::string& s) const { 
    auto it = _methodMap.find(s);
    if(it == _methodMap.end())
      return nullptr;
    return it->second;
  }

  // bool removeMethod(std::string& methodName);


private:
  // TODO: change to shared_ptr
  std::map<std::string, RPCMethod*> _methodMap;
  ConnectionManager _connectionManager;
  ThreadPool _thpool;
  int _listenfd;

  void _inEvents(int fd, int epfd); // execute method actually happens in _inEvents
  // void _outEvents(int fd, int epfd);

};

}