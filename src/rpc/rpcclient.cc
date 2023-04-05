
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>

#include "rpcclient.h"
#include "rpc_connection.h"

using namespace simprpc;


RPCClient::RPCClient(const char* ip, int port):_valid(true), _hasMaster(false), _connfd(-1), _reqID(0) {
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, ip, &addr.sin_addr);

  int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0 || ::connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
    _valid = false;
    std::cout << "RPCClient: create socket failed\n";
    return;
  }
  _connfd = sockfd;
  std::cout << "Client connection established!\n";
  
  // set non blocking
  if(fcntl(_connfd, F_SETFL, O_NONBLOCK) < 0) {
    std::cout << "RPCClient: set fd non-blocking failed.\n";
    close(_connfd);
    _valid = false;
    return;
  }
}

void RPCClient::cleanShutdown() {
  _valid = false;

  // handle response messages first, since if we close request queue
  // before clearing the response, we may face the risk having some
  // respond shall never get return since the request is cleared and
  // not send. And _valid = false makes no new respondEvent will be 
  // added into the map
  std::unique_lock<std::mutex> lresp(_respLock);
  while(1){
    if(_respMap.empty())
      break;
    // when IO thread receive a full xml respond, it always remove it from list 
    // then wakes up all threads waiting on that cv
    _respMap.begin()->second->cv.wait(lresp);
  }
  _reqLock.lock();
  while(!_reqQueue.empty())
    _reqQueue.pop();
  _reqLock.unlock();   
  
  if(_connfd != -1)
    ::close(_connfd);
  _hasMaster = false;
  _reqID = 0;
  _recvBuffer.clear();

}

void RPCClient::dirtyShutdown() {
  _valid = false;
  _respLock.lock();
  for(auto it = _respMap.begin(); it != _respMap.end(); it++)
    it->second->cv.notify_all();
  _respMap.clear();
  _respLock.unlock();

  _reqLock.lock();
  while(!_reqQueue.empty())
    _reqQueue.pop();
  _reqLock.unlock();

  ::close(_connfd);
  _hasMaster = false;
  _reqID = 0;
  _recvBuffer.clear();
}

RPCClient::~RPCClient() {
  RPCClient::cleanShutdown();
} 


bool RPCClient::execute(const std::string& funcName, const std::vector<XmlElement>& params,\
 std::vector<XmlElement>& ret)
{
  _idLock.lock();
  int id = _reqID++;
  _idLock.unlock();

  std::string xml = genXml(funcName, params, id);
  RequestEvent req(&xml, 0);
  // submit to request list
  _reqLock.lock();
  if(! _valid) {
    _reqLock.unlock();
    return false;
  }
  _reqQueue.push(req);
  _reqLock.unlock();

  // generate a RPC respond event
  RespondEvent resp;
  std::pair<std::map<int, RespondEvent*>::iterator, bool> r;

  // here we must hold lock before check the _valid field, in this
  // case, even if another thread change the _valid state after we
  // check, we can still safely insert into map, since shutdown code
  // need to hold _respLock to check _respMap status.
  _respLock.lock();
  if(! _valid) {
    _respLock.unlock();
    return false;
  }
  r = _respMap.insert(std::pair<int, RespondEvent*>{id, &resp});
  _respLock.unlock();
  
  if(r.second == false) 
    return false;

  /*
    The working procedure here is as following:
    1. check if response is ready
      + if so, check if has any master thread handling IO events
          ++ if has master, break
          ++ else if has anyother thread waiting, waking up let them be master
          ++ else if no master and no waiting thread, close remote connection or just leave it
      + else goto 2
    
    2. check if has master thread handling IO
      + if so, sleep
      + else set this thread to be master, goto handleIO function
  

   Danger: this code may hold two locks at same time
   The lock order is forced to be acquire respLock before masterLock

  */
  while(_valid) {
    std::unique_lock<std::mutex> lk(_respLock); 
    if(resp.ready) { // got respond
      if(!_hasMaster){
        if(!_respMap.empty())
          _respMap.begin()->second->cv.notify_all();  // ask other threads to be handle IO

        // TODO:could add some code for closing connection when no job to do for a long time 
        _recvBuffer.clear();  // since there is no one use this client now
      }
      break;
    }
    if(_hasMaster){ 
      resp.cv.wait(lk);
    }
    else{
      _hasMaster = true;
      lk.unlock();
      handleIO(id);
    }
  }

  // parse result and return
  bool ok = genResult(resp.xml, ret);
  return ok;
}

int RPCClient::parseID(std::string& xml) {
  size_t offset = xml.find(RPCConnection::ID_TAG);
  if(offset == std::string::npos)
    return -1;
  XmlUtil::getNextTag(xml, &offset);
  XmlElement id;
  id.decode(xml, &offset);
  XmlUtil::toTagEnd(xml, &offset, RPCConnection::ID_ETAG.c_str());
  xml = xml.substr(offset);
  return *(int*)(id.getdata());
}


/*
  1. 由于是非阻塞写，对于一次写不完的情况，需要设置一个指针指向可写的位置, req的数据结果需要修改
  2. 当接受xml完毕，解析到id后，先从_respMap中移除，然后判断是否是自己以及唤醒，注意要wakeu all
*/
void RPCClient::handleIO(int myid) {

  const int bufsz = 4096;
  char buf[bufsz];
  memset(buf, 0, sizeof(buf));

  while(1) {
    // Writing
    RequestEvent* p = nullptr;
    _reqLock.lock();
    if(!_reqQueue.empty()) {
      p = &_reqQueue.front();
    }
    _reqLock.unlock();
    if(p) {
      
      while(1) {
        int n = write(_connfd, p->xml->c_str(), p->xml->size() - p->offset);
        if(n < 0) {
          if(errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cout << " send later\n";
            continue;
          }
          std::cout << "RPCClient: sending error: " << errno << std::endl;
          RPCClient::dirtyShutdown();
          return;   
        }
        else {
          p->offset += n;
          if(p->offset == p->xml->size()) {
            _reqLock.lock();
            _reqQueue.pop();
            _reqLock.unlock();
            std::cout << "Finish sending an xml\n";
            break;
          }
        }
      } // end while loop
    } // end writing


    // Reading
    int n = read(_connfd, buf, bufsz - 1);
    if(n < 0) {
      if(errno == EAGAIN || errno == EWOULDBLOCK){
        continue;
      }
      std::cout << "RPCClient reading error: " << errno << std::endl;
      RPCClient::dirtyShutdown();
      return;
    }
    else if(n == 0){
      std::cout << "RPCClient: remote close connection!\n";
      RPCClient::dirtyShutdown();
      return; // no need to wait rest 
    }
    else{
      buf[n] = 0;
      _recvBuffer += buf;
      std::vector<std::string> ready_xmls;

      size_t pos = 0;
      size_t idx;
      while(pos < _recvBuffer.size() && (idx = _recvBuffer.find(RPCConnection::XML_END, pos)) != std::string::npos) {
        ready_xmls.push_back(_recvBuffer.substr(pos, idx + RPCConnection::XML_END.size() - pos));
        pos = idx + RPCConnection::XML_END.size();
      }

      if(pos > 0)   // must received complete messages
      { 
        bool find_my_expect = false;  // whether contain respond current thread waiting for
        _respLock.lock();
        for(auto &str : ready_xmls) {
          std::cout << "Get a complete response.\n";
          int id = parseID(str);

          auto it = _respMap.find(id);
          if(it == _respMap.end()) {  // garbage
            continue;
          } else{
            RespondEvent* pResp = it->second;
            _respMap.erase(it);
            pResp->ready = true;
            pResp->xml = std::move(str);
            if(id == myid) {
              find_my_expect = true;
              _hasMaster = false;
            }
            pResp->cv.notify_all();
          }
        }
        _respLock.unlock();
        _recvBuffer = _recvBuffer.substr(pos);
        if(find_my_expect)
          break;
      }
    }
  } // end while loop
}

bool RPCClient::genResult(const std::string& remain_xml, std::vector<XmlElement>& ret) {
  std::string tag;
  size_t offset = 0;
  tag = XmlUtil::getNextTag(remain_xml, &offset);
  
  // TODO: add falut code parsing function
  if(tag != RPCConnection::PARAMS_TAG)
    return false;
  
  XmlElement ele;
  while(ele.decode(remain_xml, &offset))
    ret.emplace_back(std::move(ele));
  return true;
}

std::string RPCClient::genXml(const std::string& fname, const std::vector<XmlElement>& params, int id) {
  std::string xml(RPCConnection::XML_START);
  xml += RPCConnection::ID_TAG;
  XmlElement ele(id);
  xml += ele.encode();
  xml += RPCConnection::ID_ETAG;

  xml += RPCConnection::FNAME_TAG;
  XmlElement funName(fname);
  xml += funName.encode();
  xml += RPCConnection::FNAME_ETAG;

  xml += RPCConnection::PARAMS_TAG;
  for(auto &param : params)
    xml += param.encode();
  xml += RPCConnection::PARAMS_ETAG;

  xml += RPCConnection::XML_END;
  return xml;
}
