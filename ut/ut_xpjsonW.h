#define protected public

#include "gtest/gtest.h"

#include <iostream>
#include "../xpjson.hpp"
using namespace std;

TEST(ut_xpjsonW, write)
{
	try {
		JSON::ArrayW a;
		a.push_back(JSON::NIL);
		a.push_back(0x7FFFFFFF);
		a.push_back((int64_t)0xFFFFFFFFF);
		a.push_back(0.1);
		a.push_back(true);
		a.push_back(false);
		a.push_back(JSON_MOVE(JSON::ValueW(JSON_MOVE(wstring(L"test\"\\/\b\f\n\r\t")))));
		// indicate DO NOT escape
		a.push_back(JSON_MOVE(JSON::ValueW(JSON_MOVE(wstring(L"test")), false)));

		wstring out;
		// reserve 100 bytes, reduce reallocation and copy cost of time
		out.reserve(100);
		JSON::WriterW::write(a, out);
		ASSERT_TRUE(out == L"[null,2147483647,68719476735,0.1,true,false,\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\",\"test\"]");

		JSON::ObjectW o;
		o.insert(make_pair(JSON_MOVE(wstring(L"test")), JSON_MOVE(a)));

		out.clear();
		// reserve 100 bytes, reduce reallocation and copy cost of time
		out.reserve(100);
		JSON::WriterW::write(o, out);
		ASSERT_TRUE(out == L"{\"test\":[null,2147483647,68719476735,0.1,true,false,\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\",\"test\"]}");

		JSON::ValueW v(-1.797693134862315e+308);
		out.clear();
		v.write(out);
		ASSERT_TRUE(out == L"-1.797693134862315e+308");
		ASSERT_TRUE(fabs(v.f() + DBL_MAX) < 1e+293);

		v = -2.225073858507201e-308;
		out.clear();
		v.write(out);
		ASSERT_TRUE(out == L"-2.225073858507201e-308");

		v.read_number(out.c_str(), out.length());
		ASSERT_TRUE(fabs(v.f() + DBL_MIN) < 1e-293);

		v = L"/\\\"";
		out.clear();
		v.write(out);
		ASSERT_TRUE(out == L"\"\\/\\\\\\\"\"");
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, read)
{
	try {
		// normal cases
		// case 1 hybird test
		wstring in(L"[null,2147483647,68719476735,1.3e-12,true,false,\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\",\"test\"]");
		JSON::ValueW v;
		{
			JSON::ReaderW::read(v, in.c_str(), in.length());
			ASSERT_TRUE(v.type() == JSON::ARRAY);

			JSON::ArrayW& a = v.a();
			ASSERT_TRUE(a.size() == 8);
			ASSERT_TRUE(a[0].type() == JSON::NIL);

			ASSERT_TRUE(a[1].type() == JSON::INTEGER);
			ASSERT_TRUE(a[1].i() == 2147483647);

			ASSERT_TRUE(a[2].type() == JSON::INTEGER);
			ASSERT_TRUE(a[2].i() == 68719476735);

			ASSERT_TRUE(a[3].type() == JSON::FLOAT);
			ASSERT_TRUE(fabs(a[3].f() - 1.3e-12) < 1e-13);

			ASSERT_TRUE(a[4].type() == JSON::BOOLEAN);
			ASSERT_TRUE(a[4].b() == true);

			ASSERT_TRUE(a[5].type() == JSON::BOOLEAN);
			ASSERT_TRUE(a[5].b() == false);

			ASSERT_TRUE(a[6].type() == JSON::STRING);
			ASSERT_TRUE(a[6].s() == L"test\"\\/\b\f\n\r\t");

			ASSERT_TRUE(a[7].type() == JSON::STRING);
			ASSERT_TRUE(a[7].s()  == L"test");
		}

		// case 2 hybird test
		{
			in = L"  \r\n\t{\"ver\":123,\r\n \"o\":\tnull,\"flag\":true,\"data\":[[0,0.1,1.3e2]\r\n]\t  }";
			JSON::ReaderW::read(v, in.c_str(), in.length());
			ASSERT_TRUE(v.type() == JSON::OBJECT);

			JSON::ObjectW& o = v.o();
			ASSERT_TRUE(o[L"ver"].type() == JSON::INTEGER);
			ASSERT_TRUE(o[L"ver"].i() == 123);

			ASSERT_TRUE(o[L"o"].type() == JSON::NIL);

			ASSERT_TRUE(o[L"flag"].type() == JSON::BOOLEAN);
			ASSERT_TRUE(o[L"flag"].b() == true);

			ASSERT_TRUE(o[L"data"].type() == JSON::ARRAY);
			JSON::ArrayW& a1 = o[L"data"].a();
			ASSERT_TRUE(a1.size() == 1);

			ASSERT_TRUE(a1[0].type() == JSON::ARRAY);

			JSON::ArrayW& a11 = a1[0].a();

			ASSERT_TRUE(a11[0].type() == JSON::INTEGER);
			ASSERT_TRUE(a11[0].i() == 0);

			ASSERT_TRUE(a11[1].type() == JSON::FLOAT);
			ASSERT_TRUE(a11[1].f() == 0.1);

			ASSERT_TRUE(a11[2].type() == JSON::FLOAT);
			ASSERT_TRUE(fabs(a11[2].f() - 1.3e2) < 1);
		}

		// case 3 empty object
		{
			in = L"{}";
			JSON::ReaderW::read(v, in.c_str(), in.length());
			ASSERT_TRUE(v.type() == JSON::OBJECT);

			JSON::ObjectW& o = v.o();
			ASSERT_TRUE(o.empty());

			in = L"[]";
			JSON::ReaderW::read(v, in.c_str(), in.length());
			ASSERT_TRUE(v.type() == JSON::ARRAY);

			JSON::ArrayW& a1 = v.a();
			ASSERT_TRUE(a1.empty());
		}

		// case 4 nested array
		{
			in = L"[[[[]]]]";
			JSON::ReaderW::read(v, in.c_str(), in.length());
			ASSERT_TRUE(v.type() == JSON::ARRAY);

			JSON::ArrayW& a1 = v.a();
			ASSERT_TRUE(a1.size() == 1);
			ASSERT_TRUE(a1[0].type() == JSON::ARRAY);

			JSON::ArrayW& a11 = a1[0].a();
			ASSERT_TRUE(a11.size() == 1);
			ASSERT_TRUE(a11[0].type() == JSON::ARRAY);

			JSON::ArrayW& a111 = a11[0].a();
			ASSERT_TRUE(a111.size() == 1);
			ASSERT_TRUE(a111[0].type() == JSON::ARRAY);

			JSON::ArrayW& a1111 = a111[0].a();
			ASSERT_TRUE(a1111.empty());

			in = L"[{}]";
			JSON::ReaderW::read(v, in.c_str(), in.length());
		}

		// case 5 escape
		{
			in = L"{\"a\":\"b\",\"key\":\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\"}";
			JSON::ReaderW::read(v, in.c_str(), in.length());
			ASSERT_TRUE(v.type() == JSON::OBJECT);

			JSON::ObjectW& o1 = v.o();
			ASSERT_TRUE(o1.size() == 2);

			ASSERT_TRUE(o1[L"a"].type() == JSON::STRING);
			ASSERT_TRUE(o1[L"a"].s() == L"b");

			ASSERT_TRUE(o1[L"key"].type() == JSON::STRING);
			ASSERT_TRUE(o1[L"key"].s() == L"test\"\\/\b\f\n\r\t");

			wstring out;
			JSON::WriterW::write(o1, out);
			ASSERT_TRUE(in == out);
		}

		// case 6 decode length
		{
			in = L"{}";
			ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == 2);

			in = L"{}  testestestest";
			ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == 2);

			in = L"{\"a\":0}  asdasfasdfgag";
			ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == 7);
		}

		// case 7 whitespace characters
		{
			in = L"\r\n\t { \"a\" \r:\n \"b\" \n\r\t}";
			ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == in.length());
		}

		// case 8 duplicate key
		{
			in = L"{\"a\":0,\"a\":1}";
			ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == in.length());
			ASSERT_TRUE(v.type() == JSON::OBJECT);

			JSON::ObjectW& o1 = v.o();
			ASSERT_TRUE(o1.size() == 1);

			ASSERT_TRUE(o1[L"a"].type() == JSON::INTEGER);
			ASSERT_TRUE(o1[L"a"].i() == 1);
		}

		// case 9 nested object
		{
			in = L"{\"a\":{\"a\":1}}";
			ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == in.length());
			in = L"[{\"a\":1}]";
			ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == in.length());
		}

		// case 10 escaped key
		in = L"{\"aa\\b\":\"b\"}";
		ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == in.length());
		ASSERT_TRUE(v[L"aa\b"].s() == L"b");

		// case 11 escaped key
		in = L"{\"aa\\b\\\"\":\"b\"}";
		ASSERT_TRUE(JSON::ReaderW::read(v, in.c_str(), in.length()) == in.length());
		ASSERT_TRUE(v[L"aa\b\""].s() == L"b");

		// exception cases
		// case 1 bracket not match
		in = L"{\"a\":\"b\"";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"[\"a\",\"b\"";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 2 no start({ or [)
		in = L"\"a\":\"b\"";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"abcd";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 3 comma before right bracket
#if __XPJSON_SUPPORT_DANGLING_COMMA__
		in = L"{\"a\":\"b\",}";
		EXPECT_NO_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()));
		in = L"{\"a\":]}";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"[\"a\",\"b\",]";
		EXPECT_NO_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()));
#else
		in = L"{\"a\":\"b\",}";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"{\"a\":]}";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"[\"a\",\"b\",]";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
#endif 

		// case 4 whitespace characters/double quotation marks/right brace after left brace
		in = L"{ \r\n\taaa";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 5 colon after key
		in = L"{\"a\" \n\r\t a}";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 6 uncompleted escaped key
		in = L"{\"aa\\b";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 7 invalid value characters
		in = L"{\"a\": a}";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"{\"a\": 1, \"b\":a}";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"[a]";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"[123,a]";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 8 not comma/whitespace characters/right square bracket/right brace after value
		in = L"{\"a\":\"b\"a";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);
		in = L"[\"a\",\"b\"a";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 9 end with backslash in key
		in = L"{\"a\\";
		EXPECT_THROW(JSON::ReaderW::read(v, in.c_str(), in.length()), std::logic_error);

		// case 10 bad cow case when hit sso(1/5 bytes if sizof(wchar_t) is 2; 1 byte if sizof(wchar_t) is 4)
		JSON::ValueW v2(L"y");
		v = v2; // incorrect treat as cow when cow(sso_len highest bit) is true
		*const_cast<wchar_t*>(v2.c_str()) = 'z';
		ASSERT_TRUE(v.s() == L"y");
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, read_write)
{
	try {
		wstring in(L"[\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\",\"test\"]");
		JSON::ValueW v;
		{
			JSON::ReaderW::read(v, in.c_str(), in.length());
			ASSERT_TRUE(v.type() == JSON::ARRAY);

			wstring out;
			v.write(out);
			ASSERT_EQ(in, out);
		}
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, ctor)
{
	try {
		JSON::ValueW nil;
		ASSERT_TRUE(nil.type() == JSON::NIL);

		JSON::ValueW int_v(123);
		ASSERT_TRUE(int_v.type() == JSON::INTEGER);

		JSON::ValueW int64_t_v(int64_t(2147483647));
		ASSERT_TRUE(int64_t_v.type() == JSON::INTEGER);

		JSON::ValueW float_v(float(2.14));
		ASSERT_TRUE(float_v.type() == JSON::FLOAT);

		JSON::ValueW double_v(double(3.1415926));
		ASSERT_TRUE(double_v.type() == JSON::FLOAT);

		JSON::ValueW bool_v(true);
		ASSERT_TRUE(bool_v.type() == JSON::BOOLEAN);

		JSON::ValueW cstr_v(L"test");
		ASSERT_TRUE(cstr_v.type() == JSON::STRING);

		JSON::ValueW cstr_l_v(L"test", size_t(4));
		ASSERT_TRUE(cstr_l_v.type() == JSON::STRING);

		JSON::ValueW wstring_v(wstring(L"test"));
		ASSERT_TRUE(wstring_v.type() == JSON::STRING);

		JSON::ValueW object_v(JSON::OBJECT);
		ASSERT_TRUE(object_v.type() == JSON::OBJECT);

		JSON::ValueW array_v(JSON::ARRAY);
		ASSERT_TRUE(array_v.type() == JSON::ARRAY);

		JSON::ValueW copy_ctor_v(array_v);
		ASSERT_TRUE(copy_ctor_v.type() == JSON::ARRAY);

#ifdef __XPJSON_SUPPORT_MOVE__
		JSON::ValueW move_ctor_v(JSON_MOVE(copy_ctor_v));
		ASSERT_TRUE(move_ctor_v.type() == JSON::ARRAY);
		ASSERT_TRUE(copy_ctor_v.type() == JSON::NIL);

		wstring s(L"test");
		JSON::ValueW move_wstring_ctor_v(JSON_MOVE(s));
		ASSERT_TRUE(move_wstring_ctor_v.type() == JSON::STRING);
		ASSERT_TRUE(s.empty());

		JSON::ValueW move_object_ctor_v(JSON_MOVE(object_v));
		ASSERT_TRUE(move_object_ctor_v.type() == JSON::OBJECT);
		ASSERT_TRUE(object_v.type() == JSON::NIL);

		JSON::ValueW move_array_ctor_v(JSON_MOVE(array_v));
		ASSERT_TRUE(move_array_ctor_v.type() == JSON::ARRAY);
		ASSERT_TRUE(array_v.type() == JSON::NIL);
#endif
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, assign)
{
	try {
		JSON::ValueW v;
		ASSERT_TRUE(v.type() == JSON::NIL);

		JSON::ValueW int_v(123);
		ASSERT_TRUE(int_v.type() == JSON::INTEGER);

		v.assign(int_v);
		ASSERT_TRUE(v.type() == JSON::INTEGER);
		ASSERT_TRUE(v.i() == int_v.i());

		float f = 1.23f;
		v.assign(f);
		ASSERT_TRUE(v.type() == JSON::FLOAT);
		ASSERT_TRUE(fabs(v.f() - f) < 1e-3);

		int64_t i64 = 123456;
		v.assign(i64);
		ASSERT_TRUE(v.type() == JSON::INTEGER);
		ASSERT_TRUE(v.i() == i64);

		double d = 3.14;
		v.assign(d);
		ASSERT_TRUE(v.type() == JSON::FLOAT);
		ASSERT_TRUE(fabs(v.f() - d) < 1e-3);

		bool b = true;
		v.assign(b);
		ASSERT_TRUE(v.type() == JSON::BOOLEAN);
		ASSERT_TRUE(v.b() == b);

		v.assign(L"test");
		ASSERT_TRUE(v.type() == JSON::STRING);
		ASSERT_TRUE(v.s() == L"test");

		JSON::ObjectW o;
		v.assign(o);
		ASSERT_TRUE(v.type() == JSON::OBJECT);

		v.assign(L"test", size_t(4));
		ASSERT_TRUE(v.type() == JSON::STRING);
		ASSERT_TRUE(v.s() == L"test");

		JSON::ArrayW a;
		v.assign(a);
		ASSERT_TRUE(v.type() == JSON::ARRAY);

		wstring s(L"test");
		v.assign(s);
		ASSERT_TRUE(v.type() == JSON::STRING);
		ASSERT_TRUE(v.s() == s);

#ifdef __XPJSON_SUPPORT_MOVE__
		JSON::ValueW v_tmp(1);
		v.assign(JSON_MOVE(v_tmp));
		ASSERT_TRUE(v_tmp.type() == JSON::NIL);
		ASSERT_TRUE(v.type() == JSON::INTEGER);

		v.assign(JSON_MOVE(s));
		ASSERT_TRUE(v.type() == JSON::STRING);
		ASSERT_TRUE(s.empty());

		o[L"a"] = 1;
		v.assign(JSON_MOVE(o));
		ASSERT_TRUE(v.type() == JSON::OBJECT);
		ASSERT_TRUE(o.empty());
		ASSERT_TRUE(!v.o().empty());
		ASSERT_TRUE(v.o()[L"a"].i() == 1);

		a.push_back(2);
		v.assign(JSON_MOVE(a));
		ASSERT_TRUE(v.type() == JSON::ARRAY);
		ASSERT_TRUE(a.empty());
		ASSERT_TRUE(!v.a().empty());
		ASSERT_TRUE(v.a()[0].i() == 2);
#endif
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, assignment)
{
	try {
		JSON::ValueW v;
		ASSERT_TRUE(v.type() == JSON::NIL);

		JSON::ValueW int_v(123);
		ASSERT_TRUE(int_v.type() == JSON::INTEGER);

		v = int_v;
		ASSERT_TRUE(v.type() == JSON::INTEGER);
		ASSERT_TRUE(v.i() == int_v.i());

		float f = 1.23f;
		v = f;
		ASSERT_TRUE(v.type() == JSON::FLOAT);
		ASSERT_TRUE(fabs(v.f() - f) < 1e-3);

		int64_t i64 = 123456;
		v = i64;
		ASSERT_TRUE(v.type() == JSON::INTEGER);
		ASSERT_TRUE(v.i() == i64);

		double d = 3.14;
		v = d;
		ASSERT_TRUE(v.type() == JSON::FLOAT);
		ASSERT_TRUE(fabs(v.f() - d) < 1e-3);

		bool b = true;
		v = b;
		ASSERT_TRUE(v.type() == JSON::BOOLEAN);
		ASSERT_TRUE(v.b() == b);

		v = L"test";
		ASSERT_TRUE(v.type() == JSON::STRING);
		ASSERT_TRUE(v.s() == L"test");

		JSON::ObjectW o;
		v = o;
		ASSERT_TRUE(v.type() == JSON::OBJECT);

		JSON::ArrayW a;
		v = a;
		ASSERT_TRUE(v.type() == JSON::ARRAY);

		wstring s(L"test");
		v = s;
		ASSERT_TRUE(v.type() == JSON::STRING);
		ASSERT_TRUE(v.s() == s);

#ifdef __XPJSON_SUPPORT_MOVE__
		JSON::ValueW v_tmp(1);
		v = JSON_MOVE(v_tmp);
		ASSERT_TRUE(v_tmp.type() == JSON::NIL);
		ASSERT_TRUE(v.type() == JSON::INTEGER);

		v = JSON_MOVE(s);
		ASSERT_TRUE(v.type() == JSON::STRING);
		ASSERT_TRUE(s.empty());

		o[L"a"] = 1;
		v = JSON_MOVE(o);
		ASSERT_TRUE(v.type() == JSON::OBJECT);
		ASSERT_TRUE(o.empty());
		ASSERT_TRUE(!v.o().empty());
		ASSERT_TRUE(v.o()[L"a"].i() == 1);

		a.push_back(2);
		v = JSON_MOVE(a);
		ASSERT_TRUE(v.type() == JSON::ARRAY);
		ASSERT_TRUE(a.empty());
		ASSERT_TRUE(!v.a().empty());
		ASSERT_TRUE(v.a()[0].i() == 2);
#endif
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, bracket_operator)
{
	try {
		JSON::ValueW o(JSON::OBJECT);
		ASSERT_TRUE(o.type() == JSON::OBJECT);

		wstring elem(L"test");
		o[L"a"] = elem;
		ASSERT_TRUE(o[L"a"].s() == elem);
		ASSERT_TRUE(o[elem].type() == JSON::NIL);

		o[elem] = elem;
		ASSERT_TRUE(o[elem].s() == elem);

		JSON::ValueW nil;
		nil[L"test"] = 0;
		ASSERT_TRUE(nil.type() == JSON::OBJECT);

		JSON::ValueW nil1;
		nil1[elem] = 0;
		ASSERT_TRUE(nil1.type() == JSON::OBJECT);

		JSON::ValueW a(JSON::ARRAY);
		a.a().push_back(1);
		a.a().push_back(2);
		a.a().push_back(3);
		for(unsigned char i = 0; i < a.a().size(); ++i) {
			ASSERT_TRUE(a[i] == i + 1);
		}
		for(size_t i = 0; i < a.a().size(); ++i) {
			a[i] = i;
		}
		for(uint64_t i = 0; i < a.a().size(); ++i) {
			ASSERT_TRUE(a[i] == i);
		}

		// auto expand test
		a[4] = 4;
		a[5] = 5;
		ASSERT_TRUE(a[3].type() == JSON::NIL);
		for(uint64_t i = 4; i < a.a().size(); ++i) {
			ASSERT_TRUE(a[i] == i);
		}

		// underflow test
		EXPECT_THROW(a[-1], std::underflow_error);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, compare_function)
{
	try {
		wstring str(L"test");
		wstring str1(L"test1");

		JSON::ArrayW a;
		a.push_back(0);
		a.push_back(0.1);
		JSON::ArrayW a1;
		JSON::ArrayW a2;
		a2.push_back(0);
		a2.push_back(0.2);

		JSON::ObjectW o;
		o[L"a"] = L"b";
		o[L"b"] = L"c";
		JSON::ObjectW o1;
		JSON::ObjectW o2;
		o2[L"a"] = L"b";
		o2[L"b"] = L"d";
		JSON::ObjectW o3;
		o3[L"c"] = L"b";
		o3[L"b"] = L"c";

		JSON::ValueW nil;
		JSON::ValueW int_v(0);
		JSON::ValueW float_v(0.1);
		JSON::ValueW bool_v(true);
		JSON::ValueW str_v(str);
		JSON::ValueW arr_v(a);
		JSON::ValueW obj_v(o);

		/* int */
		// type
		ASSERT_FALSE(nil == 1);
		ASSERT_FALSE(1 == nil);
		ASSERT_TRUE(nil != 1);
		ASSERT_TRUE(1 != nil);
		ASSERT_FALSE(nil == int_v);
		ASSERT_TRUE(nil != int_v);
		ASSERT_FALSE(int_v == float_v);
		ASSERT_TRUE(int_v != float_v);
		ASSERT_FALSE(int_v == bool_v);
		ASSERT_TRUE(int_v != bool_v);
		ASSERT_FALSE(int_v == str_v);
		ASSERT_TRUE(int_v != str_v);
		ASSERT_FALSE(int_v == arr_v);
		ASSERT_TRUE(int_v != arr_v);
		ASSERT_FALSE(int_v == obj_v);
		ASSERT_TRUE(int_v != obj_v);

		// value
		ASSERT_TRUE(int_v == 0);
		ASSERT_TRUE(0 == int_v);
		ASSERT_TRUE(int_v == JSON::ValueW(0));
		ASSERT_TRUE(int_v != 1);
		ASSERT_TRUE(1 != int_v);
		ASSERT_TRUE(int_v != JSON::ValueW(1));

		/* float_v */
		// type
		ASSERT_FALSE(nil == 0.1);
		ASSERT_FALSE(0.1 == nil);
		ASSERT_TRUE(nil != 0.1);
		ASSERT_TRUE(0.1 != nil);
		ASSERT_FALSE(nil == float_v);
		ASSERT_TRUE(nil != float_v);
		ASSERT_FALSE(float_v == bool_v);
		ASSERT_TRUE(float_v != bool_v);
		ASSERT_FALSE(float_v == str_v);
		ASSERT_TRUE(float_v != str_v);
		ASSERT_FALSE(float_v == arr_v);
		ASSERT_TRUE(float_v != arr_v);
		ASSERT_FALSE(float_v == obj_v);
		ASSERT_TRUE(float_v != obj_v);

		// value
		ASSERT_TRUE(float_v == 0.1);
		ASSERT_TRUE(0.1 == float_v);
		ASSERT_TRUE(float_v == JSON::ValueW(0.1));
		ASSERT_TRUE(float_v != 0.2);
		ASSERT_TRUE(0.2 != float_v);
		ASSERT_TRUE(float_v != JSON::ValueW(0.2));

		/* bool_v */
		// type
		ASSERT_FALSE(nil == true);
		ASSERT_FALSE(true == nil);
		ASSERT_TRUE(nil != true);
		ASSERT_TRUE(true != nil);
		ASSERT_FALSE(nil == bool_v);
		ASSERT_TRUE(nil != bool_v);
		ASSERT_FALSE(bool_v == str_v);
		ASSERT_TRUE(bool_v != str_v);
		ASSERT_FALSE(bool_v == arr_v);
		ASSERT_TRUE(bool_v != arr_v);
		ASSERT_FALSE(bool_v == obj_v);
		ASSERT_TRUE(bool_v != obj_v);

		// value
		ASSERT_TRUE(bool_v == true);
		ASSERT_TRUE(true == bool_v);
		ASSERT_TRUE(bool_v == JSON::ValueW(true));
		ASSERT_TRUE(bool_v != false);
		ASSERT_TRUE(false != bool_v);
		ASSERT_TRUE(bool_v != JSON::ValueW(false));

		/* string */
		// type
		ASSERT_FALSE(nil == str);
		ASSERT_FALSE(str == nil);
		ASSERT_TRUE(nil != str);
		ASSERT_TRUE(str != nil);
		ASSERT_FALSE(nil == str_v);
		ASSERT_TRUE(nil != str_v);
		ASSERT_FALSE(bool_v == arr_v);
		ASSERT_TRUE(bool_v != arr_v);
		ASSERT_FALSE(bool_v == obj_v);
		ASSERT_TRUE(bool_v != obj_v);

		// value
		ASSERT_TRUE(str_v == str);
		ASSERT_TRUE(str == str_v);
		ASSERT_TRUE(str_v == JSON::ValueW(str));
		ASSERT_TRUE(str_v != str1);
		ASSERT_TRUE(str1 != str_v);
		ASSERT_TRUE(str_v != JSON::ValueW(str1));

		ASSERT_TRUE(str_v == L"test");
		ASSERT_TRUE(L"test" == str_v);
		ASSERT_TRUE(str_v == JSON::ValueW(str));
		ASSERT_TRUE(str_v != L"test1");
		ASSERT_TRUE(L"test1" != str_v);
		ASSERT_TRUE(str_v != JSON::ValueW(str1));

		/* array */
		// type
		ASSERT_FALSE(nil == arr_v);
		ASSERT_FALSE(arr_v == nil);
		ASSERT_TRUE(nil != arr_v);
		ASSERT_TRUE(arr_v != nil);
		ASSERT_FALSE(nil == arr_v);
		ASSERT_TRUE(nil != arr_v);
		ASSERT_FALSE(arr_v == obj_v);
		ASSERT_TRUE(arr_v != obj_v);

		// value
		ASSERT_TRUE(arr_v == a);
		ASSERT_TRUE(a == arr_v);
		ASSERT_TRUE(arr_v != a1);
		ASSERT_TRUE(a1 != arr_v);
		ASSERT_TRUE(arr_v != a2);
		ASSERT_TRUE(a2 != arr_v);

		/* object */
		// type
		ASSERT_FALSE(nil == obj_v);
		ASSERT_FALSE(obj_v == nil);
		ASSERT_TRUE(nil != obj_v);
		ASSERT_TRUE(obj_v != nil);
		ASSERT_FALSE(nil == obj_v);
		ASSERT_TRUE(nil != obj_v);

		// value
		ASSERT_TRUE(obj_v == o);
		ASSERT_TRUE(o == obj_v);
		ASSERT_TRUE(obj_v != o1);
		ASSERT_TRUE(o1 != obj_v);
		ASSERT_TRUE(obj_v != o2);
		ASSERT_TRUE(o2 != obj_v);
		ASSERT_TRUE(obj_v != o3);
		ASSERT_TRUE(o3 != obj_v);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, type)
{
	try {
		// const member function no type conversion
		const JSON::ValueW nil;
		EXPECT_THROW(nil.i(), std::logic_error);
		EXPECT_THROW((int64_t)nil, std::logic_error);

		EXPECT_THROW(nil.f(), std::logic_error);
		EXPECT_THROW((double)nil, std::logic_error);

		EXPECT_THROW(nil.b(), std::logic_error);
		EXPECT_THROW((bool)nil, std::logic_error);

		EXPECT_THROW(nil.s(), std::logic_error);
		EXPECT_THROW((wstring)nil, std::logic_error);

		EXPECT_THROW(nil.o(), std::logic_error);
		EXPECT_THROW((JSON::ObjectW)nil, std::logic_error);

		EXPECT_THROW(nil.a(), std::logic_error);
		EXPECT_THROW((JSON::ArrayW)nil, std::logic_error);

		JSON::ValueW i(0);
		ASSERT_TRUE(i.type() == JSON::INTEGER);
		ASSERT_TRUE(i.i() == 0);
		ASSERT_TRUE((int64_t)i == 0);

		JSON::ValueW f(0.1);
		ASSERT_TRUE(f.type() == JSON::FLOAT);
		ASSERT_TRUE(f.f() == 0.1);
		ASSERT_TRUE((double)f == 0.1);

		JSON::ValueW s(JSON::STRING);
		ASSERT_TRUE(s.type() == JSON::STRING);
		ASSERT_TRUE(s.s().empty());
		ASSERT_TRUE(((wstring)s).empty());

		JSON::ValueW o(JSON::OBJECT);
		ASSERT_TRUE(o.type() == JSON::OBJECT);
		ASSERT_TRUE(((JSON::ObjectW)o).empty());

		JSON::ValueW a(JSON::ARRAY);
		ASSERT_TRUE(a.type() == JSON::ARRAY);
		ASSERT_TRUE(((JSON::ArrayW)a).empty());

		// non-const member function auto type conversion
		JSON::ValueW i1;
		ASSERT_TRUE(i1.type() == JSON::NIL);
		ASSERT_TRUE(i1.i() == 0);
		ASSERT_TRUE(i1.type() == JSON::INTEGER);

		JSON::ValueW f1;
		ASSERT_TRUE(f1.type() == JSON::NIL);
		ASSERT_TRUE(f1.f() == 0);
		ASSERT_TRUE(f1.type() == JSON::FLOAT);

		JSON::ValueW s1;
		ASSERT_TRUE(s1.type() == JSON::NIL);
		ASSERT_TRUE(s1.s().empty());
		ASSERT_TRUE(s1.type() == JSON::STRING);

		JSON::ValueW o1;
		ASSERT_TRUE(o1.type() == JSON::NIL);
		ASSERT_TRUE(o1.o().empty());
		ASSERT_TRUE(o1.type() == JSON::OBJECT);

		JSON::ValueW a1;
		ASSERT_TRUE(a1.type() == JSON::NIL);
		ASSERT_TRUE(a1.a().empty());
		ASSERT_TRUE(a1.type() == JSON::ARRAY);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, get)
{
	try {
		JSON::ValueW v;
		v[L"integer"] = 0;
		v[L"float"] = 0.1;
		v[L"boolean"] = true;
		v[L"string"] = L"test";
		v[L"array"].a().push_back(100);
		v[L"object"][L"elem"] = 0;

		const JSON::ValueW& cv = v;
		int i = cv.get<int>(L"integer", 10);
		ASSERT_TRUE(i == 0);
		i = cv.get<int>(L"integer_not_exist", 10);
		ASSERT_TRUE(i == 10);

		i = v[L"integer"].get<int>(10);
		ASSERT_TRUE(i == 0);
		i = v[L"integer_not_exist"].get<int>(10);
		ASSERT_TRUE(i == 10);

		float f = cv.get<float>(L"float", 10);
		ASSERT_TRUE(fabs(f - 0.1) < 1E-6);
		f = cv.get<float>(L"float_not_exist", 10);
		ASSERT_TRUE(fabs(f - 10) < 1E-6);

		f = v[L"float"].get<float>(10);
		ASSERT_TRUE(fabs(f - 0.1) < 1E-6);
		f = v[L"float_not_exist"].get<float>(10);
		ASSERT_TRUE(fabs(f - 10) < 1E-6);

		bool b = cv.get(L"boolean", false);
		ASSERT_TRUE(b == true);
		b = cv.get(L"boolean_not_exist", false);
		ASSERT_TRUE(b == false);

		b = v[L"boolean"].get<bool>(false);
		ASSERT_TRUE(b == true);
		b = v[L"boolean_not_exist"].get<bool>(false);
		ASSERT_TRUE(b == false);

		wstring s_tmp(L"test1");
		wstring s = cv.get(L"string", s_tmp);
		ASSERT_TRUE(s == L"test");
		s = cv.get(L"string_not_exist", s_tmp);
		ASSERT_TRUE(s == s_tmp);
		
		s = v[L"string"].get<wstring>(s_tmp);
		ASSERT_TRUE(s == L"test");
		s = v[L"string_not_exist"].get<wstring>(s_tmp);
		ASSERT_TRUE(s == s_tmp);

		JSON::ValueW ev;
		const JSON::ValueW& ecv = ev;
		i = ecv.get<int>(L"integer_not_exist", 10);
		ASSERT_TRUE(i == 10);

		f = ecv.get<float>(L"float_not_exist", 10);
		ASSERT_TRUE(fabs(f - 10) < 1E-6);

		b = ecv.get(L"boolean_not_exist", false);
		ASSERT_TRUE(b == false);

		s = ecv.get(L"string_not_exist", s_tmp);
		ASSERT_TRUE(s == s_tmp);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, get_with_casting)
{
	try {
		JSON::ValueW v;
		v[L"i"] = 1;
		v[L"f"] = 0.1;
		v[L"b"] = true;
		v[L"s_i"] = L"1";
		v[L"s_f"] = L"1.1";
		v[L"s_b_t"] = L"true";
		v[L"s_b_t_1"] = L"1";
		v[L"s_b_t_2"] = L"0.1";
		v[L"s_b_f"] = L"false";
		v[L"s_b_f_1"] = L"0";
		v[L"s_b_f_2"] = L"0.0";
		v[L"s_b_f_3"] = L"unknown";

		// 1. cast to integer
		ASSERT_TRUE(v.get<char>(L"i", 0) == 1);
		ASSERT_TRUE(v.get<char>(L"f", 1) == 0);
		ASSERT_TRUE(v.get<char>(L"b", 0) == 1);
		ASSERT_TRUE(v.get<char>(L"s_i", 0) == 1);
		ASSERT_TRUE(v.get<char>(L"s_f", 0) == 1);
		ASSERT_TRUE(v.get<char>(L"s_b_t", 0) == 1);
		ASSERT_TRUE(v.get<char>(L"s_b_f", 1) == 0);
		ASSERT_TRUE(v.get<char>(L"s_b_f_3", 0) == 0);
		ASSERT_TRUE(v.get<char>(L"s_b_f_3", 1) == 1);

		ASSERT_TRUE(v[L"i"].get<char>(0) == 1);
		ASSERT_TRUE(v[L"f"].get<char>(1) == 0);
		ASSERT_TRUE(v[L"b"].get<char>(0) == 1);
		ASSERT_TRUE(v[L"s_i"].get<char>(0) == 1);
		ASSERT_TRUE(v[L"s_f"].get<char>(0) == 1);
		ASSERT_TRUE(v[L"s_b_t"].get<char>(0) == 1);
		ASSERT_TRUE(v[L"s_b_f"].get<char>(1) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<char>(0) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<char>(1) == 1);

		ASSERT_TRUE(v.get<short>(L"i", 0) == 1);
		ASSERT_TRUE(v.get<short>(L"f", 1) == 0);
		ASSERT_TRUE(v.get<short>(L"b", 0) == 1);
		ASSERT_TRUE(v.get<short>(L"s_i", 0) == 1);
		ASSERT_TRUE(v.get<short>(L"s_f", 0) == 1);
		ASSERT_TRUE(v.get<short>(L"s_b_t", 0) == 1);
		ASSERT_TRUE(v.get<short>(L"s_b_f", 1) == 0);
		ASSERT_TRUE(v.get<short>(L"s_b_f_3", 0) == 0);
		ASSERT_TRUE(v.get<short>(L"s_b_f_3", 1) == 1);

		ASSERT_TRUE(v[L"i"].get<short>(0) == 1);
		ASSERT_TRUE(v[L"f"].get<short>(1) == 0);
		ASSERT_TRUE(v[L"b"].get<short>(0) == 1);
		ASSERT_TRUE(v[L"s_i"].get<short>(0) == 1);
		ASSERT_TRUE(v[L"s_f"].get<short>(0) == 1);
		ASSERT_TRUE(v[L"s_b_t"].get<short>(0) == 1);
		ASSERT_TRUE(v[L"s_b_f"].get<short>(1) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<short>(0) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<short>(1) == 1);

		ASSERT_TRUE(v.get<int>(L"i", 0) == 1);
		ASSERT_TRUE(v.get<int>(L"f", 1) == 0);
		ASSERT_TRUE(v.get<int>(L"b", 0) == 1);
		ASSERT_TRUE(v.get<int>(L"s_i", 0) == 1);
		ASSERT_TRUE(v.get<int>(L"s_f", 0) == 1);
		ASSERT_TRUE(v.get<int>(L"s_b_t", 0) == 1);
		ASSERT_TRUE(v.get<int>(L"s_b_f", 1) == 0);
		ASSERT_TRUE(v.get<int>(L"s_b_f_3", 0) == 0);
		ASSERT_TRUE(v.get<int>(L"s_b_f_3", 1) == 1);

		ASSERT_TRUE(v[L"i"].get<int>(0) == 1);
		ASSERT_TRUE(v[L"f"].get<int>(1) == 0);
		ASSERT_TRUE(v[L"b"].get<int>(0) == 1);
		ASSERT_TRUE(v[L"s_i"].get<int>(0) == 1);
		ASSERT_TRUE(v[L"s_f"].get<int>(0) == 1);
		ASSERT_TRUE(v[L"s_b_t"].get<int>(0) == 1);
		ASSERT_TRUE(v[L"s_b_f"].get<int>(1) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<int>(0) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<int>(1) == 1);

		ASSERT_TRUE(v.get<int64_t>(L"i", 0) == 1);
		ASSERT_TRUE(v.get<int64_t>(L"f", 1) == 0);
		ASSERT_TRUE(v.get<int64_t>(L"b", 0) == 1);
		ASSERT_TRUE(v.get<int64_t>(L"s_i", 0) == 1);
		ASSERT_TRUE(v.get<int64_t>(L"s_f", 0) == 1);
		ASSERT_TRUE(v.get<int64_t>(L"s_b_t", 0) == 1);
		ASSERT_TRUE(v.get<int64_t>(L"s_b_f", 1) == 0);
		ASSERT_TRUE(v.get<int64_t>(L"s_b_f_3", 0) == 0);
		ASSERT_TRUE(v.get<int64_t>(L"s_b_f_3", 1) == 1);

		ASSERT_TRUE(v[L"i"].get<int64_t>(0) == 1);
		ASSERT_TRUE(v[L"f"].get<int64_t>(1) == 0);
		ASSERT_TRUE(v[L"b"].get<int64_t>(0) == 1);
		ASSERT_TRUE(v[L"s_i"].get<int64_t>(0) == 1);
		ASSERT_TRUE(v[L"s_f"].get<int64_t>(0) == 1);
		ASSERT_TRUE(v[L"s_b_t"].get<int64_t>(0) == 1);
		ASSERT_TRUE(v[L"s_b_f"].get<int64_t>(1) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<int64_t>(0) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<int64_t>(1) == 1);

		// 2. cast to float
		ASSERT_TRUE(v.get<float>(L"i", 0) == 1);
		ASSERT_TRUE(fabs(v.get<float>(L"f", 0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE(v.get<float>(L"b", 0) == 1);
		ASSERT_TRUE(v.get<float>(L"s_i", 0) == 1);
		ASSERT_TRUE(fabs(v.get<float>(L"s_f", 0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE(v.get<float>(L"s_b_t", 0) == 1);
		ASSERT_TRUE(v.get<float>(L"s_b_f", 1) == 0);
		ASSERT_TRUE(v.get<float>(L"s_b_f_3", 0) == 0);
		ASSERT_TRUE(v.get<float>(L"s_b_f_3", 1) == 1);

		ASSERT_TRUE(v[L"i"].get<float>(0) == 1);
		ASSERT_TRUE(fabs(v[L"f"].get<float>(0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE(v[L"b"].get<float>(0) == 1);
		ASSERT_TRUE(v[L"s_i"].get<float>(0) == 1);
		ASSERT_TRUE(fabs(v[L"s_f"].get<float>(0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE(v[L"s_b_t"].get<float>(0) == 1);
		ASSERT_TRUE(v[L"s_b_f"].get<float>(1) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<float>(0) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<float>(1) == 1);

		ASSERT_TRUE(v.get<double>(L"i", 0) == 1);
		ASSERT_TRUE(fabs(v.get<double>(L"f", 0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE(v.get<double>(L"b", 0) == 1);
		ASSERT_TRUE(v.get<double>(L"s_i", 0) == 1);
		ASSERT_TRUE(fabs(v.get<double>(L"s_f", 0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE(v.get<double>(L"s_b_t", 0) == 1);
		ASSERT_TRUE(v.get<double>(L"s_b_f", 1) == 0);
		ASSERT_TRUE(v.get<double>(L"s_b_f_3", 0) == 0);
		ASSERT_TRUE(v.get<double>(L"s_b_f_3", 1) == 1);

		ASSERT_TRUE(v[L"i"].get<double>(0) == 1);
		ASSERT_TRUE(fabs(v[L"f"].get<double>(0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE(v[L"b"].get<double>(0) == 1);
		ASSERT_TRUE(v[L"s_i"].get<double>(0) == 1);
		ASSERT_TRUE(fabs(v[L"s_f"].get<double>(0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE(v[L"s_b_t"].get<double>(0) == 1);
		ASSERT_TRUE(v[L"s_b_f"].get<double>(1) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<double>(0) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<double>(1) == 1);

		ASSERT_TRUE(v.get<long double>(L"i", 0) == 1);
		ASSERT_TRUE(fabs(v.get<long double>(L"f", 0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE(v.get<long double>(L"b", 0) == 1);
		ASSERT_TRUE(v.get<long double>(L"s_i", 0) == 1);
		ASSERT_TRUE(fabs(v.get<long double>(L"s_f", 0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE(v.get<long double>(L"s_b_t", 0) == 1);
		ASSERT_TRUE(v.get<long double>(L"s_b_f", 1) == 0);
		ASSERT_TRUE(v.get<long double>(L"s_b_f_3", 0) == 0);
		ASSERT_TRUE(v.get<long double>(L"s_b_f_3", 1) == 1);

		ASSERT_TRUE(v[L"i"].get<long double>(0) == 1);
		ASSERT_TRUE(fabs(v[L"f"].get<long double>(0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE(v[L"b"].get<long double>(0) == 1);
		ASSERT_TRUE(v[L"s_i"].get<long double>(0) == 1);
		ASSERT_TRUE(fabs(v[L"s_f"].get<long double>(0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE(v[L"s_b_t"].get<long double>(0) == 1);
		ASSERT_TRUE(v[L"s_b_f"].get<long double>(1) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<long double>(0) == 0);
		ASSERT_TRUE(v[L"s_b_f_3"].get<long double>(1) == 1);

		// 3. cast to bool
		ASSERT_TRUE(v.get<bool>(L"i", false) == true);
		ASSERT_TRUE(v.get<bool>(L"f", true) == true);
		ASSERT_TRUE(v.get<bool>(L"b", false) == true);
		ASSERT_TRUE(v.get<bool>(L"s_i", false) == true);
		ASSERT_TRUE(v.get<bool>(L"s_f", false) == true);
		ASSERT_TRUE(v.get<bool>(L"s_b_t", false) == true);
		ASSERT_TRUE(v.get<bool>(L"s_b_t_1", false) == true);
		ASSERT_TRUE(v.get<bool>(L"s_b_t_2", false) == true);
		ASSERT_TRUE(v.get<bool>(L"s_b_f", true) == false);
		ASSERT_TRUE(v.get<bool>(L"s_b_f_1", true) == false);
		ASSERT_TRUE(v.get<bool>(L"s_b_f_2", true) == false);
		ASSERT_TRUE(v.get<bool>(L"s_b_f_3", true) == true);
		ASSERT_TRUE(v.get<bool>(L"s_b_f_3", false) == false);

		ASSERT_TRUE(v[L"i"].get<bool>(false) == true);
		ASSERT_TRUE(v[L"f"].get<bool>(true) == true);
		ASSERT_TRUE(v[L"b"].get<bool>(false) == true);
		ASSERT_TRUE(v[L"s_i"].get<bool>(false) == true);
		ASSERT_TRUE(v[L"s_f"].get<bool>(false) == true);
		ASSERT_TRUE(v[L"s_b_t"].get<bool>(false) == true);
		ASSERT_TRUE(v[L"s_b_t_1"].get<bool>(false) == true);
		ASSERT_TRUE(v[L"s_b_t_2"].get<bool>(false) == true);
		ASSERT_TRUE(v[L"s_b_f"].get<bool>(true) == false);
		ASSERT_TRUE(v[L"s_b_f_1"].get<bool>(true) == false);
		ASSERT_TRUE(v[L"s_b_f_2"].get<bool>(true) == false);
		ASSERT_TRUE(v[L"s_b_f_3"].get<bool>(true) == true);
		ASSERT_TRUE(v[L"s_b_f_3"].get<bool>(false) == false);

		// 4. cast to string
		wstring empty;
		ASSERT_TRUE(v.get<wstring>(L"i", empty) == L"1");
		ASSERT_TRUE(v.get<wstring>(L"f", empty) == L"0.1");
		ASSERT_TRUE(v.get<wstring>(L"b", empty) == L"true");
		ASSERT_TRUE(v.get<wstring>(L"s_i", empty) == L"1");
		ASSERT_TRUE(v.get<wstring>(L"s_f", empty) == L"1.1");
		ASSERT_TRUE(v.get<wstring>(L"s_b_t", empty) == L"true");
		ASSERT_TRUE(v.get<wstring>(L"s_b_t_1", empty) == L"1");
		ASSERT_TRUE(v.get<wstring>(L"s_b_t_2", empty) == L"0.1");
		ASSERT_TRUE(v.get<wstring>(L"s_b_f", empty) == L"false");
		ASSERT_TRUE(v.get<wstring>(L"s_b_f_1", empty) == L"0");
		ASSERT_TRUE(v.get<wstring>(L"s_b_f_2", empty) == L"0.0");

		ASSERT_TRUE(v[L"i"].get<wstring>(empty) == L"1");
		ASSERT_TRUE(v[L"f"].get<wstring>(empty) == L"0.1");
		ASSERT_TRUE(v[L"b"].get<wstring>(empty) == L"true");
		ASSERT_TRUE(v[L"s_i"].get<wstring>(empty) == L"1");
		ASSERT_TRUE(v[L"s_f"].get<wstring>(empty) == L"1.1");
		ASSERT_TRUE(v[L"s_b_t"].get<wstring>(empty) == L"true");
		ASSERT_TRUE(v[L"s_b_t_1"].get<wstring>(empty) == L"1");
		ASSERT_TRUE(v[L"s_b_t_2"].get<wstring>(empty) == L"0.1");
		ASSERT_TRUE(v[L"s_b_f"].get<wstring>(empty) == L"false");
		ASSERT_TRUE(v[L"s_b_f_1"].get<wstring>(empty) == L"0");
		ASSERT_TRUE(v[L"s_b_f_2"].get<wstring>(empty) == L"0.0");
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, read_string)
{
	try {
		JSON::ValueW v;

		// 1. normal cases
		// case 1 no whitespace character on both sides
		ASSERT_TRUE(v.read_string(L"\"abc\"", 5) == 5);
		ASSERT_TRUE(v.s() == L"abc");

		// case 2 whitespace character before
		ASSERT_TRUE(v.read_string(L" \r\n\t\"abc\"", 9) == 9);
		ASSERT_TRUE(v.s() == L"abc");

		// case 3 escape
		wstring in(L"\"\\\\\\/\\b\\f\\n\\r\\t\"");
		ASSERT_TRUE(v.read_string(in.c_str(), in.length()) == in.length());
		ASSERT_TRUE(v.s() == L"\\/\b\f\n\r\t");

		// check escape string equal
		wstring out;
		v.write(out);
		ASSERT_TRUE(in == out);

		in = L"\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\u0008\\u0009\\u000a\\u000b\\u000c\\u000d\\u000e\\u000f\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f\"";
		ASSERT_TRUE(v.read_string(in.c_str(), in.length()) == in.length());
		ASSERT_TRUE(v.s() == wstring(L"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f", 32));

		in = L"\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f\"";
		ASSERT_TRUE(v.read_string(in.c_str(), in.length()) == in.length());
		ASSERT_TRUE(v.s() == wstring(L"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f", 32));

		// check escape string equal
		out.clear();
		v.write(out);
		ASSERT_TRUE(in == out);

		wstring sepcial_in = wstring(L"\"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\"", 34);
		ASSERT_TRUE(v.read_string(sepcial_in.c_str(), sepcial_in.length()) == sepcial_in.length());

		// check escape string equal
		out.clear();
		v.write(out);
		ASSERT_TRUE(in == out);

		ASSERT_TRUE(v.read_string(L"\"\\\"\"", 4) == 4);
		ASSERT_TRUE(v.s() == L"\"");

		// case 4 test rewrite
		v = JSON_MOVE(wstring(L"test"));
		ASSERT_TRUE(v.read_string(L"\"abc\"", 5) == 5);
		ASSERT_TRUE(v.s() == L"abc");
		v = JSON_MOVE(wstring(L"test"));
		ASSERT_TRUE(v.read_string(L"\"\b\"", 3) == 3);
		ASSERT_TRUE(v.s() == L"\b");

		// case 5 decode as possible
		ASSERT_TRUE(v.read_string(L"\"abc\" {} []", 11) == 5);
		ASSERT_TRUE(v.s() == L"abc");

		// 2. exception cases
		// case 1 no left double quotation
		EXPECT_THROW(v.read_string(L"abc", 3), std::logic_error);

		// case 2 no right double quotation
		EXPECT_THROW(v.read_string(L"\"abc", 4), std::logic_error);
		EXPECT_THROW(v.read_string(L"\"a\bc", 4), std::logic_error);
		EXPECT_THROW(v.read_string(L"\"a\\\"\bc", 6), std::logic_error);

		// case 3 non-complete escape string
		EXPECT_THROW(v.read_string(L"\"a\\", 6), std::logic_error);

		// case 4 unkonwn escape sequence
		EXPECT_THROW(v.read_string(L"\"\\v\"", 4), std::logic_error);

		// case 5 all whitespace
		EXPECT_THROW(v.read_string(L" \t\r\n", 4), std::logic_error);

		// case 6 capital u
		EXPECT_THROW(v.read_string(L"\"\\U75ab\"", 8), std::logic_error);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, read_number)
{
	try {
		JSON::ValueW v;

		// 1. normal cases
		// case 1 zero
		ASSERT_TRUE(v.read_number(L"0", 1) == 1);
		ASSERT_TRUE(v.i() == 0);
		ASSERT_TRUE(v.read_number(L"0]", 2) == 1);
		ASSERT_TRUE(v.i() == 0);
		ASSERT_TRUE(v.read_number(L"-0,", 3) == 2);
		ASSERT_TRUE(v.i() == 0);
		ASSERT_TRUE(v.read_number(L"-0}", 3) == 2);
		ASSERT_TRUE(v.i() == 0);

		// case 2 normal integer number
		ASSERT_TRUE(v.read_number(L"12345", 5) == 5);
		ASSERT_TRUE(v.i() == 12345);
		ASSERT_TRUE(v.read_number(L"12345}", 6) == 5);
		ASSERT_TRUE(v.i() == 12345);
		ASSERT_TRUE(v.read_number(L"-12345,", 7) == 6);
		ASSERT_TRUE(v.i() == -12345);
		ASSERT_TRUE(v.read_number(L"-12345]", 7) == 6);
		ASSERT_TRUE(v.i() == -12345);

		// case 3 integer number limits
		ASSERT_TRUE(v.read_number(L"2147483647", 10) == 10);
		ASSERT_TRUE(v.i() == INT_MAX);
		ASSERT_TRUE(v.read_number(L"2147483647,", 11) == 10);
		ASSERT_TRUE(v.i() == INT_MAX);
		ASSERT_TRUE(v.read_number(L"-2147483648}", 12) == 11);
		ASSERT_TRUE(v.i() == INT_MIN);
		ASSERT_TRUE(v.read_number(L"9223372036854775807]", 20) == 19);
		ASSERT_TRUE(v.i() == LLONG_MAX);
		ASSERT_TRUE(v.read_number(L"-9223372036854775808,", 21) == 20);
		ASSERT_TRUE(v.i() == LLONG_MIN);

		// case 4 floating number
		ASSERT_TRUE(v.read_number(L"-0.0", 4) == 4);
		ASSERT_TRUE(v.f() == 0);
		ASSERT_TRUE(v.read_number(L"-0.0,", 5) == 4);
		ASSERT_TRUE(v.f() == 0);
		ASSERT_TRUE(v.read_number(L"-123.1e2", 8) == 8);
		ASSERT_TRUE(v.f() == -123.1e2);
		ASSERT_TRUE(v.read_number(L"-3.123e-2", 9) == 9);
		ASSERT_TRUE(v.f() == -3.123e-2);

		// case 5 whitespace characters
		ASSERT_TRUE(v.read_number(L" \r\n\t-3.123e-2", 13) == 13);
		ASSERT_TRUE(v.f() == -3.123e-2);
		ASSERT_TRUE(v.read_number(L" \r\n\t-3.12 3e-2", 14) == 9);
		ASSERT_TRUE(v.f() == -3.12);

		// case 6 special cases
		ASSERT_TRUE(v.read_number(L"-9223372036854775809", 20) == 20);
		ASSERT_TRUE(v.f() == -9.2233720368547758e+18);
		ASSERT_TRUE(v.read_number(L"-9223372036854774784.0", 22) == 22);
		ASSERT_TRUE(v.f() == -9.2233720368547748e+18);
		ASSERT_TRUE(v.read_number(L"9223372036854775808.0", 21) == 21);
		ASSERT_TRUE(v.f() == 9.2233720368547758e+18);
		ASSERT_TRUE(v.read_number(L"7205759403792793199999e-5", 25) == 25);
		ASSERT_TRUE(v.f() == 72057594037927936);
		ASSERT_TRUE(v.read_number(L"7205759403792793200001e-5", 25) == 25);
		ASSERT_TRUE(v.f() == 72057594037927936);
		ASSERT_TRUE(v.read_number(L"1.725073858507201136057409796709131975934819546351645648023426109724822222021076945516529523908135087914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012981122451451889849057222307285255133155755015914397476397983411801999323962548289017107081850690630666655994938275772572015763062690663332647565300009245888316433037779791869612049497390377829704905051080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621572289880258182545180325707018860872113128079512233426288368622321503775666622503982534335974568884423900265498198385487948292206894721689831099698365846814022854243330660339850886445804001034933970427567186443383770486037861622771738545623065874679014086723327636718751234567890123456789012345678901e-290", 805) == 805);
		ASSERT_TRUE(v.f() == 1.7250738585072009e-290);
		ASSERT_TRUE(v.read_number(L"1014120480182583464902367222169599999e-5", 40) == 40);
		ASSERT_TRUE(v.f() == 1.0141204801825834e+31);
		ASSERT_TRUE(v.read_number(L"1014120480182583464902367222169600001e-5", 40) == 40);
		ASSERT_TRUE(v.f() == 1.0141204801825834e+31);
		ASSERT_TRUE(v.read_number(L"5708990770823839207320493820740630171355185151999e-3", 52) == 52);
		ASSERT_TRUE(v.f() == 5.7089907708238389e+45);
		ASSERT_TRUE(v.read_number(L"5708990770823839207320493820740630171355185152001e-3", 52) == 52);
		ASSERT_TRUE(v.f() == 5.7089907708238389e+45);

		// 2. exception cases
		// case 1 unexcept endings after integer number
		EXPECT_THROW(v.read_number(L"0a", 2), std::logic_error);
		EXPECT_THROW(v.read_number(L"-a", 2), std::logic_error);
		EXPECT_THROW(v.read_number(L"-0.a", 4), std::logic_error);
		EXPECT_THROW(v.read_number(L"-0.1a", 5), std::logic_error);
		EXPECT_THROW(v.read_number(L"-0.1ea", 6), std::logic_error);
		EXPECT_THROW(v.read_number(L"-0.1e+a", 7), std::logic_error);
		EXPECT_THROW(v.read_number(L"-0.1e+1a", 8), std::logic_error);

		// case 2 unexpect zero-leading
		EXPECT_THROW(v.read_number(L"00", 2), std::logic_error);
		EXPECT_THROW(v.read_number(L"01", 2), std::logic_error);
		EXPECT_THROW(v.read_number(L"01234", 5), std::logic_error);
		EXPECT_THROW(v.read_number(L"-012.34", 7), std::logic_error);

		// case 3 no number after floating point
		EXPECT_THROW(v.read_number(L"0.", 2), std::logic_error);
		EXPECT_THROW(v.read_number(L"0.,", 2), std::logic_error);

		// case 4 no number after exp
		EXPECT_THROW(v.read_number(L"0.1e", 4), std::logic_error);
		EXPECT_THROW(v.read_number(L"0.1e+", 5), std::logic_error);

		// case 5 positive sign
		EXPECT_THROW(v.read_number(L"+123", 4), std::logic_error);
		EXPECT_THROW(v.read_number(L"+13.2", 5), std::logic_error);

		// case 6 all whitespace characters
		EXPECT_THROW(v.read_number(L" \t\r\n", 4), std::logic_error);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, read_boolean)
{
	try {
		JSON::ValueW v;

		// 1. normal cases
		// case 1 
		ASSERT_TRUE(v.read_boolean(L"true", 4) == 4);
		ASSERT_TRUE(v.b() == true);
		ASSERT_TRUE(v.read_boolean(L"false", 5) == 5);
		ASSERT_TRUE(v.b() == false);

		// case 2 whitespace characters
		ASSERT_TRUE(v.read_boolean(L" \r\n\ttrue", 8) == 8);
		ASSERT_TRUE(v.b() == true);
		ASSERT_TRUE(v.read_boolean(L" \r\n\tfalse", 9) == 9);
		ASSERT_TRUE(v.b() == false);

		// 2. exception cases
		// case 1 insufficient data
		EXPECT_THROW(v.read_boolean(L"tru", 3), std::logic_error);
		EXPECT_THROW(v.read_boolean(L"fals", 4), std::logic_error);
		EXPECT_THROW(v.read_boolean(L" \r\n\ttru", 7), std::logic_error);
		EXPECT_THROW(v.read_boolean(L" \r\n\tfals", 8), std::logic_error);

		// case 2 typos
		EXPECT_THROW(v.read_boolean(L"trua", 4), std::logic_error);
		EXPECT_THROW(v.read_boolean(L"falsv", 5), std::logic_error);

		// case 3 capital error
		EXPECT_THROW(v.read_boolean(L"True", 4), std::logic_error);
		EXPECT_THROW(v.read_boolean(L"falsE", 5), std::logic_error);

		// case 4 all whitespace characters
		EXPECT_THROW(v.read_boolean(L" \t\r\n", 4), std::logic_error);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, read_nil)
{
	try {
		JSON::ValueW v;

		// 1. normal cases
		// case 1 
		ASSERT_TRUE(v.read_nil(L"null", 4) == 4);
		ASSERT_TRUE(v.type() == JSON::NIL);

		// case 2 whitespace characters
		ASSERT_TRUE(v.read_nil(L" \r\n\tnull", 8) == 8);
		ASSERT_TRUE(v.type() == JSON::NIL);

		// 2. exception cases
		// case 1 insufficient data
		EXPECT_THROW(v.read_nil(L"nul", 3), std::logic_error);
		EXPECT_THROW(v.read_nil(L" \r\n\tnul", 7), std::logic_error);

		// case 2 typos
		EXPECT_THROW(v.read_nil(L"nule", 4), std::logic_error);
		EXPECT_THROW(v.read_nil(L"nill", 3), std::logic_error);

		// case 3 capital error
		EXPECT_THROW(v.read_nil(L"Null", 4), std::logic_error);
		EXPECT_THROW(v.read_nil(L"NULL", 4), std::logic_error);

		// case 4 all whitespace characters
		EXPECT_THROW(v.read_nil(L" \t\r\n", 4), std::logic_error);
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}

TEST(ut_xpjsonW, zh_cn)
{
	try {
		// UTF16 encoding
		// 正常测试
		// 未转义的
		wstring chinese = L"\"\u6d4b\u8bd5\u4e2d\u6587\"";
		JSON::ValueW v;
		ASSERT_TRUE(v.read_string(chinese.c_str(), chinese.length()) == chinese.length());
		ASSERT_TRUE(v.s() == L"测试中文");

		// after escape
		chinese = L"\"\\u6d4b\\u8bd5\\u4e2d\\u6587\"";
		ASSERT_TRUE(v.read_string(chinese.c_str(), chinese.length()) == chinese.length());
		ASSERT_TRUE(v.s() == L"测试中文");

		chinese = L"用户名或密码不正确";
		v = JSON_MOVE(JSON::ObjectW());
		v.o()[L"test"] = JSON_MOVE(JSON::ValueW(chinese));

		wstring out;
		JSON::WriterW::write(v.o(), out);
		ASSERT_TRUE(out == L"{\"test\":\"\\u7528\\u6237\\u540d\\u6216\\u5bc6\\u7801\\u4e0d\\u6b63\\u786e\"}");

		JSON::ReaderW::read(v, out.c_str(), out.length());
		ASSERT_TRUE(v.type() == JSON::OBJECT);
		ASSERT_TRUE(v.o()[L"test"].s() == chinese);

		// exception cases
		// case 1 not 0-9A-F
		wstring in(L"\"\\u75ag\"");
		EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);

		// case 2 insufficient data
		in = L"\"\\u\"";
		EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);
		in = L"\"\\u2\"";
		EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);
		in = L"\"\\u32\"";
		EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);
		in = L"\"\\u75a\"";
		EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);

		// UTF32 encoding
		if(sizeof(wchar_t) == 4) {
			// normal case
			in = L"\"\\ud84c\\udf50\"";
			ASSERT_TRUE(v.read_string(in.c_str(), in.length()) == in.length());
			ASSERT_TRUE(v.s() == L"𣍐");

			out.clear();
			v.write(out);
			ASSERT_TRUE(out == in);

			// exception cases
			// case 1 insufficient data
			in = L"\"\\uD84C\"";
			EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);
			in = L"\"\\uD84C\\uDF\"";
			EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);

			// case 2 not \u
			in = L"\"\\uD84C/uDF50\"";
			EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);
			in = L"\"\\uD84C\\xDF50\"";
			EXPECT_THROW(v.read_string(in.c_str(), in.length()), std::logic_error);
		}
	}
	catch(std::exception &e) {
		printf("Error : %s.", e.what());
		ASSERT_TRUE(false);
	}
}
