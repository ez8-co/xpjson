#include "gtest/gtest.h"
#include "../xpjson.hpp"

#ifdef _WIN32
	#include <Winbase.h>
#else
	#include <sys/time.h>
#endif

#define RUN_TIMES  100000

class TimeCost
{
public:
	TimeCost()
#ifdef _WIN32
	{
		QueryPerformanceFrequency(&_freq);
	}
#else
		: _lastStart(0)
		, _milliseconds(0)
	{
	}
#endif
	void start()
	{
#ifdef _WIN32
		QueryPerformanceCounter(&_lastStart);
#else
		timeval v;
		gettimeofday(&v, NULL);
		_lastStart = v.tv_sec * 1000 + v.tv_usec / 1000;
#endif
	}
	void end()
	{
#ifdef _WIN32
		LARGE_INTEGER _endTime;
		QueryPerformanceCounter(&_endTime);
		_milliseconds += (double)(_endTime.QuadPart-_lastStart.QuadPart)/(double)_freq.QuadPart;
#else
		timeval v;
		gettimeofday(&v, NULL);
		_milliseconds += v.tv_sec * 1000 + v.tv_usec / 1000 - _lastStart;
#endif
	}
	uint64_t timecost()
	{
		return _milliseconds;
	}
private:
#ifdef _WIN32
	LARGE_INTEGER _freq;
	LARGE_INTEGER _lastStart;
	uint64_t _milliseconds;
#else
	uint64_t _lastStart;
	uint64_t _milliseconds;
#endif
};

void create(JSON::Value& v)
{
	JSON::Object& media = v["media"].o();
	media["uri"].assign("http://javaone.com/keynote.mpg", false);
	media["title"].assign("Javaone Keynote", false);
	media["width"] = 640;
	media["height"] = 480;
	media["format"].assign("video/mpg4", false);
	media["duration"] = 1800000;
	media["size"] = 58982400;
	media["bitrate"] = 262144;
	media["player"].assign("JAVA", false);

	JSON::Array& persons = media["persons"].a();
	persons.resize(2);
	persons[0].assign("Bill Gates", false);
	persons[1].assign("Steve Jobs", false);

	JSON::Array& images = v["images"].a();
	images.resize(2);
	
	JSON::Object& image1 = images[0].o();
	image1["uri"].assign("http://javaone.com/keynote_large.jpg", false);
	image1["title"].assign("Javaone Keynote", false);
	image1["width"] = 1024;
	image1["height"] = 768;
	image1["size"].assign("LARGE", false);

	JSON::Object& image2 = images[1].o();
	image2["uri"].assign("http://javaone.com/keynote_small.jpg", false);
	image2["title"].assign("Javaone Keynote", false);
	image2["width"] = 320;
	image2["height"] = 240;
	image2["size"].assign("SMALL", false);
}

TEST(benchmark_xpjson, create)
{
	try {
		TimeCost tc;
		JSON::Value v;
		int times = RUN_TIMES;
		do {
			tc.start();
			create(v);
			tc.end();
			v.clear();
		}
		while (--times);
		printf("time cost: %" PRId64 "ms\n", tc.timecost());
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(benchmark_xpjson, write)
{
	try {
		TimeCost tc;
		JSON::Value v;
		create(v);
		int times = RUN_TIMES;
		do {
			string out;
			// reserve 600 bytes, reduce reallocation and copy cost of time
			out.reserve(600);
			tc.start();
			JSON::Writer::write(v, out);
			tc.end();
		}
		while (--times);
		printf("time cost: %" PRId64 "ms\n", tc.timecost());
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(benchmark_xpjson, read)
{
	try {
		TimeCost tc;
		int times = RUN_TIMES;
		string in("{\"images\":[{\"height\":768,\"size\":\"LARGE\",\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote_large.jpg\",\"width\":1024},{\"height\":240,\"size\":\"SMALL\",\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote_small.jpg\",\"width\":320}],\"media\":{\"bitrate\":262144,\"copyright\":null,\"duration\":1800000,\"format\":\"video\\/mpg4\",\"height\":480,\"persons\":[\"Bill Gates\",\"Steve Jobs\"],\"player\":\"JAVA\",\"size\":58982400,\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote.mpg\",\"width\":640}}");
		do {
			JSON::Value v;
			tc.start();
			JSON::Reader::read(v, in.c_str(), in.length());
			tc.end();
		}
		while (--times);
		printf("time cost: %" PRId64 "ms\n", tc.timecost());
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS ();
}