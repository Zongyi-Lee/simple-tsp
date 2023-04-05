
#include <iostream>
#include <map>
#include <cstring>
#include <cctype>

#include "xmlutil.h"
#include "xmldata.h"
#include "base64.h"
#include "../common/assert.h"



namespace simprpc{

static const char ELEMENT_TAG[]   = "<element>";
static const char ELEMENT_ETAG[]  = "</element>";

static const char BOOLEAN_TAG[]   = "<boolean>";
static const char BOOLEAN_ETAG[]  = "</boolean>";
static const char CHAR_TAG[]      = "<char>";
static const char CHAR_ETAG[]     = "</char>";
static const char DOUBLE_TAG[]    = "<double>";
static const char DOUBLE_ETAG[]   = "</double>";
static const char INT_TAG[]       = "<int>";
static const char I4_TAG[]        = "<i4>";
static const char I4_ETAG[]       = "</i4>";
static const char STRING_TAG[]    = "<string>";
static const char STRING_ETAG[]   = "</string>";
static const char TIME_TAG[]      = "</Time.iso8601>";
static const char TIME_ETAG[]     = "</Time.iso8601>";
static const char BASE64_TAG[]    = "<base64>";
static const char BASE64_ETAG[]   = "</base64>";

static const char ARRAY_TAG[]     = "<array>";
static const char ARRAY_ETAG[]    = "</array>";
static const char BINARY_TAG[]    = "<binary>";
static const char BINARY_ETAG[]   = "</binary>";

static const char STRUCT_TAG[]    = "<struct>";
static const char MEMBER_TAG[]    = "<member>";
static const char NAME_TAG[]      = "<name>";
static const char NAME_ETAG[]     = "</name>";
static const char MEMBER_ETAG[]   = "</member>";
static const char STRUCT_ETAG[]   = "</struct>";

// ================= CONSTRUCTORS ==================== //

XmlElement::XmlElement(const XmlElement& ele) {
  _type = ele.gettype();
  switch(_type) {
    case TypeString:
      _value.asString = new std::string(*(std::string*)ele.getdata());
      break;
    case TypeBinary:
      _value.asBinary = new BinaryData(*(BinaryData*)ele.getdata());
      break;
    case TypeTime:
      _value.asTime = new struct tm(*(struct tm*)ele.getdata());
      break;
    case TypeArray:
      _value.asArray = new DataArray(*(DataArray*)ele.getdata());
      break;
    case TypeStruct:
      _value.asStruct = new StructData(*(StructData*)ele.getdata());
      break;
    default:
      _value.asDouble = *(double*)ele.getdata();  // take 64 bit datacopy
      break;
  }
}

XmlElement::XmlElement(XmlElement&& ele) noexcept {
  _type = ele.gettype();
  switch(_type){
    case TypeBoolean:
    case TypeChar:
    case TypeInt:
    case TypeDouble:
      _value.asDouble = *(double*)ele.getdata();
      break;
    default:
      _value.asString = (std::string*)ele.getdata();
      break;
  }
  ele.clear();
}

XmlElement& XmlElement::operator=(const XmlElement& ele) {
  if(this != &ele){
    if(this->_type != TypeNone)
      this->free(); // free resources
    _type = ele.gettype();
    switch(_type) {
      case TypeString:
        _value.asString = new std::string(*(std::string*)ele.getdata());
        break;
      case TypeBinary:
        _value.asBinary = new BinaryData(*(BinaryData*)ele.getdata());
        break;
      case TypeTime:
        _value.asTime = new struct tm(*(struct tm*)ele.getdata());
        break;
      case TypeArray:
        _value.asArray = new DataArray(*(DataArray*)ele.getdata());
        break;
      case TypeStruct:
        _value.asStruct = new StructData(*(StructData*)ele.getdata());
        break;
      default:
        _value.asDouble = *(double*)ele.getdata();  // take 64 bit datacopy
        break;
    }
  }
  return *this;
}

XmlElement& XmlElement::operator=(XmlElement&& ele) noexcept {
  if(this != &ele) {
    if(this->_type != TypeNone)
      this->free();

    _type = ele.gettype();
    switch(_type){
      case TypeBoolean:
      case TypeChar:
      case TypeInt:
      case TypeDouble:
        _value.asDouble = *(double*)ele.getdata();
        break;
      default:
        _value.asString = (std::string*)ele.getdata();
        break;
    }
    ele.clear();
  }
  return *this;
}

void XmlElement::clear() {
  _type = TypeNone;
  _value.asStruct = nullptr;  // clear value field
}

void XmlElement::free() {
  switch(_type) {
    case TypeString:
      if(_value.asString != nullptr)
        delete _value.asString;
      break;
    case TypeBinary:
      if(_value.asBinary != nullptr)
        delete _value.asBinary;
      break;
    case TypeTime:
      if(_value.asTime != nullptr)
        delete _value.asTime;
      break;
    case TypeArray:
      if(_value.asArray != nullptr)
        delete _value.asArray;
      break;
    case TypeStruct:
      if(_value.asStruct != nullptr)
        delete _value.asStruct; 
    default:
      break;
  }
  _value.asStruct = nullptr;
}

/* ============================================================= */
// =================== for debug ================================//

static const std::map<ElementType, const char*> TYPE2NAME = {
    {TypeBoolean, "BOOL"},
    {TypeChar, "CHAR"},
    {TypeInt, "INT"},
    {TypeDouble, "DOUBLE"},
    {TypeString, "STRING"},
    {TypeBinary, "BINARY"},
    {TypeArray, "ARRAY"},
    {TypeStruct, "STRUCT"}
};

static const std::map<std::string, ElementType> TAG2TYPE = {
  {BOOLEAN_TAG, TypeBoolean},
  {CHAR_TAG, TypeChar},
  {I4_TAG, TypeInt},
  {DOUBLE_TAG, TypeDouble},
  {TIME_TAG, TypeTime},
  {STRING_TAG, TypeString},
  {BINARY_TAG, TypeBinary},
  {ARRAY_TAG, TypeArray},
  {STRUCT_TAG, TypeStruct},
};

static void printtype(ElementType type) {
  if(TYPE2NAME.find(type) == TYPE2NAME.end())
    printf("unexpected type\n");
  else
    printf("%s\n", TYPE2NAME.find(type)->second);
}
// ===============================================================//


std::string XmlElement::encode() const {
  switch(_type) {
    case TypeBoolean:
      return _bool2xml();
    case TypeChar:
      return _char2xml();
    case TypeInt:
      return _int2xml();
    case TypeDouble:
      return _double2xml();
    case TypeTime:
      return _time2xml();
    case TypeString:
      return _string2xml();
    case TypeBinary:
      return _binary2xml();
    case TypeArray:
      return _array2xml();
    case TypeStruct:
      return _struct2xml();
    default:
    {
      printf("unexpected encode type:");
      printtype(_type);
      return "";
    }
  }
}

bool XmlElement::decode(const std::string& xml, size_t *offset) {
  if(_type != TypeNone) {
    this->free(); // free resource
    this->clear();
  }
  if (!XmlUtil::nextTagIs(ELEMENT_TAG, xml, offset))
    return false;
  // fist skip ELEMENT_TAg
  XmlUtil::getNextTag(xml, offset);
  std::string tag = XmlUtil::getNextTag(xml, offset);
  ElementType type;
  if(TAG2TYPE.find(tag) != TAG2TYPE.end())
    type = TAG2TYPE.find(tag)->second;
  else
    return false;
  switch(type) {
    case TypeBoolean:
      return _xml2bool(xml, offset);
    case TypeChar:
      return _xml2char(xml, offset);
    case TypeInt:
      return _xml2int(xml, offset);
    case TypeDouble:
      return _xml2double(xml, offset);
    case TypeTime:
      return _xml2time(xml, offset);
    case TypeString:{
      return _xml2string(xml, offset);
    }
    case TypeBinary:
      return _xml2binary(xml, offset);
    case TypeArray:
      return _xml2array(xml, offset);
    case TypeStruct:
      return _xml2struct(xml, offset);
    default:
    {
      printf("unexpected encode type:");
      printtype(_type);
      return "";
    }
  }
}

std::string XmlElement::_bool2xml() const{
  SIMPRPC_ASSERT(istype(TypeBoolean));
  std::string xml(ELEMENT_TAG);
  xml += BOOLEAN_TAG;
  xml += _value.asBool ? "1" : "0";
  xml += BOOLEAN_ETAG;
  xml += ELEMENT_ETAG;
  return xml;
}

bool XmlElement::_xml2bool(const std::string &xml, size_t* offset) {
  if(*offset < xml.size() && (xml[*offset] == '0' || xml[*offset] == '1')) {
    *offset += 1;
    _type = TypeBoolean;
    _value.asBool = xml[*offset] == '1';
    XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
    return true;
  }
  return false;
}

std::string XmlElement::_char2xml() const {
  SIMPRPC_ASSERT(istype(TypeChar));
  std::string xml(ELEMENT_TAG);
  xml += CHAR_TAG;
  xml += _value.asChar;
  xml += CHAR_ETAG;
  xml += ELEMENT_ETAG;
  return xml;
}

bool XmlElement::_xml2char(const std::string &xml, size_t* offset) {
  if(*offset >= xml.size())
    return false;
  _type = TypeChar;
  _value.asChar = xml[*offset];
  *offset += 1;
  XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
  return true;
}

std::string XmlElement::_int2xml() const{
  SIMPRPC_ASSERT(istype(TypeInt));
  std::string xml(ELEMENT_TAG);
  char buf[32] = {0}; 
  memset(buf, 0, sizeof(buf));
  snprintf(buf, sizeof(buf) - 1, "%d", _value.asInt);
  xml += I4_TAG;
  xml += std::string(buf);
  xml += I4_ETAG;
  xml += ELEMENT_ETAG;
  return xml;
}

bool XmlElement::_xml2int(const std::string& xml, size_t* offset) {
  if(*offset >= xml.size())
    return false;
  
  const char *start = xml.c_str() + *offset;
  char *end;

  long val = strtol(start, &end, 10);
  if(end == start)
    return false;
  _type = TypeInt;
  _value.asInt = static_cast<int>(val);
  *offset += static_cast<size_t>(end - start);
  XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
  return true;
}

std::string XmlElement::_double2xml() const {
  SIMPRPC_ASSERT(istype(TypeDouble));
  char buf[80];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, sizeof(buf) - 1, "%f", _value.asDouble);
  std::string xml(ELEMENT_TAG);
  xml += DOUBLE_TAG;
  xml += std::string(buf);
  xml += DOUBLE_ETAG;
  xml += ELEMENT_ETAG;
  return xml;
}

bool XmlElement::_xml2double(const std::string& xml, size_t* offset) {
  const char *start = xml.c_str() + *offset;
  char *end;
  double val = strtod(start, &end);
  if(end == start)
    return false;
  _type = TypeDouble;
  _value.asDouble = val;
  *offset += static_cast<size_t>(end - start);
  XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
  return true;
}

std::string XmlElement::_string2xml() const {
  SIMPRPC_ASSERT(istype(TypeString));
  std::string xml(ELEMENT_TAG);
  xml += STRING_TAG;
  xml += XmlUtil::xmlEncode(*_value.asString);  // encode raw string to avoid some symbols that may confuse decoding
  xml += STRING_ETAG;
  xml += ELEMENT_ETAG;
  return xml;
}

bool XmlElement::_xml2string(const std::string& xml, size_t* offset){
  if(*offset>= xml.size())
    return false;
  size_t end = xml.find('<', *offset);
  if(end == std::string::npos)
    return false;
  _type = TypeString;
  _value.asString = new std::string(XmlUtil::xmlDecode(xml.substr(*offset, end - *offset)));
  *offset = end;
  XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
  return true;
}

std::string XmlElement::_time2xml() const {
  SIMPRPC_ASSERT(istype(TypeTime));
  struct tm* t = _value.asTime;
  char buf[20];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, sizeof(buf) - 1, "%4d%02d%02dT%02d:%02d:%02d", 
    t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  std::string xml = ELEMENT_TAG;
  xml += TIME_TAG;
  xml += std::string(buf);
  xml += TIME_ETAG;
  xml += ELEMENT_ETAG;
  return xml;
}

bool XmlElement::_xml2time(const std::string& xml, size_t* offset) {
  size_t end = xml.find('<', *offset);
  if(end == *offset)
    return false;
  std::string stime = xml.substr(*offset, end - *offset);
  struct tm t;
  if (sscanf(stime.c_str(),"%4d%2d%2dT%2d:%2d:%2d",
    &t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) != 6)
    return false;
  t.tm_isdst = -1;
  _type = TypeTime;
  _value.asTime = new struct tm(t);
  *offset = end;
  XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
  return true;
}

// for safety reasons, we need to translate binary data  into
// ascii text for before translating, search 'base64 encoding'
// for more details
// we do not need to specify the length of base64 data, since
// the chosen characters donot contail '<' or '>'
std::string XmlElement::_binary2xml() const {
  SIMPRPC_ASSERT(istype(TypeBinary));
  std::vector<char> base64data;
  int iostatus = 0;
  base64<char> encoder;
  std::back_insert_iterator<std::vector<char> > ins = std::back_inserter(base64data);
  encoder.put(_value.asBinary->begin(), _value.asBinary->end(), ins, iostatus, base64<>::crlf());

  std::string xml(ELEMENT_TAG);
  xml += BINARY_TAG;
  xml.append(base64data.begin(), base64data.end());
  xml += BINARY_ETAG;
  xml += ELEMENT_ETAG;
  return xml;
}

bool XmlElement::_xml2binary(const std::string& xml, size_t* offset) {
  size_t end = xml.find('<', *offset);
  if(end == *offset)
    return false;
  _type = TypeBinary;
  std::string str = xml.substr(*offset, end - *offset);
  _value.asBinary = new BinaryData();

  int iostatus = 0;
  base64<char> decoder;
  std::back_insert_iterator<std::vector<char> > ins = std::back_inserter(*_value.asBinary);
  decoder.get(str.begin(), str.end(), ins, iostatus);

  *offset = end;
  XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
  return true;
}

std::string XmlElement::_array2xml() const {
  SIMPRPC_ASSERT(istype(TypeArray));
  std::string xml(ELEMENT_TAG);
  xml += ARRAY_TAG;
  for(auto &ele : *_value.asArray) 
    xml += ele.encode();

  xml += ARRAY_ETAG;
  xml += ELEMENT_ETAG;
  return xml;  
}

bool XmlElement::_xml2array(const std::string& xml, size_t* offset) {
  _type = TypeArray;
  _value.asArray = new DataArray();
  XmlElement ele;
  while(ele.decode(xml, offset))
      _value.asArray->push_back(ele);
  
  XmlUtil::toTagEnd(xml, offset, ELEMENT_ETAG);
  return true;
}

std::string XmlElement::_struct2xml() const {
  printf("Not implemented.\n");
  SIMPRPC_ASSERT(0);
  return "";
}

bool XmlElement::_xml2struct(const std::string& xml, size_t* offset) {
  SIMPRPC_ASSERT(0);
  return false;
}

std::ostream& XmlElement::write(std::ostream& os) const {
  printtype(_type);
  switch(_type) {
    case TypeBoolean:
      os << _value.asBool << std::endl; break;
    case TypeChar:
      os << _value.asChar << std::endl; break;
    case TypeInt:
      os << _value.asInt << std::endl; break;
    case TypeDouble:
      os << _value.asDouble << std::endl; break;
    case TypeString:
      os << *_value.asString << std::endl; break;
    case TypeBinary:
    {
      int iostatus = 0;
      std::ostreambuf_iterator<char> out(os);
      base64<char> encoder;
      encoder.put(_value.asBinary->begin(), _value.asBinary->end(), out, iostatus, base64<>::crlf());
      break;
    }
    case TypeArray:
      break;
    case TypeStruct:
      break;
    default: break;
  }
  return os;
}
}