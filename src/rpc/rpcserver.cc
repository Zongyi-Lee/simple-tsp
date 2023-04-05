
#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <cmath>

#include "rpcserver.h"
#include "rpc_method.h"
#include "rpc_connection.h"

using namespace simprpc;


// this function specify the working thread job, which is parsing xml, execute command
// and send response back to client
void th_work(RPCConnection* pc, const std::string xml) {
  pc->execute(xml);
}


static int set_nonblock(int fd) {
  int old = fcntl(fd, F_GETFL);
  if(fcntl(fd, F_SETFL, old | O_NONBLOCK) < 0) {
    std::cout << "set non-block failed.\n";
    exit(EXIT_FAILURE);
  }
  return old;
}


/* ========= RPCServer ========= */

RPCServer::RPCServer(const char* ip, int port, size_t thpoll_sz, size_t bucksz):  _connectionManager(bucksz), _thpool(thpoll_sz){
  // initialize threadpoll
  _thpool.init();

  // initialize listenfd
  struct sockaddr_in saddr;
  bzero(&saddr, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  inet_pton(AF_INET, ip, &saddr.sin_addr);

  _listenfd = socket(PF_INET, SOCK_STREAM, 0);
  if(_listenfd < 0){
    std::cout << "Error creating socket.\n";
    exit(EXIT_FAILURE);
  }

  if(bind(_listenfd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
    std::cout << "Error binding.\n";
    exit(EXIT_FAILURE);
  }

  if(listen(_listenfd, 10) < 0) {
    std::cout << "Error listening.\n";
    exit(EXIT_FAILURE);
  }

}

RPCServer::~RPCServer() {
  _methodMap.clear();
  _connectionManager.shutdown();
  _thpool.shutdown();
}

bool RPCServer::registMethod(RPCMethod* method) {
  std::string mname = method->getName();
  std::cout << "Register method: " << mname;

  std::pair<std::string, RPCMethod*> new_method{mname, method};

  std::pair<std::map<std::string, RPCMethod*>::iterator, bool> ret;
  ret = _methodMap.insert(new_method);
  if(ret.second)
    std::cout << " succeed.\n";
  else
    std::cout << " failed.\n";
  return ret.second;
}

// bool RPCServer::removeMethod(std::string& methodName) {
//   auto it = _methodMap.find(methodName);
//   if(it != _methodMap.end()) {
//     std::cout << "Remove method: " << it->first << std::endl;
//     _methodMap.erase(it);
//     return true;
//   }
//   std::cout << "Method not existed: " << it->first << std::endl;
//   return false;
// }


void RPCServer::start() {
  int epfd = epoll_create(5);
  epoll_event ev;
  ev.data.fd = _listenfd;
  ev.events = EPOLLIN | EPOLLET;
  epoll_ctl(epfd, EPOLL_CTL_ADD, _listenfd, &ev);
  set_nonblock(_listenfd);

  // using vector to dynamically handle ready events  
  std::vector<epoll_event> read_evs(20);  

  std::cout << "Server started.\n";
  while(1) {
    int ret = epoll_wait(epfd, &read_evs.front(), read_evs.size(), -1);
    if(ret < 0) {
      if(errno == EINTR)
        continue;
      std::cout << "error epoll_wait. errno: " << errno << "\n";
      break;
    }
    if(ret == int(read_evs.size()))
      read_evs.resize(read_evs.size() * 2);
    for(int i = 0; i < ret; i++) {
      int sockfd = read_evs[i].data.fd;
      if(read_evs[i].events & EPOLLRDHUP) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
        std::cout << "Try closing a connection...";
        // free a connection
        if(_connectionManager.close(sockfd) == false){
          std::cout << "Error connection not found.\n";
          close(sockfd);
        }
        else std::cout << "OK\n";
      }
      else if(read_evs[i].events & EPOLLIN)
        _inEvents(sockfd, epfd);
      // else if(read_evs[i].events & EPOLLOUT)
      //   _outEvents(sockfd, epfd);
    }
  }
  close(epfd);
  close(_listenfd);
}

void RPCServer::_inEvents(int fd, int epfd) {
  if(fd == _listenfd) {
    while(1){
      struct sockaddr_in caddr;
      socklen_t clen = sizeof(caddr);

      int connfd = accept(_listenfd, (struct sockaddr*)&caddr, &clen);
      if(connfd < 0) {
        if(errno != EAGAIN && errno == EINTR)
          std::cout << "Error accepting client, errno:" << errno << std::endl;
        break;
      }
      bool ret = _connectionManager.add(connfd, this);
      if(ret == false){
        std::cout << "Error creating connection, already existed.\n";
        close(connfd);
        continue;
      }
      std::cout << "New connection established.\n";
      
      // register epoll event
      struct epoll_event ev;
      ev.data.fd = connfd;
      ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
      epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
      set_nonblock(connfd);
    }
  }
  else{ // data from client
    RPCConnection *pc = _connectionManager.find(fd);
    if(pc == nullptr) {
      std::cout << "Error: receive data but connection not found.\n";
      exit(EXIT_FAILURE);
    }
    while(1) {
      int n = pc->recvXml();
      if(n < 0) {
        if(errno != EAGAIN && errno != EINTR)
          std::cout << "Error reading from " << fd << "errno: " << errno << std::endl;
        break;
      }
      if(n == 0) {  // receive a complete xml 

        // submit to thread pool to handle the work
        std::string s;
        while(1) {
          pc->getReqXml(s);
          if(!s.empty()) {
            _thpool.submit(th_work, pc, s);
            s.clear();
          }
          else
            break;
        }
      }
    }
  }
}

// void RPCServer::_outEvents(int fd, int epfd) {
//   RPCConnection *pc = _connectionManager.find(fd);
//   if(pc == nullptr){
//     std::cout << "Error: write data but no connection found.\n";
//     ::close(fd);
//   }
//   while(1) {
//     int n = pc->sendMessage();
//     if(n < 0) {
//       if(errno != EAGAIN && errno != EWOULDBLOCK)
//         std::cout << "Error writing to " << fd << "errno: " << errno << std::endl;
//       break;
//     }
//     if(n == 0) { // finish writing a buffer
//       epoll_event ev;
//       ev.data.fd = fd;
//       ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
//       epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
//       pc->resetBuffer(OUT_BUFFER);  // this will also wake up a worker waiting on outbuf to be available
//       break;
//     }
//   }
// }



/* ============= ConnectionManager ============= */

ConnectionManager::~ConnectionManager() {
  ConnectionManager::shutdown();
}

RPCConnection* ConnectionManager::find(int fd) {
  size_t buck_id = _getBuckId(fd);
  std::unique_lock<std::mutex> lock(_locks.at(buck_id));
  auto it = _recorders.at(buck_id).find(fd);
  if(it == _recorders.at(buck_id).end())
    return nullptr;
  return it->second;
}

bool ConnectionManager::add(int fd, RPCServer* ps) {
  size_t buck_id = _getBuckId(fd);
  std::pair<std::map<int, RPCConnection*>::iterator, bool> ret;
  std::unique_lock<std::mutex> lock(_locks.at(buck_id));

  RPCConnection * pc = new RPCConnection(fd, ps);
  ret = _recorders.at(buck_id).insert(std::pair<int, RPCConnection*>{fd, pc});
  if(ret.second != true)
    delete pc;
  return ret.second;
}

bool ConnectionManager::close(int fd) {
  size_t buck_id = _getBuckId(fd);
  std::unique_lock<std::mutex> lock(_locks.at(buck_id));

  auto it = _recorders.at(buck_id).find(fd);
  if(it == _recorders.at(buck_id).end()) {
    std::cout << "ConnectionManager error: fd to be closed not exist\n" ;
    return false;
  }
  delete it->second;  // 
  _recorders.at(buck_id).erase(it);
  return true;
}

void ConnectionManager::shutdown() {
  if(_bucket_sz == 0)
    return;
  for(size_t i = 0; i < _bucket_sz; i++) {
    std::unique_lock<std::mutex> lock(_locks.at(i));
    auto &submap = _recorders.at(i);
    for(auto it = submap.begin(); it != submap.end(); ) {
      delete it->second;
      it = submap.erase(it);
    }
  }
  _bucket_sz = 0;
  _locks.clear();
}


size_t ConnectionManager::_getBuckId(int val) const {
  if(val < 0)
    val = -val;
  const static double base = sqrt(2.0) - 1;
  double v = val * base;
  return static_cast<size_t>(_bucket_sz * (v - int(v)));
}


