
#include <iostream>
#include <vector>
#include <string>

#include "rpc/rpc.h"


using namespace simprpc;
using std::string;
using std::vector;
using std::cout;
using std::endl;


class HelloMethod : public RPCMethod {
public:
  HelloMethod(std::string& s): RPCMethod(s) { }
  HelloMethod(const char* s): RPCMethod(s) { }

  void execute(const std::vector<XmlElement> &params, std::vector<XmlElement> &results) override;
};

void HelloMethod::execute(const std::vector<XmlElement> &params, std::vector<XmlElement> &results) {
  // std::cout << "Hello Simple RPC!\n";
  // std::cout << "Received parameters:\n";
  // for(auto &ele : params)
    // ele.write(cout);

  results.push_back({"Ok from RPC Server!"});
  results.push_back({2222});
  results.push_back({3.1415926});
}

void start_server() {
  RPCServer server("127.0.0.1", 12345, 4);
  HelloMethod md("hello");
  server.registMethod(&md);
  server.start();

}

int main() {
  start_server();
}


