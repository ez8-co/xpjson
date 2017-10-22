#include "gtest/gtest.h"

#include "ut_xpjson.h"
#include "ut_xpjsonW.h"

#ifdef _WIN32
	#pragma comment(lib, "gtest.lib")
#endif

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS ();
}
