#include "../xpjson.hpp"
#include <iostream>
#include <cassert>

int main()
{
	std::string in("{\"orca\":{\"personalities\":{\"age\":28,\"company\":\"ez8.co\",\"name\":\"Zhang Wei\",\"sex\":\"male\"},\"skills\":[\"C++\",\"golang\",\"python\"]}}");
	JSON::Value v;
	size_t ret = v.read(in);
	assert(ret == in.length());

	JSON::Object& orca = v["orca"].o();
	assert(orca.size() == 2);

	JSON::Object& personalities = orca["personalities"].o();
	JSON::Array& skills = orca["skills"].a();

	std::cout << "Personalities:" << std::endl;
	for(JSON::Object::const_iterator it=personalities.begin(); it!=personalities.end(); ++it) {
		std::string out;
		it->second.to_string(out);
		std::cout << it->first << " -> " << out << std::endl;
	}

	std::cout << std::endl << "Skills:" << std::endl;
	for(JSON::Array::const_iterator it=skills.begin(); it!=skills.end(); ++it) {
		std::cout << it->s() << std::endl;
	}

	return 0;
}