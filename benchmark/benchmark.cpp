#include "gtest/gtest.h"
#include "../xpjson.hpp"

#define RUN_TIMES  10000

JSON::Value v;
string in("{\"images\":[{\"height\":768,\"size\":\"LARGE\",\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote_large.jpg\",\"width\":1024},{\"height\":240,\"size\":\"SMALL\",\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote_small.jpg\",\"width\":320}],\"media\":{\"bitrate\":262144,\"copyright\":null,\"duration\":1800000,\"format\":\"video\\/mpg4\",\"height\":480,\"persons\":[\"Bill Gates\",\"Steve Jobs\"],\"player\":\"JAVA\",\"size\":58982400,\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote.mpg\",\"width\":640}}");

TEST(benchmark_xpjson, create)
{
	try {
		int times = RUN_TIMES;
		do {
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
		while (--times);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(benchmark_xpjson, write)
{
	try {
		int times = RUN_TIMES;
		do {
			string out;
			// reserve 600 bytes, reduce reallocation and copy cost of time
			out.reserve(600);
			JSON::Writer::write(v, out);
		}
		while (--times);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(benchmark_xpjson, read)
{
	try {
		int times = RUN_TIMES;
		do {
			JSON::Value v;
			JSON::Reader::read(v, in.c_str(), in.length());
		}
		while (--times);
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