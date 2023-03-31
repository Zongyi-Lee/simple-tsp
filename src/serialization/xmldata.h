
/*
    This file describe some class that used for data serialization
*/

#pragma once

#include <string>
#include <vector>
#include <map>
#include <time.h>

#include "../common/types.h"

/* This class defines the basic data elements supported in xml */
namespace simprpc{
enum ElementType {
	TypeNone,
	TypeBoolean,
	TypeChar,
	TypeInt,
	TypeDouble,
	TypeTime,
	TypeString,
	TypeBinary,
	TypeStruct,
	TypeArray,
};

class XmlElement {
public:



	typedef std::vector<char> BinaryData;
	typedef std::map<std::string, XmlElement> StructData;
	typedef std::vector<XmlElement> DataArray;

	// constructors
	XmlElement(): _type(TypeNone) { _value.asBinary = 0; };
	explicit XmlElement(char ch): 				_type(TypeChar) { _value.asChar = ch; }
	explicit XmlElement(bool b): 					_type(TypeBoolean) { _value.asBool = b; }
	explicit XmlElement(int v): 					_type(TypeInt) {_value.asInt = v; }
	explicit XmlElement(double v):			 	_type(TypeDouble) {_value.asDouble = v;}
	explicit XmlElement(std::string s): 	_type(TypeString) { _value.asString = new std::string(s); }
	explicit XmlElement(struct tm& t):		_type(TypeTime) {_value.asTime = new struct tm(t); }	
	XmlElement(const char *p, size_t sz): _type(TypeBinary) {_value.asBinary = new BinaryData(p, p + sz);}

	~XmlElement() { clear(); }	
	void clear();	// free resources

	// Element type relevent functions
	bool istype(ElementType type) const { return _type == type; }
	void settype(ElementType type) { _type = type; }
	bool valid() const	{ return _type != TypeNone; }
	ElementType gettype() const { return _type; }

	// XML encode
	std::string encode();

	// XML decode
	bool decode(const std::string&, size_t*);

	std::ostream& write(std::ostream& os) const;
protected:
  ElementType _type;
	union {
		bool 							asBool;
		char 							asChar;
		int	 							asInt;
		double						asDouble;
		struct tm*				asTime;
		std::string*			asString;
		BinaryData*				asBinary;
		DataArray*				asArray;
		StructData*				asStruct;
	}	_value;


	std::string _bool2xml() const;
	std::string _char2xml() const;
	std::string _int2xml() const;
	std::string _double2xml() const;
	std::string _string2xml() const;
	std::string _binary2xml() const;
	std::string _struct2xml() const;
	std::string _array2xml() const;
	std::string _time2xml() const;

	bool _xml2bool(const std::string&, size_t*);
	bool _xml2char(const std::string&, size_t*);
	bool _xml2int(const std::string &, size_t*);
	bool _xml2double(const std::string&, size_t*);
	bool _xml2string(const std::string&, size_t*);
	bool _xml2binary(const std::string&, size_t*);
	bool _xml2struct(const std::string&, size_t*);
	bool _xml2array(const std::string&, size_t*);
	bool _xml2time(const std::string&, size_t*);


};


}