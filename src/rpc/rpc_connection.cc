
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <mutex>

#include "rpc.h"
#include "../serialization/serialization.h"
#include "assert.h"

using namespace simprpc;

const std::string RPCConnection::XML_END("</XML>");
const std::string RPCConnection::XML_START("<XML>");

const std::string RPCConnection::ID_TAG("<id>");
const std::string RPCConnection::ID_ETAG("</id>");
const std::string RPCConnection::PARAMS_TAG("<params>");
const std::string RPCConnection::PARAMS_ETAG("</params>");
const std::string RPCConnection::FNAME_TAG("<fname>");  // function nmae tag
const std::string RPCConnection::FNAME_ETAG("</fname>");
const std::string RPCConnection::FAULT_TAG("<fault>");
const std::string RPCConnection::FAULT_ETAG("</fault>");

RPCConnection::~RPCConnection() {
  terminateConnection();  // close connfd
}

void RPCConnection::terminateConnection(){
  ::close(_connfd);
  _connfd = -1;
  _sending = false;
}


int RPCConnection::recvXml(){
  if(!isValid()){
    std::cout << "Error read message but connfd is invalid.\n";
    return -1;
  }

  const int BufferSize = 4096;
  char buf[BufferSize];
  memset(buf, 0, BufferSize);
  int len = read(_connfd, buf, BufferSize - 1);
  if(len < 0)
    return -1;
  if(len == 0) {
    std::cout << "Error read EOF but should be handled by epoll.\n";
    terminateConnection();
    return -1;
  }
  buf[len] = 0;

  // since maybe many request got received at the same time, we nned to 
  // seperate different xml
  _inbuf += buf;
  size_t pos = 0;
  size_t idx;
  while((idx = _inbuf.find(XML_END, pos)) != std::string::npos) {
    _readyLock.lock();
    _readyQueue.push(_inbuf.substr(pos, idx + XML_END.size() - pos));
    _readyLock.unlock();
    pos = idx + XML_END.size();
  }


  if(pos > 0) { // find an end
    _inbuf = _inbuf.substr(pos);
    return 0; // have already finish reading a xml request
  }
  return len;
}


int RPCConnection::sendXml(const std::string& xml) {
  const char *p = xml.c_str();
  size_t offset = 0;
  size_t len = xml.size();

  std::unique_lock<std::mutex> lock(_outlock);

  // if someone else is sending data, wait
  while(_sending == true)
    _cv.wait(lock);
  
  // set _sending to be true for this worker is about to send xml
  _sending = true;
  lock.unlock();  // avoid other threads spin on locking

  while(offset < len) {
    int n = write(_connfd, p + offset, len - offset);
    if(n < 0) {
      if(errno == EAGAIN || errno == EWOULDBLOCK)
        continue;
      std::cout << "Error in sending response.\n";
      break;
    }
    offset += n;
  }

  bool flag = offset == len;
  std::cout << "Finish sending response...";
  if(flag)
    std::cout << "OK\n";
  else
    std::cout << "Failed\n";

  lock.lock();
  _sending = false;
  _cv.notify_one();

  return flag ? 0 : -1;
}

void RPCConnection::errorHandler(const char* msg, int id){
  std::cout << msg << std::endl;
  generateErrorResponse(id);
}

void RPCConnection::generateErrorResponse(int id) {
  std::string errxml(XML_START);
  errxml += ID_TAG;
  XmlElement ele(id);
  errxml += ele.encode();
  errxml += ID_ETAG;
  errxml += FAULT_TAG + FAULT_ETAG + XML_END;

  sendXml(errxml);
}

void RPCConnection::parse(const std::string& xml, request& req) {
    size_t offset = 0;
    if(!XmlUtil::nextTagIs(XML_START.c_str(), xml, &offset)){
      errorHandler("Error invalid xml format: header not found.", req.id);
      return;
    }

    XmlUtil::getNextTag(xml, &offset);
    std::string tag = XmlUtil::getNextTag(xml, &offset);
    if(!(tag == ID_TAG)) {
      errorHandler("Invalid xml format: id tag not found.\n", req.id);
      return;
    }

    // get request id
    XmlElement id;
    id.decode(xml, &offset);
    XmlUtil::toTagEnd(xml, &offset, ID_ETAG.c_str());
    req.id = *((int*)id.getdata());

    if(!((tag = XmlUtil::getNextTag(xml, &offset)) == FNAME_TAG)) {
      errorHandler("Error invalid xml format: fname not found.", req.id);
      return;
    }

    // get request function name
    XmlElement func_name;
    func_name.decode(xml, &offset);
    XmlUtil::toTagEnd(xml, &offset, FNAME_ETAG.c_str());
    req.fun_name = *((std::string*)func_name.getdata());

    // get request parameters
    if((tag = XmlUtil::getNextTag(xml, &offset)) != PARAMS_TAG) {
      errorHandler("Invalid xml format: params tag not found.\n", req.id);
      return;
    }
      
    XmlElement ele;
    while(ele.decode(xml, &offset)) {
      req.params.emplace_back(std::move(ele));
    }
    if((tag = XmlUtil::getNextTag(xml, &offset)) != PARAMS_ETAG) {
      errorHandler("Invalid xml format: params end not found.\n", req.id);
      return;
    }

    if((tag = XmlUtil::getNextTag(xml, &offset)) != XML_END)
      errorHandler("Error invalid xml format: xml end not found.", req.id);

}

void RPCConnection::execute(const std::string& xml) {
  request req;
  parse(xml, req);
  RPCMethod * func = _p_server->getMethod(req.fun_name);
  if(func == nullptr) {
    std::cout << "Error: execute function not found. \"" << req.fun_name << "\"\n";
    errorHandler("", req.id);
    return;
  }

  std::vector<XmlElement> result;
  func->execute(req.params, result);

  // generate response xml data
  std::string response(XML_START);
  response += ID_TAG;
  XmlElement id((int)req.id);
  response += id.encode(); 
  response += ID_ETAG;

  response += PARAMS_TAG;
  for(auto &ele : result)
    response += ele.encode();
  response += PARAMS_ETAG;
  response += XML_END;

  // sneding result
  sendXml(response);

}






