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
	personalities["age"] = 28;

	JSON::Array& skills = orca["skills"].a();
	skills.resize(2);
	skills[0] = "C++";
	skills[1] = "golang";
	// Use JSON_MOVE to gain perf opt benefits under C++11 if possible.
	skills.push_back(JSON_MOVE(std::string("python")));

	std::string out;
	// reserve 100 bytes, reduce reallocation and copy cost of time
	out.reserve(100);
	v.write(out);

	std::cout << out << std::endl;
	return 0;
}