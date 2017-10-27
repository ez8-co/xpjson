#include "../xpjson.hpp"
#include <iostream>

int main()
{
	JSON::Value v;
	JSON::Object& orca = v["orca"].o();

	JSON::Object& personalities = orca["personalities"].o();
	personalities["name"] = "Zhang Wei";
	personalities["sex"] = "male";
	personalities["company"] = "ez8.co";

	JSON::Array& skills = orca["skills"].a();
	skills.reserve(3);
	// You should use resize or push_back for Array, coz it's vector.
	skills.resize(2);
	skills[0] = "C++";
	skills[1] = "golang";
	// Use JSON_MOVE to gain perf opt benefits under C++11 if possible.
	skills.push_back(JSON_MOVE(string("python")));

	string out;
	// reserve 100 bytes, reduce reallocation and copy cost of time
	out.reserve(100);
	v.write(out);

	cout << out << endl;
	return 0;
}