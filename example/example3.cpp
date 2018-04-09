#include "../xpjson.hpp"
#include <iostream>
#include <cassert>

int main()
{
	string in("{\"orca\":{\"personalities\":{\"age\":28,\"company\":\"ez8.co\",\"name\":\"Zhang Wei\",\"sex\":\"male\"},\"skills\":[\"C++\",\"golang\",\"python\"]}}");
	JSON::Value v;
	size_t ret = v.read(in);
	assert(ret == in.length());

	JSON::Object& orca = v["orca"].o();
	assert(orca.size() == 2);

	JSON::Object& personalities = orca["personalities"].o();
	JSON::Array& skills = orca["skills"].a();

	cout << "Personalities:" << endl;
	for(JSON::Object::const_iterator it=personalities.begin(); it!=personalities.end(); ++it) {
		string out;
		it->second.to_string(out);
		cout << it->first << " -> " << out << endl;
	}

	cout << endl << "Skills:" << endl;
	for(JSON::Array::const_iterator it=skills.begin(); it!=skills.end(); ++it) {
		cout << it->s() << endl;
	}

	return 0;
}