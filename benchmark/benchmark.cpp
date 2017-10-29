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
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("uri")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("http://javaone.com/keynote.mpg")), false))));
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("title")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("Javaone Keynote")), false))));
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("width")), JSON_MOVE(JSON::Value(640))));
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("height")), JSON_MOVE(JSON::Value(480))));
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("format")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("video/mpg4")), false))));
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("duration")), JSON_MOVE(JSON::Value(1800000))));
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("size")), JSON_MOVE(JSON::Value(58982400))));
	media.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("bitrate")), JSON_MOVE(JSON::Value(262144))));

	JSON::Array& persons = media["persons"].a();
	persons.push_back(JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("Bill Gates")), false)));
	persons.push_back(JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("Steve Jobs")), false)));

	media["player"] = JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("JAVA")), false));
	media["copyright"];

	JSON::Array& images = v["images"].a();
	images.push_back(JSON_MOVE(JSON::Value(JSON_MOVE(JSON::Object()))));
	images.push_back(JSON_MOVE(JSON::Value(JSON_MOVE(JSON::Object()))));

	JSON::Object& image1 = images[0].o();
	image1.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("uri")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("http://javaone.com/keynote_large.jpg")), false))));
	image1.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("title")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("Javaone Keynote")), false))));
	image1.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("width")), JSON_MOVE(JSON::Value(1024))));
	image1.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("height")), JSON_MOVE(JSON::Value(768))));
	image1.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("size")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("LARGE"))))));

	JSON::Object& image2 = images[1].o();
	image2.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("uri")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("http://javaone.com/keynote_small.jpg")), false))));
	image2.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("title")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("Javaone Keynote")), false))));
	image2.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("width")), JSON_MOVE(JSON::Value(320))));
	image2.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("height")), JSON_MOVE(JSON::Value(240))));
	image2.insert(make_pair(JSON_MOVE(JSON_TSTRING(char)("size")), JSON_MOVE(JSON::Value(JSON_MOVE(JSON_TSTRING(char)("SMALL")), false))));
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
		printf("time cost: %"PRId64"ms\n", tc.timecost());
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
		printf("time cost: %"PRId64"ms\n", tc.timecost());
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
		printf("time cost: %"PRId64"ms\n", tc.timecost());
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