
#pragma once
#include <vector>
#include <string>

#include "../serialization/serialization.h"

namespace simprpc{

class RPCMethod{
public:
  RPCMethod(const std::string&s): _name(s) { }
  RPCMethod(const char* s): _name(s) { }
  virtual ~RPCMethod() = default;
  RPCMethod(const RPCMethod&) = delete;
  RPCMethod& operator=(const RPCMethod&) = delete;
  RPCMethod& operator=(const RPCMethod&&) = delete;

  virtual void execute(const std::vector<XmlElement>& params, std::vector<XmlElement>& result) = 0;
  std::string& getName(){ return _name; }
private:
  std::string _name;
};

}