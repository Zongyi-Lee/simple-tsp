
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

#include "../serialization/serialization.h"

namespace simprpc{

// support multi-thread sending request with same client instance
class RPCClient{
public:
  RPCClient(const char*ip, int port);
  ~RPCClient();

  bool execute(const std::string& funcName, const std::vector<XmlElement>& params, std::vector<XmlElement>& ret);

protected:
  bool _valid;    // whether this client instance is valid
  // TODO: In current implementation, we have to mege masterlock and respndLock into one
  // otherwise thread may find job not done and has master then fall asleep but other thread
  // has just got all respond and give up master. We cannot sleep on two locks!
  bool _hasMaster; // whether there is a thread handling io for this instance
  int _connfd;  // socket connection to remote server
  int _reqID;

  std::mutex _idLock; // a lock used for alocate request id;
  // std::mutex _masterLock;
  std::condition_variable _cv;


  struct RespondEvent{
    bool ready;
    std::mutex lock;
    std::condition_variable cv;
    std::string xml;

    RespondEvent(): ready(false) {} 
  };

  struct RequestEvent {
    const std::string *xml;
    size_t offset;

    RequestEvent(const std::string* p, size_t off): xml(p), offset(off) { }
  };

  std::mutex _reqLock;
  std::queue<RequestEvent> _reqQueue;
  std::string _recvBuffer;
  std::mutex _respLock;
  std::map<int, RespondEvent*> _respMap;

  // int buildConnection();
  int parseID(std::string& xml);
  void handleIO(int myid);    // myid represent the reqeust id that the working thread hold
  std::string genXml(const std::string& fname, const std::vector<XmlElement>& params, int id);
  bool genResult(const std::string& reamin_xml, std::vector<XmlElement>& ret); 

  void cleanShutdown();  // close connection
  void dirtyShutdown();  // directly clear all events and set _valid to be false
};


}