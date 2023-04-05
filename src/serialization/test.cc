
#include <iostream>
#include <string>

#include "serialization.h"

using std::string;
using std::cout;
using std::endl;
using namespace simprpc;


void decodestr(const string& str, size_t pos, size_t *offset, size_t *sz) {
  try{
    string tag("<string>");
    size_t start = str.find(tag, pos);
    *sz = stoi(str.substr(start + tag.size() + 1));
    *offset = str.find('>', start + tag.size() + 1) + 1;
  } catch (...){
    cout << "decode failed.\n";
    cout << "Raw string:" << str << std::endl;
    exit(EXIT_FAILURE);
  }
}

void test_string(){
  // test 1
  string s1 = "hello simple-rpc";
  XmlElement ele1(s1);
  string msg = ele1.encode();
  cout << msg;
}

void test_xml_ele() {
  string s1 = "<element><char> acahr </char> <int> 122432fsdf </int> </element> and some";
  XmlElement ele(s1);
  std::string s = ele.encode();
  cout << s1 << endl;

  XmlElement dec;
  size_t offset = 0;
  dec.decode(s, &offset);
  cout << "Decode result:\n";
  dec.write(cout);

  // test int
  int v1 = 1234;
  int v2 = -123;
  XmlElement e3(v1);
  XmlElement e4(v2);
  std::string s3 = e3.encode();
  std::string s4 = e4.encode();
  cout << s3 << endl << s4 <<endl;
  cout << "Decode result:\n";
  XmlElement d3, d4;
  size_t t1 = 0, t2 = 0;
  d3.decode(s3, &t1);
  d4.decode(s4, &t2);
  d3.write(cout);
  d4.write(cout);
  
}


int main() {

  // test_string();
  test_xml_ele();
  return 0;
}