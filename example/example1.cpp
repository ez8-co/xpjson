#include "../xpjson.hpp"
#include <iostream>

int main()
{
	JSON::Value v;
	v["orca"]["personalities"]["name"] = "Zhang Wei";
	v["orca"]["personalities"]["sex"] = "male";
	v["orca"]["personalities"]["company"] = "ez8.co";
	v["orca"]["personalities"]["age"] = 28;
	v["orca"]["skills"][0] = "C++";
	v["orca"]["skills"][1] = "golang";
	v["orca"]["skills"][2] = "python";

	std::string out;
	v.write(out);

	std::cout << out << std::endl;
	return 0;
}