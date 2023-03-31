
#pragma once
#include <string>

namespace simprpc {

class XmlUtil{

public:
  // return contents between <tag> and </tag>, then update offset field
  static std::string parseTag(const char* tag, const std::string& xml, size_t* offset) ;

  // return true if tag is found and update offset field
  static bool findTag(const char* tag, const std::string& xml, size_t* offset);

  // return next tag and updates offset to the char after the tag
  static std::string getNextTag(const std::string& xml, size_t* offset);

  // if the next tag is the given tag  
  static bool nextTagIs(const char* tag, const std::string& xml, size_t* offset);

  // convert raw text into encoded xml
  static std::string xmlEncode(const std::string& xml);

  // convert encoded xml into raw text
  static std::string xmlDecode(const std::string& encoded);


  static void toTagEnd(const std::string& xml, size_t* offset, const char* etag);
  
};

} // namespace simprpc