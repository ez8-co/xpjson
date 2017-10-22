#define protected public

#include "gtest/gtest.h"

#include <iostream>
#include <climits>
#include "../xpjson.hpp"

TEST (ut_xpjson, write)
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
	try {
		JSON::Array a;
		a.push_back (JSON::NIL);
		a.push_back (0x7FFFFFFF);
		a.push_back ((int64_t)0xFFFFFFFFF);
		a.push_back (0.1);
		a.push_back (true);
		a.push_back (false);
		a.push_back (JSON_MOVE (JSON::Value (JSON_MOVE (string ("test\"\\/\b\f\n\r\t")))));
		// indicate DO NOT escape
		a.push_back (JSON_MOVE (JSON::Value (JSON_MOVE (string ("test")), false)));

		string out;
		// reserve 100 bytes, reduce reallocation and copy cost of time
		out.reserve (100);
		JSON::Writer::write (a, out);
		ASSERT_TRUE (out == "[null,2147483647,68719476735,0.1,true,false,\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\",\"test\"]");

		JSON::Object o;
		o.insert (make_pair (JSON_MOVE (string ("test")), JSON_MOVE (a)));

		out.clear ();
		// reserve 100 bytes, reduce reallocation and copy cost of time
		out.reserve (100);
		JSON::Writer::write (o, out);
		ASSERT_TRUE (out == "{\"test\":[null,2147483647,68719476735,0.1,true,false,\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\",\"test\"]}");

		JSON::Value v (-1.797693134862315e+308);
		out.clear ();
		v.write (out);
		ASSERT_TRUE (out == "-1.797693134862315e+308");
		ASSERT_TRUE (fabs (v.f () + DBL_MAX) < 1e+293);

		v = -2.225073858507201e-308;
		out.clear ();
		v.write (out);
		ASSERT_TRUE (out == "-2.225073858507201e-308");

		v.read_number (out.c_str (), out.length ());
		ASSERT_TRUE (fabs (v.f () + DBL_MIN) < 1e-293);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, read)
{
	try {
		// normal cases
		// case 1 hybird test
		string in ("[null,2147483647,68719476735,1.3e-12,true,false,\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\",\"test\"]");
		JSON::Value v;
		{
			JSON::Reader::read (v, in.c_str (), in.length ());
			ASSERT_TRUE (v.type () == JSON::ARRAY);

			JSON::Array& a = v.a ();
			ASSERT_TRUE (a.size () == 8);
			ASSERT_TRUE (a[0].type () == JSON::NIL);

			ASSERT_TRUE (a[1].type () == JSON::INTEGER);
			ASSERT_TRUE (a[1].i () == 2147483647);

			ASSERT_TRUE (a[2].type () == JSON::INTEGER);
			ASSERT_TRUE (a[2].i () == 68719476735);

			ASSERT_TRUE (a[3].type () == JSON::FLOAT);
			ASSERT_TRUE (fabs (a[3].f () - 1.3e-12) < 1e-13);

			ASSERT_TRUE (a[4].type () == JSON::BOOLEAN);
			ASSERT_TRUE (a[4].b () == true);

			ASSERT_TRUE (a[5].type () == JSON::BOOLEAN);
			ASSERT_TRUE (a[5].b () == false);

			ASSERT_TRUE (a[6].type () == JSON::STRING);
			ASSERT_TRUE (a[6].s () == "test\"\\/\b\f\n\r\t");

			ASSERT_TRUE (a[7].type () == JSON::STRING);
			ASSERT_TRUE (a[7].s ()  == "test");
		}

		// case 2 hybird test
		{
			in = "  \b\f\r\n\t{\"ver\":123,\r\n \"o\"\f\b:\tnull,\"flag\":true,\"data\":[[0,0.1,1.3e2\f]\r\n]\t  }";
			JSON::Reader::read (v, in.c_str (), in.length ());
			ASSERT_TRUE (v.type () == JSON::OBJECT);

			JSON::Object& o = v.o ();
			ASSERT_TRUE (o["ver"].type () == JSON::INTEGER);
			ASSERT_TRUE (o["ver"].i () == 123);

			ASSERT_TRUE (o["o"].type () == JSON::NIL);

			ASSERT_TRUE (o["flag"].type () == JSON::BOOLEAN);
			ASSERT_TRUE (o["flag"].b () == true);

			ASSERT_TRUE (o["data"].type () == JSON::ARRAY);
			JSON::Array& a1 = o["data"].a ();
			ASSERT_TRUE (a1.size () == 1);

			ASSERT_TRUE (a1[0].type () == JSON::ARRAY);

			JSON::Array& a11 = a1[0].a ();

			ASSERT_TRUE (a11[0].type () == JSON::INTEGER);
			ASSERT_TRUE (a11[0].i () == 0);

			ASSERT_TRUE (a11[1].type () == JSON::FLOAT);
			ASSERT_TRUE (a11[1].f () == 0.1);

			ASSERT_TRUE (a11[2].type () == JSON::FLOAT);
			ASSERT_TRUE (fabs (a11[2].f () - 1.3e2) < 1);
		}

		// case 3 empty object
		{
			in = "{}";
			JSON::Reader::read (v, in.c_str (), in.length ());
			ASSERT_TRUE (v.type () == JSON::OBJECT);

			JSON::Object& o = v.o ();
			ASSERT_TRUE (o.empty ());

			in = "[]";
			JSON::Reader::read (v, in.c_str (), in.length ());
			ASSERT_TRUE (v.type () == JSON::ARRAY);

			JSON::Array& a1 = v.a ();
			ASSERT_TRUE (a1.empty ());
		}

		// case 4 nested array
		{
			in = "[[[[]]]]";
			JSON::Reader::read (v, in.c_str (), in.length ());
			ASSERT_TRUE (v.type () == JSON::ARRAY);

			JSON::Array& a1 = v.a ();
			ASSERT_TRUE (a1.size () == 1);
			ASSERT_TRUE (a1[0].type () == JSON::ARRAY);

			JSON::Array& a11 = a1[0].a ();
			ASSERT_TRUE (a11.size () == 1);
			ASSERT_TRUE (a11[0].type () == JSON::ARRAY);

			JSON::Array& a111 = a11[0].a ();
			ASSERT_TRUE (a111.size () == 1);
			ASSERT_TRUE (a111[0].type () == JSON::ARRAY);

			JSON::Array& a1111 = a111[0].a ();
			ASSERT_TRUE (a1111.empty ());

			in = "[{}]";
			JSON::Reader::read (v, in.c_str (), in.length ());
		}

		// case 5 escape
		{
			in = "{\"a\":\"b\",\"key\":\"test\\\"\\\\\\/\\b\\f\\n\\r\\t\"}";
			JSON::Reader::read (v, in.c_str (), in.length ());
			ASSERT_TRUE (v.type () == JSON::OBJECT);

			JSON::Object& o1 = v.o ();
			ASSERT_TRUE (o1.size () == 2);

			ASSERT_TRUE (o1["a"].type () == JSON::STRING);
			ASSERT_TRUE (o1["a"].s () == "b");

			ASSERT_TRUE (o1["key"].type () == JSON::STRING);
			ASSERT_TRUE (o1["key"].s () == "test\"\\/\b\f\n\r\t");

			string out;
			JSON::Writer::write (o1, out);
			ASSERT_TRUE (in == out);
		}

		// case 6 decode length
		{
			in = "{}";
			ASSERT_TRUE (JSON::Reader::read (v, in.c_str (), in.length ()) == 2);

			in = "{}  testestestest";
			ASSERT_TRUE (JSON::Reader::read (v, in.c_str (), in.length ()) == 2);

			in = "{\"a\":0}  asdasfasdfgag";
			ASSERT_TRUE (JSON::Reader::read (v, in.c_str (), in.length ()) == 7);
		}

		// case 7 whitespace characters
		{
			in = "\r\n\t {\b\f \"a\" \r:\n \"b\" \n\r\t}";
			ASSERT_TRUE (JSON::Reader::read (v, in.c_str (), in.length ()) == in.length ());
		}

		// case 8 duplicate key
		{
			in = "{\"a\":0,\"a\":1}";
			ASSERT_TRUE (JSON::Reader::read (v, in.c_str (), in.length ()) == in.length ());
			ASSERT_TRUE (v.type () == JSON::OBJECT);

			JSON::Object& o1 = v.o ();
			ASSERT_TRUE (o1.size () == 1);

			ASSERT_TRUE (o1["a"].type () == JSON::INTEGER);
			ASSERT_TRUE (o1["a"].i () == 1);
		}

		// case 9 nested object
		{
			in = "{\"a\":{\"a\":1}}";
			ASSERT_TRUE (JSON::Reader::read (v, in.c_str (), in.length ()) == in.length ());
			in = "[{\"a\":1}]";
			ASSERT_TRUE (JSON::Reader::read (v, in.c_str (), in.length ()) == in.length ());
		}

		// exception cases
		// case 1 bracket not match
		in = "{\"a\":\"b\"";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
		in = "[\"a\",\"b\"";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);

		// case 2 no start ({ or [)
		in = "\"a\":\"b\"";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
		in = "abcd";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);

		// case 3 comma before right bracket
		in = "{\"a\":\"b\",}";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
		in = "[\"a\",\"b\",]";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);

		// case 4 whitespace characters/double quotation marks/right brace after left brace
		in = "{ \b\f\r\n\taaa";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);

		// case 5 key not support escape
		in = "{\"aa\\b\":\"b\"}";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);

		// case 6 colon after key
		in = "{\"a\" \b\n\r\f\t a}";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);

		// case 7 invalid value characters
		in = "{\"a\": a}";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
		in = "{\"a\": 1, \"b\":a}";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
		in = "[a]";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
		in = "[123,a]";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);

		// case 8 not comma/whitespace characters/right square bracket/right brace after value
		in = "{\"a\":\"b\"a";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
		in = "[\"a\",\"b\"a";
		EXPECT_THROW (JSON::Reader::read (v, in.c_str (), in.length ()), std::logic_error);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, ctor)
{
	try {
		JSON::Value nil;
		ASSERT_TRUE (nil.type () == JSON::NIL);

		JSON::Value int_v (123);
		ASSERT_TRUE (int_v.type () == JSON::INTEGER);

		JSON::Value int64_t_v (int64_t (2147483647));
		ASSERT_TRUE (int64_t_v.type () == JSON::INTEGER);

		JSON::Value float_v (float (2.14));
		ASSERT_TRUE (float_v.type () == JSON::FLOAT);

		JSON::Value double_v (double (3.1415926));
		ASSERT_TRUE (double_v.type () == JSON::FLOAT);

		JSON::Value bool_v (true);
		ASSERT_TRUE (bool_v.type () == JSON::BOOLEAN);

		JSON::Value cstr_v ("test");
		ASSERT_TRUE (cstr_v.type () == JSON::STRING);

		JSON::Value cstr_l_v ("test", size_t (4));
		ASSERT_TRUE (cstr_l_v.type () == JSON::STRING);

		JSON::Value string_v (string ("test"));
		ASSERT_TRUE (string_v.type () == JSON::STRING);

		JSON::Value object_v (JSON::OBJECT);
		ASSERT_TRUE (object_v.type () == JSON::OBJECT);

		JSON::Value array_v (JSON::ARRAY);
		ASSERT_TRUE (array_v.type () == JSON::ARRAY);

		JSON::Value copy_ctor_v (array_v);
		ASSERT_TRUE (copy_ctor_v.type () == JSON::ARRAY);

#ifdef __XPJSON_CXX11__
		JSON::Value move_ctor_v (JSON_MOVE (copy_ctor_v));
		ASSERT_TRUE (move_ctor_v.type () == JSON::ARRAY);
		ASSERT_TRUE (copy_ctor_v.type () == JSON::NIL);

		string s ("test");
		JSON::Value move_string_ctor_v (JSON_MOVE (s));
		ASSERT_TRUE (move_string_ctor_v.type () == JSON::STRING);
		ASSERT_TRUE (s.empty ());

		JSON::Value move_object_ctor_v (JSON_MOVE (object_v));
		ASSERT_TRUE (move_object_ctor_v.type () == JSON::OBJECT);
		ASSERT_TRUE (object_v.type () == JSON::NIL);

		JSON::Value move_array_ctor_v (JSON_MOVE (array_v));
		ASSERT_TRUE (move_array_ctor_v.type () == JSON::ARRAY);
		ASSERT_TRUE (array_v.type () == JSON::NIL);
#endif
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, assign)
{
	try {
		JSON::Value v;
		ASSERT_TRUE (v.type () == JSON::NIL);

		JSON::Value int_v (123);
		ASSERT_TRUE (int_v.type () == JSON::INTEGER);

		v.assign (int_v);
		ASSERT_TRUE (v.type () == JSON::INTEGER);
		ASSERT_TRUE (v.i () == int_v.i ());

		float f = 1.23f;
		v.assign (f);
		ASSERT_TRUE (v.type () == JSON::FLOAT);
		ASSERT_TRUE (fabs (v.f () - f) < 1e-3);

		int64_t i64 = 123456;
		v.assign (i64);
		ASSERT_TRUE (v.type () == JSON::INTEGER);
		ASSERT_TRUE (v.i () == i64);

		double d = 3.14;
		v.assign (d);
		ASSERT_TRUE (v.type () == JSON::FLOAT);
		ASSERT_TRUE (fabs (v.f () - d) < 1e-3);

		bool b = true;
		v.assign (b);
		ASSERT_TRUE (v.type () == JSON::BOOLEAN);
		ASSERT_TRUE (v.b () == b);

		v.assign ("test");
		ASSERT_TRUE (v.type () == JSON::STRING);
		ASSERT_TRUE (v.s () == "test");

		JSON::Object o;
		v.assign (o);
		ASSERT_TRUE (v.type () == JSON::OBJECT);

		v.assign ("test", size_t (4));
		ASSERT_TRUE (v.type () == JSON::STRING);
		ASSERT_TRUE (v.s () == "test");

		JSON::Array a;
		v.assign (a);
		ASSERT_TRUE (v.type () == JSON::ARRAY);

		string s ("test");
		v.assign (s);
		ASSERT_TRUE (v.type () == JSON::STRING);
		ASSERT_TRUE (v.s () == s);

#ifdef __XPJSON_CXX11__
		JSON::Value v_tmp (1);
		v.assign (JSON_MOVE (v_tmp));
		ASSERT_TRUE (v_tmp.type () == JSON::NIL);
		ASSERT_TRUE (v.type () == JSON::INTEGER);

		v.assign (JSON_MOVE (s));
		ASSERT_TRUE (v.type () == JSON::STRING);
		ASSERT_TRUE (s.empty ());

		o["a"] = 1;
		v.assign (JSON_MOVE (o));
		ASSERT_TRUE (v.type () == JSON::OBJECT);
		ASSERT_TRUE (o.empty ());
		ASSERT_TRUE (!v.o ().empty ());
		ASSERT_TRUE (v.o ()["a"].i () == 1);

		a.push_back (2);
		v.assign (JSON_MOVE (a));
		ASSERT_TRUE (v.type () == JSON::ARRAY);
		ASSERT_TRUE (a.empty ());
		ASSERT_TRUE (!v.a ().empty ());
		ASSERT_TRUE (v.a ()[0].i () == 2);
#endif
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, assignment)
{
	try {
		JSON::Value v;
		ASSERT_TRUE (v.type () == JSON::NIL);

		JSON::Value int_v (123);
		ASSERT_TRUE (int_v.type () == JSON::INTEGER);

		v = int_v;
		ASSERT_TRUE (v.type () == JSON::INTEGER);
		ASSERT_TRUE (v.i () == int_v.i ());

		float f = 1.23f;
		v = f;
		ASSERT_TRUE (v.type () == JSON::FLOAT);
		ASSERT_TRUE (fabs (v.f () - f) < 1e-3);

		int64_t i64 = 123456;
		v = i64;
		ASSERT_TRUE (v.type () == JSON::INTEGER);
		ASSERT_TRUE (v.i () == i64);

		double d = 3.14;
		v = d;
		ASSERT_TRUE (v.type () == JSON::FLOAT);
		ASSERT_TRUE (fabs (v.f () - d) < 1e-3);

		bool b = true;
		v = b;
		ASSERT_TRUE (v.type () == JSON::BOOLEAN);
		ASSERT_TRUE (v.b () == b);

		v = "test";
		ASSERT_TRUE (v.type () == JSON::STRING);
		ASSERT_TRUE (v.s () == "test");

		JSON::Object o;
		v = o;
		ASSERT_TRUE (v.type () == JSON::OBJECT);

		JSON::Array a;
		v = a;
		ASSERT_TRUE (v.type () == JSON::ARRAY);

		string s ("test");
		v = s;
		ASSERT_TRUE (v.type () == JSON::STRING);
		ASSERT_TRUE (v.s () == s);

#ifdef __XPJSON_CXX11__
		JSON::Value v_tmp (1);
		v = JSON_MOVE (v_tmp);
		ASSERT_TRUE (v_tmp.type () == JSON::NIL);
		ASSERT_TRUE (v.type () == JSON::INTEGER);

		v = JSON_MOVE (s);
		ASSERT_TRUE (v.type () == JSON::STRING);
		ASSERT_TRUE (s.empty ());

		o["a"] = 1;
		v = JSON_MOVE (o);
		ASSERT_TRUE (v.type () == JSON::OBJECT);
		ASSERT_TRUE (o.empty ());
		ASSERT_TRUE (!v.o ().empty ());
		ASSERT_TRUE (v.o ()["a"].i () == 1);

		a.push_back (2);
		v = JSON_MOVE (a);
		ASSERT_TRUE (v.type () == JSON::ARRAY);
		ASSERT_TRUE (a.empty ());
		ASSERT_TRUE (!v.a ().empty ());
		ASSERT_TRUE (v.a ()[0].i () == 2);
#endif
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, bracket_operator)
{
	try {
		JSON::Value o (JSON::OBJECT);
		ASSERT_TRUE (o.type () == JSON::OBJECT);

		string elem ("test");
		o["a"] = elem;
		ASSERT_TRUE (o["a"].s () == elem);
		ASSERT_TRUE (o[elem].type () == JSON::NIL);

		o[elem] = elem;
		ASSERT_TRUE (o[elem].s () == elem);

		JSON::Value nil;
		nil["test"] = 0;
		ASSERT_TRUE (nil.type () == JSON::OBJECT);

		JSON::Value nil1;
		nil1[elem] = 0;
		ASSERT_TRUE (nil1.type () == JSON::OBJECT);

		JSON::Value a (JSON::ARRAY);
		a.a ().push_back (1);
		a.a ().push_back (2);
		a.a ().push_back (3);
		for (size_t i = 0; i < a.a ().size (); ++i) {
			a[i] = (int)i;
		}
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, compare_function)
{
	try {
		string str ("test");
		string str1 ("test1");

		JSON::Array a;
		a.push_back (0);
		a.push_back (0.1);
		JSON::Array a1;
		JSON::Array a2;
		a2.push_back (0);
		a2.push_back (0.2);

		JSON::Object o;
		o["a"] = "b";
		o["b"] = "c";
		JSON::Object o1;
		JSON::Object o2;
		o2["a"] = "b";
		o2["b"] = "d";
		JSON::Object o3;
		o3["c"] = "b";
		o3["b"] = "c";

		JSON::Value nil;
		JSON::Value int_v (0);
		JSON::Value float_v (0.1);
		JSON::Value bool_v (true);
		JSON::Value str_v (str);
		JSON::Value arr_v (a);
		JSON::Value obj_v (o);

		/* int */
		// type
		ASSERT_FALSE (nil == 1);
		ASSERT_FALSE (1 == nil);
		ASSERT_TRUE (nil != 1);
		ASSERT_TRUE (1 != nil);
		ASSERT_FALSE (nil == int_v);
		ASSERT_TRUE (nil != int_v);
		ASSERT_FALSE (int_v == float_v);
		ASSERT_TRUE (int_v != float_v);
		ASSERT_FALSE (int_v == bool_v);
		ASSERT_TRUE (int_v != bool_v);
		ASSERT_FALSE (int_v == str_v);
		ASSERT_TRUE (int_v != str_v);
		ASSERT_FALSE (int_v == arr_v);
		ASSERT_TRUE (int_v != arr_v);
		ASSERT_FALSE (int_v == obj_v);
		ASSERT_TRUE (int_v != obj_v);
		
		// value
		ASSERT_TRUE (int_v == 0);
		ASSERT_TRUE (0 == int_v);
		ASSERT_TRUE (int_v != 1);
		ASSERT_TRUE (1 != int_v);

		/* float_v */
		// type
		ASSERT_FALSE (nil == 0.1);
		ASSERT_FALSE (0.1 == nil);
		ASSERT_TRUE (nil != 0.1);
		ASSERT_TRUE (0.1 != nil);
		ASSERT_FALSE (nil == float_v);
		ASSERT_TRUE (nil != float_v);
		ASSERT_FALSE (float_v == bool_v);
		ASSERT_TRUE (float_v != bool_v);
		ASSERT_FALSE (float_v == str_v);
		ASSERT_TRUE (float_v != str_v);
		ASSERT_FALSE (float_v == arr_v);
		ASSERT_TRUE (float_v != arr_v);
		ASSERT_FALSE (float_v == obj_v);
		ASSERT_TRUE (float_v != obj_v);

		// value
		ASSERT_TRUE (float_v == 0.1);
		ASSERT_TRUE (0.1 == float_v);
		ASSERT_TRUE (float_v != 0.2);
		ASSERT_TRUE (0.2 != float_v);

		/* bool_v */
		// type
		ASSERT_FALSE (nil == true);
		ASSERT_FALSE (true == nil);
		ASSERT_TRUE (nil != true);
		ASSERT_TRUE (true != nil);
		ASSERT_FALSE (nil == bool_v);
		ASSERT_TRUE (nil != bool_v);
		ASSERT_FALSE (bool_v == str_v);
		ASSERT_TRUE (bool_v != str_v);
		ASSERT_FALSE (bool_v == arr_v);
		ASSERT_TRUE (bool_v != arr_v);
		ASSERT_FALSE (bool_v == obj_v);
		ASSERT_TRUE (bool_v != obj_v);

		// value
		ASSERT_TRUE (bool_v == true);
		ASSERT_TRUE (true == bool_v);
		ASSERT_TRUE (bool_v != false);
		ASSERT_TRUE (false != bool_v);

		/* string */
		// type
		ASSERT_FALSE (nil == str);
		ASSERT_FALSE (str == nil);
		ASSERT_TRUE (nil != str);
		ASSERT_TRUE (str != nil);
		ASSERT_FALSE (nil == str_v);
		ASSERT_TRUE (nil != str_v);
		ASSERT_FALSE (bool_v == arr_v);
		ASSERT_TRUE (bool_v != arr_v);
		ASSERT_FALSE (bool_v == obj_v);
		ASSERT_TRUE (bool_v != obj_v);

		// value
		ASSERT_TRUE (str_v == str);
		ASSERT_TRUE (str == str_v);
		ASSERT_TRUE (str_v != str1);
		ASSERT_TRUE (str1 != str_v);

		/* array */
		// type
		ASSERT_FALSE (nil == arr_v);
		ASSERT_FALSE (arr_v == nil);
		ASSERT_TRUE (nil != arr_v);
		ASSERT_TRUE (arr_v != nil);
		ASSERT_FALSE (nil == arr_v);
		ASSERT_TRUE (nil != arr_v);
		ASSERT_FALSE (arr_v == obj_v);
		ASSERT_TRUE (arr_v != obj_v);

		// value
		ASSERT_TRUE (arr_v == a);
		ASSERT_TRUE (a == arr_v);
		ASSERT_TRUE (arr_v != a1);
		ASSERT_TRUE (a1 != arr_v);
		ASSERT_TRUE (arr_v != a2);
		ASSERT_TRUE (a2 != arr_v);

		/* object */
		// type
		ASSERT_FALSE (nil == obj_v);
		ASSERT_FALSE (obj_v == nil);
		ASSERT_TRUE (nil != obj_v);
		ASSERT_TRUE (obj_v != nil);
		ASSERT_FALSE (nil == obj_v);
		ASSERT_TRUE (nil != obj_v);

		// value
		ASSERT_TRUE (obj_v == o);
		ASSERT_TRUE (o == obj_v);
		ASSERT_TRUE (obj_v != o1);
		ASSERT_TRUE (o1 != obj_v);
		ASSERT_TRUE (obj_v != o2);
		ASSERT_TRUE (o2 != obj_v);
		ASSERT_TRUE (obj_v != o3);
		ASSERT_TRUE (o3 != obj_v);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, type)
{
	try {
		// const member function no type conversion
		const JSON::Value nil;
		EXPECT_THROW (nil.i (), std::logic_error);
		EXPECT_THROW ((int64_t)nil, std::logic_error);

		EXPECT_THROW (nil.f (), std::logic_error);
		EXPECT_THROW ((double)nil, std::logic_error);

		EXPECT_THROW (nil.b (), std::logic_error);
		EXPECT_THROW ((bool)nil, std::logic_error);

		EXPECT_THROW (nil.s (), std::logic_error);
		EXPECT_THROW ((string)nil, std::logic_error);

		EXPECT_THROW (nil.o (), std::logic_error);
		EXPECT_THROW ((JSON::Object)nil, std::logic_error);

		EXPECT_THROW (nil.a (), std::logic_error);
		EXPECT_THROW ((JSON::Array)nil, std::logic_error);

		JSON::Value i (0);
		ASSERT_TRUE (i.type () == JSON::INTEGER);
		ASSERT_TRUE (i.i () == 0);
		ASSERT_TRUE ((int64_t)i == 0);

		JSON::Value f (0.1);
		ASSERT_TRUE (f.type () == JSON::FLOAT);
		ASSERT_TRUE (f.f () == 0.1);
		ASSERT_TRUE ((double)f == 0.1);

		JSON::Value s (JSON::STRING);
		ASSERT_TRUE (s.type () == JSON::STRING);
		ASSERT_TRUE (s.s ().empty ());
		ASSERT_TRUE (((string)s).empty ());

		JSON::Value o (JSON::OBJECT);
		ASSERT_TRUE (o.type () == JSON::OBJECT);
		ASSERT_TRUE (((JSON::Object)o).empty ());

		JSON::Value a (JSON::ARRAY);
		ASSERT_TRUE (a.type () == JSON::ARRAY);
		ASSERT_TRUE (((JSON::Array)a).empty ());

		// non-const member function auto type conversion
		JSON::Value i1;
		ASSERT_TRUE (i1.type () == JSON::NIL);
		ASSERT_TRUE (i1.i () == 0);
		ASSERT_TRUE (i1.type () == JSON::INTEGER);

		JSON::Value f1;
		ASSERT_TRUE (f1.type () == JSON::NIL);
		ASSERT_TRUE (f1.f () == 0);
		ASSERT_TRUE (f1.type () == JSON::FLOAT);

		JSON::Value s1;
		ASSERT_TRUE (s1.type () == JSON::NIL);
		ASSERT_TRUE (s1.s ().empty ());
		ASSERT_TRUE (s1.type () == JSON::STRING);

		JSON::Value o1;
		ASSERT_TRUE (o1.type () == JSON::NIL);
		ASSERT_TRUE (o1.o ().empty ());
		ASSERT_TRUE (o1.type () == JSON::OBJECT);

		JSON::Value a1;
		ASSERT_TRUE (a1.type () == JSON::NIL);
		ASSERT_TRUE (a1.a ().empty ());
		ASSERT_TRUE (a1.type () == JSON::ARRAY);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, get)
{
	try {
		JSON::Value v;
		v["integer"] = 0;
		v["float"] = 0.1;
		v["boolean"] = true;
		v["string"] = "test";
		v["array"].a ().push_back (100);
		v["object"]["elem"] = 0;

		const JSON::Value& cv = v;
		int i = cv.get<int> ("integer", 10);
		ASSERT_TRUE (i == 0);
		i = cv.get<int> ("integer_not_exist", 10);
		ASSERT_TRUE (i == 10);

		float f = cv.get<float> ("float", 10);
		ASSERT_TRUE (fabs (f - 0.1) < 1E-6);
		f = cv.get<float> ("float_not_exist", 10);
		ASSERT_TRUE (fabs (f - 10) < 1E-6);

		bool b = cv.get ("boolean", false);
		ASSERT_TRUE (b == true);
		b = cv.get ("boolean_not_exist", false);
		ASSERT_TRUE (b == false);

		string s_tmp ("test1");
		string s = cv.get ("string", s_tmp);
		ASSERT_TRUE (s == "test");
		s = cv.get ("string_not_exist", s_tmp);
		ASSERT_TRUE (s == s_tmp);

		JSON::Array a_tmp;
		a_tmp.push_back (0);
		JSON::Array a = cv.get ("array", a_tmp);
		ASSERT_TRUE (a.size () == 1);
		ASSERT_TRUE (a[0] == 100);
		a = cv.get ("array_not_exist", a_tmp);
		ASSERT_TRUE (a == a_tmp);

		JSON::Object o_tmp;
		o_tmp["elem"] = 100;
		JSON::Object o = cv.get ("object", o_tmp);
		ASSERT_TRUE (o.size () == 1);
		ASSERT_TRUE (o["elem"] == 0);
		o = cv.get ("object_not_exist", o_tmp);
		ASSERT_TRUE (o == o_tmp);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, get_with_casting)
{
	try {
		JSON::Value v;
		v["i"] = 1;
		v["f"] = 0.1;
		v["b"] = true;
		v["s_i"] = "1";
		v["s_f"] = "1.1";
		v["s_b_t"] = "true";
		v["s_b_t_1"] = "1";
		v["s_b_t_2"] = "0.1";
		v["s_b_f"] = "false";
		v["s_b_f_1"] = "0";
		v["s_b_f_2"] = "0.0";

		// 1. cast to integer
		ASSERT_TRUE (v.get<char> ("i", 0) == 1);
		ASSERT_TRUE (v.get<char> ("f", 1) == 0);
		ASSERT_TRUE (v.get<char> ("b", 0) == 1);
		ASSERT_TRUE (v.get<char> ("s_i", 0) == 1);
		ASSERT_TRUE (v.get<char> ("s_f", 0) == 1);
		ASSERT_TRUE (v.get<char> ("s_b_t", 0) == 1);
		ASSERT_TRUE (v.get<char> ("s_b_f", 1) == 0);

		ASSERT_TRUE (v.get<short> ("i", 0) == 1);
		ASSERT_TRUE (v.get<short> ("f", 1) == 0);
		ASSERT_TRUE (v.get<short> ("b", 0) == 1);
		ASSERT_TRUE (v.get<short> ("s_i", 0) == 1);
		ASSERT_TRUE (v.get<short> ("s_f", 0) == 1);
		ASSERT_TRUE (v.get<short> ("s_b_t", 0) == 1);
		ASSERT_TRUE (v.get<short> ("s_b_f", 1) == 0);

		ASSERT_TRUE (v.get<int> ("i", 0) == 1);
		ASSERT_TRUE (v.get<int> ("f", 1) == 0);
		ASSERT_TRUE (v.get<int> ("b", 0) == 1);
		ASSERT_TRUE (v.get<int> ("s_i", 0) == 1);
		ASSERT_TRUE (v.get<int> ("s_f", 0) == 1);
		ASSERT_TRUE (v.get<int> ("s_b_t", 0) == 1);
		ASSERT_TRUE (v.get<int> ("s_b_f", 1) == 0);

		ASSERT_TRUE (v.get<int64_t> ("i", 0) == 1);
		ASSERT_TRUE (v.get<int64_t> ("f", 1) == 0);
		ASSERT_TRUE (v.get<int64_t> ("b", 0) == 1);
		ASSERT_TRUE (v.get<int64_t> ("s_i", 0) == 1);
		ASSERT_TRUE (v.get<int64_t> ("s_f", 0) == 1);
		ASSERT_TRUE (v.get<int64_t> ("s_b_t", 0) == 1);
		ASSERT_TRUE (v.get<int64_t> ("s_b_f", 1) == 0);

		// 2. cast to float
		ASSERT_TRUE (v.get<float> ("i", 0) == 1);
		ASSERT_TRUE (fabs (v.get<float> ("f", 0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE (v.get<float> ("b", 0) == 1);
		ASSERT_TRUE (v.get<float> ("s_i", 0) == 1);
		ASSERT_TRUE (fabs (v.get<float> ("s_f", 0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE (v.get<float> ("s_b_t", 0) == 1);
		ASSERT_TRUE (v.get<float> ("s_b_f", 1) == 0);

		ASSERT_TRUE (v.get<double> ("i", 0) == 1);
		ASSERT_TRUE (fabs (v.get<double> ("f", 0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE (v.get<double> ("b", 0) == 1);
		ASSERT_TRUE (v.get<double> ("s_i", 0) == 1);
		ASSERT_TRUE (fabs (v.get<double> ("s_f", 0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE (v.get<double> ("s_b_t", 0) == 1);
		ASSERT_TRUE (v.get<double> ("s_b_f", 1) == 0);

		ASSERT_TRUE (v.get<long double> ("i", 0) == 1);
		ASSERT_TRUE (fabs (v.get<long double> ("f", 0) - 0.1) < JSON_EPSILON);
		ASSERT_TRUE (v.get<long double> ("b", 0) == 1);
		ASSERT_TRUE (v.get<long double> ("s_i", 0) == 1);
		ASSERT_TRUE (fabs (v.get<long double> ("s_f", 0) - 1.1) < JSON_EPSILON);
		ASSERT_TRUE (v.get<long double> ("s_b_t", 0) == 1);
		ASSERT_TRUE (v.get<long double> ("s_b_f", 1) == 0);

		// 3. cast to bool
		ASSERT_TRUE (v.get<bool> ("i", false) == true);
		ASSERT_TRUE (v.get<bool> ("f", true) == true);
		ASSERT_TRUE (v.get<bool> ("b", false) == true);
		ASSERT_TRUE (v.get<bool> ("s_i", false) == true);
		ASSERT_TRUE (v.get<bool> ("s_f", false) == true);
		ASSERT_TRUE (v.get<bool> ("s_b_t", false) == true);
		ASSERT_TRUE (v.get<bool> ("s_b_t_1", false) == true);
		ASSERT_TRUE (v.get<bool> ("s_b_t_2", false) == true);
		ASSERT_TRUE (v.get<bool> ("s_b_f", true) == false);
		ASSERT_TRUE (v.get<bool> ("s_b_f_1", true) == false);
		ASSERT_TRUE (v.get<bool> ("s_b_f_2", true) == false);

		// 4. cast to string
		string empty;
		ASSERT_TRUE (v.get<string> ("i", empty) == "1");
		ASSERT_TRUE (v.get<string> ("f", empty) == "0.1");
		ASSERT_TRUE (v.get<string> ("b", empty) == "true");
		ASSERT_TRUE (v.get<string> ("s_i", empty) == "1");
		ASSERT_TRUE (v.get<string> ("s_f", empty) == "1.1");
		ASSERT_TRUE (v.get<string> ("s_b_t", empty) == "true");
		ASSERT_TRUE (v.get<string> ("s_b_t_1", empty) == "1");
		ASSERT_TRUE (v.get<string> ("s_b_t_2", empty) == "0.1");
		ASSERT_TRUE (v.get<string> ("s_b_f", empty) == "false");
		ASSERT_TRUE (v.get<string> ("s_b_f_1", empty) == "0");
		ASSERT_TRUE (v.get<string> ("s_b_f_2", empty) == "0.0");

		JSON::Value v_e;
		v["s_i"] = "1x";
		v["s_f"] = "1.1x";
		v["s_b_t"] = "trux";
		v["s_b_f"] = "falsx";

		EXPECT_THROW (v.get<int> ("s_i", 0), std::logic_error);
		EXPECT_THROW (v.get<int> ("s_f", 0), std::logic_error);
		EXPECT_THROW (v.get<bool> ("s_b_t", true), std::logic_error);
		EXPECT_THROW (v.get<bool> ("s_b_f", true), std::logic_error);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, read_string)
{
	try {
		JSON::Value v;

		// 1. normal cases
		// case 1 no whitespace character on both sides
		ASSERT_TRUE (v.read_string ("\"abc\"", 5) == 5);
		ASSERT_TRUE (v.s () == "abc");

		// case 2 whitespace character before
		ASSERT_TRUE (v.read_string (" \r\n\t\f\b\"abc\"", 11) == 11);
		ASSERT_TRUE (v.s () == "abc");

		// case 3 escape
		string in ("\"\\\\\\/\\b\\f\\n\\r\\t\"");
		ASSERT_TRUE (v.read_string (in.c_str (), in.length ()) == in.length ());
		ASSERT_TRUE (v.s () == "\\/\b\f\n\r\t");

		// check escape string equal
		string out;
		v.write (out);
		ASSERT_TRUE (in == out);

		ASSERT_TRUE (v.read_string ("\"\\\"\"", 4) == 4);
		ASSERT_TRUE (v.s () == "\"");

		// case 4 test rewrite
		v = JSON_MOVE (string ("test"));
		ASSERT_TRUE (v.read_string ("\"abc\"", 5) == 5);
		ASSERT_TRUE (v.s () == "abc");
		v = JSON_MOVE (string ("test"));
		ASSERT_TRUE (v.read_string ("\"\b\"", 3) == 3);
		ASSERT_TRUE (v.s () == "\b");

		// case 5 decode as possible
		ASSERT_TRUE (v.read_string ("\"abc\" {} []", 11) == 5);
		ASSERT_TRUE (v.s () == "abc");

		// 2. exception cases
		// case 1 no left double quotation
		EXPECT_THROW (v.read_string ("abc", 3), std::logic_error);

		// case 2 no right double quotation
		EXPECT_THROW (v.read_string ("\"abc", 4), std::logic_error);
		EXPECT_THROW (v.read_string ("\"a\bc", 4), std::logic_error);
		EXPECT_THROW (v.read_string ("\"a\\\"\bc", 6), std::logic_error);

		// case 3 non-complete escape string
		EXPECT_THROW (v.read_string ("\"a\\", 6), std::logic_error);

		// case 4 unkonwn escape sequence
		EXPECT_THROW (v.read_string ("\"\\v\"", 4), std::logic_error);

		// case 5 all whitespace
		EXPECT_THROW (v.read_string (" \t\b\f\r\n", 6), std::logic_error);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, read_number)
{
	try {
		JSON::Value v;

		// 1. normal cases
		// case 1 zero
		ASSERT_TRUE (v.read_number ("0", 1) == 1);
		ASSERT_TRUE (v.i () == 0);
		ASSERT_TRUE (v.read_number ("0]", 2) == 1);
		ASSERT_TRUE (v.i () == 0);
		ASSERT_TRUE (v.read_number ("-0,", 3) == 2);
		ASSERT_TRUE (v.i () == 0);
		ASSERT_TRUE (v.read_number ("-0}", 3) == 2);
		ASSERT_TRUE (v.i () == 0);

		// case 2 normal integer number
		ASSERT_TRUE (v.read_number ("12345", 5) == 5);
		ASSERT_TRUE (v.i () == 12345);
		ASSERT_TRUE (v.read_number ("12345}", 6) == 5);
		ASSERT_TRUE (v.i () == 12345);
		ASSERT_TRUE (v.read_number ("-12345,", 7) == 6);
		ASSERT_TRUE (v.i () == -12345);
		ASSERT_TRUE (v.read_number ("-12345]", 7) == 6);
		ASSERT_TRUE (v.i () == -12345);

		// case 3 integer number limits
		ASSERT_TRUE (v.read_number ("2147483647", 10) == 10);
		ASSERT_TRUE (v.i () == INT_MAX);
		ASSERT_TRUE (v.read_number ("2147483647,", 11) == 10);
		ASSERT_TRUE (v.i () == INT_MAX);
		ASSERT_TRUE (v.read_number ("-2147483648}", 12) == 11);
		ASSERT_TRUE (v.i () == INT_MIN);
		ASSERT_TRUE (v.read_number ("9223372036854775807]", 20) == 19);
		ASSERT_TRUE (v.i () == LLONG_MAX);
		ASSERT_TRUE (v.read_number ("-9223372036854775808,", 21) == 20);
		ASSERT_TRUE (v.i () == LLONG_MIN);

		// case 4 floating number
		ASSERT_TRUE (v.read_number ("-0.0", 4) == 4);
		ASSERT_TRUE (v.f () == 0);
		ASSERT_TRUE (v.read_number ("-0.0,", 5) == 4);
		ASSERT_TRUE (v.f () == 0);
		ASSERT_TRUE (v.read_number ("-123.1e2", 8) == 8);
		ASSERT_TRUE (v.f () == -123.1e2);
		ASSERT_TRUE (v.read_number ("-3.123e-2", 9) == 9);
		ASSERT_TRUE (v.f () == -3.123e-2);

		// case 5 whitespace characters
		ASSERT_TRUE (v.read_number (" \b\f\r\n\t-3.123e-2", 15) == 15);
		ASSERT_TRUE (v.f () == -3.123e-2);
		ASSERT_TRUE (v.read_number (" \b\f\r\n\t-3.12 3e-2", 16) == 11);
		ASSERT_TRUE (v.f () == -3.12);

		// 2. exception cases
		// case 1 unexcept endings after integer number
		EXPECT_THROW (v.read_number ("0a", 2), std::logic_error);
		EXPECT_THROW (v.read_number ("-a", 2), std::logic_error);
		EXPECT_THROW (v.read_number ("-0.a", 4), std::logic_error);
		EXPECT_THROW (v.read_number ("-0.1a", 5), std::logic_error);
		EXPECT_THROW (v.read_number ("-0.1ea", 6), std::logic_error);
		EXPECT_THROW (v.read_number ("-0.1e+a", 7), std::logic_error);
		EXPECT_THROW (v.read_number ("-0.1e+1a", 8), std::logic_error);

		// case 2 unexpect zero-leading
		EXPECT_THROW (v.read_number ("00", 2), std::logic_error);
		EXPECT_THROW (v.read_number ("01", 2), std::logic_error);
		EXPECT_THROW (v.read_number ("01234", 5), std::logic_error);
		EXPECT_THROW (v.read_number ("-012.34", 7), std::logic_error);

		// case 3 no number after floating point
		EXPECT_THROW (v.read_number ("0.", 2), std::logic_error);
		EXPECT_THROW (v.read_number ("0.,", 2), std::logic_error);

		// case 4 no number after exp
		EXPECT_THROW (v.read_number ("0.1e", 4), std::logic_error);
		EXPECT_THROW (v.read_number ("0.1e+", 5), std::logic_error);

		// case 5 positive sign
		EXPECT_THROW (v.read_number ("+123", 4), std::logic_error);
		EXPECT_THROW (v.read_number ("+13.2", 5), std::logic_error);

		// case 6 all whitespace characters
		EXPECT_THROW (v.read_number (" \t\b\f\r\n", 6), std::logic_error);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, read_boolean)
{
	try {
		JSON::Value v;

		// 1. normal cases
		// case 1 
		ASSERT_TRUE (v.read_boolean ("true", 4) == 4);
		ASSERT_TRUE (v.b () == true);
		ASSERT_TRUE (v.read_boolean ("false", 5) == 5);
		ASSERT_TRUE (v.b () == false);

		// case 2 whitespace characters
		ASSERT_TRUE (v.read_boolean (" \b\f\r\n\ttrue", 10) == 10);
		ASSERT_TRUE (v.b () == true);
		ASSERT_TRUE (v.read_boolean (" \b\f\r\n\tfalse", 11) == 11);
		ASSERT_TRUE (v.b () == false);

		// 2. exception cases
		// case 1 insufficient data
		EXPECT_THROW (v.read_boolean ("tru", 3), std::logic_error);
		EXPECT_THROW (v.read_boolean ("fals", 4), std::logic_error);
		EXPECT_THROW (v.read_boolean (" \b\f\r\n\ttru", 9), std::logic_error);
		EXPECT_THROW (v.read_boolean (" \b\f\r\n\tfals", 10), std::logic_error);

		// case 2 typos
		EXPECT_THROW (v.read_boolean ("trua", 4), std::logic_error);
		EXPECT_THROW (v.read_boolean ("falsv", 5), std::logic_error);

		// case 3 capital error
		EXPECT_THROW (v.read_boolean ("True", 4), std::logic_error);
		EXPECT_THROW (v.read_boolean ("falsE", 5), std::logic_error);

		// case 4 all whitespace characters
		EXPECT_THROW (v.read_boolean (" \t\b\f\r\n", 6), std::logic_error);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, read_nil)
{
	try {
		JSON::Value v;

		// 1. normal cases
		// case 1 
		ASSERT_TRUE (v.read_nil ("null", 4) == 4);
		ASSERT_TRUE (v.type () == JSON::NIL);

		// case 2 whitespace characters
		ASSERT_TRUE (v.read_nil (" \b\f\r\n\tnull", 10) == 10);
		ASSERT_TRUE (v.type () == JSON::NIL);

		// 2. exception cases
		// case 1 insufficient data
		EXPECT_THROW (v.read_nil ("nul", 3), std::logic_error);
		EXPECT_THROW (v.read_nil (" \b\f\r\n\tnul", 9), std::logic_error);

		// case 2 typos
		EXPECT_THROW (v.read_nil ("nule", 4), std::logic_error);
		EXPECT_THROW (v.read_nil ("nill", 3), std::logic_error);

		// case 3 capital error
		EXPECT_THROW (v.read_nil ("Null", 4), std::logic_error);
		EXPECT_THROW (v.read_nil ("NULL", 4), std::logic_error);

		// case 4 all whitespace characters
		EXPECT_THROW (v.read_nil (" \t\b\f\r\n", 6), std::logic_error);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, benchmark_write)
{
	try {
		JSON::Object media;
		media.insert (make_pair (JSON_MOVE (string ("uri")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("http://javaone.com/keynote.mpg"))))));
		media.insert (make_pair (JSON_MOVE (string ("title")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("Javaone Keynote"))))));
		media.insert (make_pair (JSON_MOVE (string ("width")), JSON_MOVE (JSON::Value (640))));
		media.insert (make_pair (JSON_MOVE (string ("height")), JSON_MOVE (JSON::Value (480))));
		media.insert (make_pair (JSON_MOVE (string ("format")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("video/mpg4"))))));
		media.insert (make_pair (JSON_MOVE (string ("duration")), JSON_MOVE (JSON::Value (1800000))));
		media.insert (make_pair (JSON_MOVE (string ("size")), JSON_MOVE (JSON::Value (58982400))));
		media.insert (make_pair (JSON_MOVE (string ("bitrate")), JSON_MOVE (JSON::Value (262144))));

		JSON::Array persons;
		persons.push_back (JSON_MOVE (JSON::Value (JSON_MOVE (string ("Bill Gates")))));
		persons.push_back (JSON_MOVE (JSON::Value (JSON_MOVE (string ("Steve Jobs")))));
		media.insert (make_pair (JSON_MOVE (string ("persons")), JSON_MOVE (persons)));

		media.insert (make_pair (JSON_MOVE (string ("player")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("JAVA"))))));
		media.insert (make_pair (JSON_MOVE (string ("copyright")), JSON_MOVE (JSON::Value ())));

		JSON::Array images;
		images.push_back (JSON_MOVE (JSON::Value (JSON_MOVE (JSON::Object ()))));
		images.push_back (JSON_MOVE (JSON::Value (JSON_MOVE (JSON::Object ()))));

		JSON::Object& image1 = images[0].o ();
		image1.insert (make_pair (JSON_MOVE (string ("uri")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("http://javaone.com/keynote_large.jpg"))))));
		image1.insert (make_pair (JSON_MOVE (string ("title")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("Javaone Keynote"))))));
		image1.insert (make_pair (JSON_MOVE (string ("width")), JSON_MOVE (JSON::Value (1024))));
		image1.insert (make_pair (JSON_MOVE (string ("height")), JSON_MOVE (JSON::Value (768))));
		image1.insert (make_pair (JSON_MOVE (string ("size")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("LARGE"))))));

		JSON::Object& image2 = images[1].o ();
		image2.insert (make_pair (JSON_MOVE (string ("uri")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("http://javaone.com/keynote_small.jpg"))))));
		image2.insert (make_pair (JSON_MOVE (string ("title")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("Javaone Keynote"))))));
		image2.insert (make_pair (JSON_MOVE (string ("width")), JSON_MOVE (JSON::Value (320))));
		image2.insert (make_pair (JSON_MOVE (string ("height")), JSON_MOVE (JSON::Value (240))));
		image2.insert (make_pair (JSON_MOVE (string ("size")), JSON_MOVE (JSON::Value (JSON_MOVE (string ("SMALL"))))));

		JSON::Object o;
		o.insert (make_pair (JSON_MOVE (string ("media")), JSON_MOVE (media)));
		o.insert (make_pair (JSON_MOVE (string ("images")), JSON_MOVE (images)));

		string out;
		// reserve 512 bytes, reduce reallocation and copy cost of time
		out.reserve (512);
		JSON::Writer::write (o, out);
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, benchmark_read)
{
	try {
		string in ("{\"images\":[{\"height\":768,\"size\":\"LARGE\",\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote_large.jpg\",\"width\":1024},{\"height\":240,\"size\":\"SMALL\",\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote_small.jpg\",\"width\":320}],\"media\":{\"bitrate\":262144,\"copyright\":null,\"duration\":1800000,\"format\":\"video\\/mpg4\",\"height\":480,\"persons\":[\"Bill Gates\",\"Steve Jobs\"],\"player\":\"JAVA\",\"size\":58982400,\"title\":\"Javaone Keynote\",\"uri\":\"http:\\/\\/javaone.com\\/keynote.mpg\",\"width\":640}}");
		JSON::Value v;
		JSON::Reader::read (v, in.c_str (), in.length ());
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}

TEST (ut_xpjson, zh_cn)
{
	try {
		// UTF8 encoding
		string test = "测试中文";
		string chinese = '\"' + test + '\"';
		JSON::Value v;
		ASSERT_TRUE (v.read_string (chinese.c_str (), chinese.length ()) == chinese.length ());
		ASSERT_TRUE (v.s () == test);

		// after escape
		chinese = "\"\\u6d4b\\u8bd5\\u4e2d\\u6587\"";
		ASSERT_TRUE (v.read_string (chinese.c_str (), chinese.length ()) == chinese.length ());
		ASSERT_TRUE (v.s () == test);

		test = "test";
		chinese = test;
		v = JSON_MOVE (JSON::Object ());
		v.o ()["test"] = JSON_MOVE (JSON::Value (chinese));

		string out;
		JSON::Writer::write (v.o (), out);
		ASSERT_TRUE (out == "{\"test\":\"" + test + "\"}");

		JSON::Reader::read (v, out.c_str (), out.length ());
		ASSERT_TRUE (v.type () == JSON::OBJECT);
		ASSERT_TRUE (v.o ()["test"].s () == chinese);

		// exception cases
		// case 1 not 0-9A-F
		string in ("\"\\u75ag\"");
		EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);

		// case 2 insufficient data
		in = "\"\\u\"";
		EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);
		in = "\"\\u2\"";
		EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);
		in = "\"\\u32\"";
		EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);
		in = "\"\\u75a\"";
		EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);

		// UTF32(UCS-4) encoding under linux
		if (sizeof(wchar_t) == 4) {
			// normal case
			in = "\"\\ud84c\\udf50\"";
			ASSERT_TRUE (v.read_string (in.c_str (), in.length ()) == in.length ());
			ASSERT_TRUE (v.s () == "𣍐");

			out.clear ();
			v.write (out);
			ASSERT_TRUE (out == "\"𣍐\"");

			// exception cases
			// case 1 insufficient data
			in = "\"\\uD84C\"";
			EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);
			in = "\"\\uD84C\\uDF\"";
			EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);

			// case 2 not \u
			in = "\"\\uD84C/uDF50\"";
			EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);
			in = "\"\\uD84C\\xDF50\"";
			EXPECT_THROW (v.read_string (in.c_str (), in.length ()), std::logic_error);
		}
	}
	catch (std::exception &e) {
		printf ("Error : %s.", e.what ());
		ASSERT_TRUE (false);
	}
}
