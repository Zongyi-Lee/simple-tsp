

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <unistd.h>
#include <mutex>
#include "rpc/rpcclient.h"


using namespace simprpc;

using std::string;
using std::vector;
using std::cout;
using std::endl;

void simple_test() {
  string fname("123");
  vector<XmlElement> params;
  params.emplace_back("hello from client");
  params.emplace_back(1234);
  params.emplace_back(3.1415926);
  
  RPCClient client("127.0.0.1", 12345);
  vector<XmlElement> ret;
  bool success = client.execute(fname, params, ret);
  if(success) {
    cout << "Request OK! ";
    cout << "Return elements:\n";
    for(auto &ele : ret)
      ele.write(cout);
    cout << "ALL PASSED!\n";
  } else{
    cout << "Request failed\n";
  }

}


std::mutex lock;
int count = 0;

void f1(RPCClient *client) {
  string fname("1231423");
  vector<XmlElement> params;
  params.emplace_back("hello from client");
  params.emplace_back(1234);
  params.emplace_back(3.1415926);

  vector<XmlElement> ret;
  bool success = client->execute(fname, params, ret);
  if(success) {
    cout << "F1 failed!\n";
  }
  else{
    lock.lock();
    count++;
    lock.unlock();
    cout << "F1 OK!\n";
  }
}

void f2(RPCClient *client) {
  string fname("hello");
  vector<XmlElement> params;
  vector<XmlElement> ret;

  bool success = client->execute(fname, params, ret);

  if(success){
    lock.lock();
    count++;
    lock.unlock();
    cout << "F2 OK!\n";
  }
  else
    cout << "F2 Wrong Answer!\n";
}

void f3(RPCClient *client) {
  string fname("hello");
  vector<XmlElement> params;
  params.emplace_back("hello from client");
  params.emplace_back(1234);
  params.emplace_back(3.1415926);
  
  
  vector<XmlElement> ret;
  bool success = client->execute(fname, params, ret);
  if(success){ 
    lock.lock();
    count++;
    cout << "F3 OK!\n";
    lock.unlock();
  }
  else 
    cout << "F3 Wrong Answer!\n";
}

void medium_test() {
  RPCClient client("127.0.0.1", 12345);
  void * funcs[3] = {(void*)f1, (void*)f2, (void*)f3};

  const int NUM_THDS = 20;
  for(int i = 0; i < NUM_THDS; i++) {
    std::thread th((void (*)(RPCClient*))funcs[i % 3], &client);
    th.detach();
  }
  sleep(5);
  lock.lock();
  if(count == NUM_THDS)
    cout << "ALL PASSED!\n";
  else
    cout << count << "passed, " << NUM_THDS - count << "failed.\n";
}

int main() {
  // simple_test();
  medium_test();
}




