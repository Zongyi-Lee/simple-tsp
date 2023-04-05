

#pragma once
#include <list>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "../serialization/serialization.h"

namespace simprpc{

/*
  This class represent a connection between server and client, which is identified by the unique socket.
  A client may ask for multiple rpc call at a same time, all will be handled and responded by this class.

  RPCConnection class will receive and parse the message from client and parse them into function calls and
  parameters which have been registerd on server. After receiving 'XML_END' string, which indicates we have
  got the function name and parameters, the working thread will then execute parse() and execute(). Then 
  creating an respond xml message and sending back.

  Since We support client to raise rpc call in parallel, thus we must identify the coming message belongs to which
  call, and this is implemented by assigning an id for each rpc call, and this id is also carried  in the xml
  data.
*/
class RPCServer;

// enum{ OUT_BUFFER,IN_BUFFER};

class RPCConnection{
public:
  static const std::string XML_END;
  static const std::string XML_START;
  static const std::string ID_TAG;
  static const std::string ID_ETAG;
  static const std::string PARAMS_TAG;
  static const std::string PARAMS_ETAG;
  static const std::string FNAME_TAG;
  static const std::string FNAME_ETAG;


  static const std::string FAULT_TAG;
  static const std::string FAULT_ETAG;

  struct request{
    uint32_t id;
    std::string fun_name; // funciton name that client ask for
    std::vector<XmlElement> params;

    request(): id(0) {}
  };
  RPCConnection(int sockfd, RPCServer* ps): _connfd(sockfd), _sending(false), _p_server(ps) {}
  ~RPCConnection();

  void terminateConnection(); // close socket

  // void resetBuffer() { _inbuf.clear();}
  int recvXml(); 
  int sendXml(const std::string& xml);

  // const std::string& getBuffer() const { return _inbuf; }
   
  // parsing the xml and excute the cresponding command
  void execute(const std::string& xml);

  void getReqXml(std::string&s) { 
    _readyLock.lock();
    if(!_readyQueue.empty()) {
      s = std::move(_readyQueue.front());    
      _readyQueue.pop();
    }
    _readyLock.unlock();
  }
  
  // for debug only
#ifdef DEBUG
  void show_parsing() {
    request req;
    parse(_inbuf, req);
    std::cout << "ID: " << req.id << std::endl;
    std::cout << "Function name: " << req.fun_name << std::endl;
    std::cout << "Parameters:\n";
    for(auto &ele : req.params) 
      ele.write(std::cout);
  }
#endif



private:
  int _connfd; 
  std::string _inbuf;   // store the received data of a single request (encoded)

  std::mutex _readyLock;
  std::queue<std::string> _readyQueue;

  std::mutex _outlock;
  std::condition_variable _cv;   // if worker thread want to send result but find _outbuf is in use, wait on cr
  bool _sending;        // flag to identify whether some worker are sending response.

  const RPCServer* const _p_server;

  // parse a receved xml string into a function call request
  void parse(const std::string& xml, request& pr);  

  bool isValid() { return _connfd > -1; }

  void errorHandler(const char* msg, int errcode);
  void generateErrorResponse(int id);
};



}