
#include <cstring>
#include "xmlutil.h"

namespace simprpc{

std::string XmlUtil::parseTag(const char* tag, const std::string& xml, size_t* offset) {
  if(*offset >= xml.size())
    return "";
  size_t start = xml.find(tag, *offset);
  if(start == std::string::npos)
    return "";
  const char* etag = "</";
  start += strlen(tag) + 1;
  size_t end = xml.find(etag, start);
  if(end == std::string::npos)
    return "";
  *offset = end + strlen(tag) + 3; // </tag> length is strlen(tag) + 3
  return xml.substr(start, end - start);
}

bool XmlUtil::findTag(const char* tag, const std::string& xml, size_t* offset) {
  size_t start;
  if(*offset >= xml.size() || (start = xml.find(tag, *offset)) == std::string::npos)
    return false;
  *offset = start + strlen(tag);
  return true;
}

std::string XmlUtil::getNextTag(const std::string& xml, size_t* offset) {
  if(*offset >= xml.size())
    return "";
  size_t start = xml.find('<', *offset);
  if(start == std::string::npos)
    return "";
  size_t end = xml.find('>', start);
  if(end == std::string::npos)
    return "";
  *offset = end + 1;
  return xml.substr(start, end + 1 - start);
}

bool XmlUtil::nextTagIs(const char* tag, const std::string& xml, size_t* offset) {
  if(*offset >= xml.size())
    return false;
  size_t start = xml.find('<', *offset);
  if(start == std::string::npos)
    return false;
  size_t end = xml.find('>', start);
  if(end == std::string::npos)
    return false;
  return xml.substr(start, end - start + 1) == tag;
}

void XmlUtil::toTagEnd(const std::string& xml, size_t* offset, const char* etag) {
  std::string s;
  while((s = XmlUtil::getNextTag(xml, offset)) != "" && s != etag)
    ;
}


// XML Encoding/Decoding functions
static const char  AMP = '&';
static const char  rawEntity[] = { '<',   '>',   '&',    '\'',    '\"',    0 };
static const char* xmlEntity[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
static const int   xmlEntLen[] = { 3,     3,     4,      5,       5 };

std::string XmlUtil::xmlDecode(const std::string& encoded) {
  size_t amp = encoded.find(AMP);
  if(amp == std::string::npos)
    return encoded;
  std::string decoded(encoded, 0, amp);
  size_t len = encoded.size();
  decoded.reserve(len);

  const char* p = encoded.c_str();
  while(amp < len) {
    if(p[amp] == AMP && amp + 1 < len) {
      int idx;
      for(idx = 0; xmlEntity[idx] != 0; idx++) {
        if(strncmp(p + amp + 1, xmlEntity[idx], xmlEntLen[idx]) == 0) {
          decoded.push_back(rawEntity[idx]);
          amp += xmlEntLen[idx] + 1;
          break;
        }
      }
      if(xmlEntity[idx] == 0)
        decoded.push_back(p[amp++]);
    } else{
      decoded.push_back(p[amp++]);
    }
  }
  return decoded;
}

std::string XmlUtil::xmlEncode(const std::string& raw) {
  size_t pos = raw.find_first_of(rawEntity[0]);
  if(pos == std::string::npos)
    return raw;
  std::string encoded(raw, 0, pos);
  size_t len = raw.size();
  encoded.reserve(len);
  while(pos < len) {
    int idx;
    for(idx = 0; rawEntity[idx] != 0; idx++) {
      if(raw[pos] == rawEntity[idx]) {
        encoded.push_back(AMP);
        encoded += xmlEntity[idx];
        break;
      }
    }
    if(rawEntity[idx] == 0)
      encoded.push_back(raw[pos]);
    pos++;
  }
  return encoded;
} 


} // namespace simprpc