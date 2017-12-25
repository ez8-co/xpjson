#include "gtest/gtest.h"

#include "ut_xpjson.h"
#include "ut_xpjsonW.h"

#ifdef _WIN32
	#ifdef _DEBUG
		#pragma comment(lib, "gtestd.lib")
		#pragma comment(lib, "gtest_maind.lib")
	#else
		#pragma comment(lib, "gtest.lib")
		#pragma comment(lib, "gtest_main.lib")
	#endif
#endif

int main(int argc, char** argv)
{
	class LocaleChanger
	{
	public:
		LocaleChanger (void)
		{
#ifdef _WIN32
			std::locale::global (std::locale ("chs"));
#else
			std::locale::global (std::locale ("zh_CN.UTF-8"));
#endif
		}
		~LocaleChanger (void)
		{
			std::locale::global (std::locale ("C"));
		}
	} lc;
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS ();
}
