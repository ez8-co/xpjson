/*
Copyright (c) 2010-2017 <http://ez8.co> <orca.zhang@yahoo.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#ifndef __XPJSON_HPP__
#define __XPJSON_HPP__

#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <cmath>
#include <cfloat>
#include <stdexcept>
#include <algorithm>
using namespace std;

// support redundant dangling comma like : [1,]  {"a":"b",}
#ifndef __XPJSON_SUPPORT_DANGLING_COMMA__
#	define __XPJSON_SUPPORT_DANGLING_COMMA__ 0
#endif

#if defined(__clang__)
#	ifndef __has_extension
#		define __has_extension __has_feature
#	endif
#	if __has_extension(__cxx_rvalue_references__)
#		define __XPJSON_SUPPORT_MOVE__
#	endif
#elif defined(__GNUC__)
#	if defined(_GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#		if (__GNUC__ * 1000 + __GNU_MINOR__) >= 4006
#			define __XPJSON_SUPPORT_MOVE__
#		endif
#	endif
#elif _MSC_VER >= 1600
#	define __XPJSON_SUPPORT_MOVE__
#endif

#ifdef _WIN32
#	if !defined(__MINGW32__) && !defined(__CYGWIN__)
		typedef signed __int64     int64_t;
		typedef unsigned __int64   uint64_t;
#	else /* other OS */
		typedef signed long long   int64_t;
		typedef unsigned long long uint64_t;
#	endif /* other OS */

#	define snprintf 	_snprintf
#	define strtoll		_strtoi64
#	define wcstoll		_wcstoi64

#	define PRId64       "I64d"
#	define LPRId64 		L"I64d"
#else
#	include <inttypes.h>
#	define LPRId64 		L"lld"
#endif

#ifdef _WIN32
// disable performance degradation warnings on casting from arithmetic type to bool
#	pragma warning(disable:4800)
// disable deprecated interface warnings
#	pragma warning(disable:4996)
#endif

#ifdef _WIN32
#	define JSON_TSTRING(type)			basic_string<type, char_traits<type>, allocator<type> >
#else
#	define JSON_TSTRING(type)			basic_string<type>
#endif

#define JSON_ASSERT_CHECK(expression, exception_type, what)	\
	if(!(expression)) {throw exception_type(what);}
#define JSON_ASSERT_CHECK1(expression, fmt, arg1)		\
	if(!(expression)) {char what[0x100] = {0}; sprintf(what, fmt"(line:%d)", arg1, __LINE__); throw std::logic_error(what);}
#define JSON_ASSERT_CHECK2(expression, fmt, arg1, arg2)	\
	if(!(expression)) {char what[0x100] = {0}; sprintf(what, fmt"(line:%d)", arg1, arg2, __LINE__); throw std::logic_error(what);}
#define JSON_CHECK_TYPE(type, except) JSON_ASSERT_CHECK2(type == except, "Type error: except(%s), actual(%s).", get_type_name(except), get_type_name(type))
#define JSON_PARSE_CHECK(expression)  JSON_ASSERT_CHECK2(expression, "Parse error: in=%.50s pos=%zd.", detail::get_cstr(in, len).c_str (), pos)
#define JSON_DECODE_CHECK(expression) JSON_ASSERT_CHECK1(expression, "Decode error: in=%.50s.", detail::get_cstr(in, len).c_str ())

#ifdef __XPJSON_SUPPORT_MOVE__
#	define JSON_MOVE(statement)		std::move(statement)
#else
#	define JSON_MOVE(statement)		(statement)
#endif

#define JSON_EPSILON				FLT_EPSILON

namespace JSON
{
	namespace detail
	{
		// type traits
		template<bool, typename T = void> struct json_enable_if   {};
		template<typename T> struct json_enable_if<true, T>       {typedef T type;};

		template<class T> struct json_remove_const                {typedef T type;};
		template<class T> struct json_remove_const<const T>       {typedef T type;};
		template<class T> struct json_remove_volatile             {typedef T type;};
		template<class T> struct json_remove_volatile<volatile T> {typedef T type;};
		template<class T> struct json_remove_cv                   {typedef typename json_remove_const<typename json_remove_volatile<T>::type>::type type;};

		struct json_true_type  {enum {value = true};};
		struct json_false_type {enum {value = false};};

		template<class T1, class T2> struct json_is_same_ :      json_false_type {};
		template<class T> struct json_is_same_<T, T> :           json_true_type  {};
		template<class T1, class T2> struct json_is_same :       json_is_same_<typename json_remove_cv<T1>::type, T2> {};

		template<class T> struct json_is_integral_ :             json_false_type {};
		template<> struct json_is_integral_<char> :              json_true_type  {};
		template<> struct json_is_integral_<unsigned char> :     json_true_type  {};
		template<> struct json_is_integral_<signed char> :       json_true_type  {};
#ifdef _NATIVE_WCHAR_T_DEFINED
		template<> struct json_is_integral_<wchar_t> :           json_true_type  {};
#endif /* _NATIVE_WCHAR_T_DEFINED */
		template<> struct json_is_integral_<unsigned short> :    json_true_type  {};
		template<> struct json_is_integral_<signed short> :      json_true_type  {};
		template<> struct json_is_integral_<unsigned int> :      json_true_type  {};
		template<> struct json_is_integral_<signed int> :        json_true_type  {};
#if (defined(__GNUC__) && !defined(__x86_64__)) || (defined(_WIN32) && !defined(_WIN64))
		template<> struct json_is_integral_<unsigned long> :     json_true_type  {};
		template<> struct json_is_integral_<signed long> :       json_true_type  {};
#endif
		template<> struct json_is_integral_<uint64_t> :          json_true_type  {};
		template<> struct json_is_integral_<int64_t> :           json_true_type  {};

		template<class T> struct json_is_floating_point_ :       json_false_type {};
		template<> struct json_is_floating_point_<float> :       json_true_type  {};
		template<> struct json_is_floating_point_<double> :      json_true_type  {};
		template<> struct json_is_floating_point_<long double> : json_true_type  {};

		template<class T> struct json_is_integral       {typedef typename json_remove_cv<T>::type type; enum {value = json_is_integral_<type>::value};};
		template<class T> struct json_is_floating_point {typedef typename json_remove_cv<T>::type type; enum {value = json_is_floating_point_<type>::value};};
		template<class T> struct json_is_arithmetic     {typedef typename json_remove_cv<T>::type type; enum {value = json_is_integral_<type>::value || json_is_same<type, bool>::value || json_is_floating_point_<type>::value};};
		// end of type traits

		template<class char_t> JSON_TSTRING(char) get_cstr(const char_t* str, size_t len);
		template<> JSON_TSTRING(char) get_cstr<char>(const char* str, size_t len) {return JSON_MOVE(JSON_TSTRING(char)(str, len));}
		template<> JSON_TSTRING(char) get_cstr<wchar_t>(const wchar_t* str, size_t len)
		{
			JSON_TSTRING(char) out;
			out.resize(len * sizeof(wchar_t));
			size_t l = 0;
			while(len--) l += wctomb(&out[l], *str++);
			out.resize(l);
			return JSON_MOVE(out);
		}
	}

	/** JSON type of a value. */
	enum Type
	{
		NIL,        // Null
		BOOLEAN,    // Boolean(true, false)
		INTEGER,    // Integer
		FLOAT,      // Float 3.14 12e-10
		STRING,     // String " ... "
		OBJECT,     // Object {...}
		ARRAY       // Array  [ ... ]
	};

	inline const char* get_type_name(int type);

	// Forward declaration
	template<class char_t>
	class ValueT;

	/** A JSON object, i.e., a container whose keys are strings, this
	is roughly equivalent to a Python dictionary, a PHP's associative
	array, a Perl or a C++ map(depending on the implementation). */
	template<class char_t>
	class ObjectT : public std::map<JSON_TSTRING(char_t), ValueT<char_t> > {};

	typedef ObjectT<char>    Object;
	typedef ObjectT<wchar_t> ObjectW;

	/** A JSON array, i.e., an indexed container of elements. It contains
	JSON values, that can have any of the types in ValueType. */
	template<class char_t>
	class ArrayT : public std::vector<ValueT<char_t> > {};

	typedef ArrayT<char>    Array;
	typedef ArrayT<wchar_t> ArrayW;

	/** A JSON value. Can have either type in ValueTypes. */
	template<class char_t>
	class ValueT
	{
	public:
		typedef JSON_TSTRING(char_t) tstring;

		/** Default constructor(type = NIL). */
		ValueT(void) : _type(NIL) {}
		/** Constructor with type. */
		ValueT(Type type);
		/** Copy constructor. */
		ValueT(const ValueT<char_t>& v);

		/** Constructor from bool. */
		ValueT(bool b) : _type(BOOLEAN), _b(b) {}
		/** Constructor from integer. */
#define JSON_INTEGER_CTOR(type)		ValueT(type i) : _type(INTEGER), _i(i) {}
		JSON_INTEGER_CTOR(unsigned char)
		JSON_INTEGER_CTOR(signed char)
#ifdef _NATIVE_WCHAR_T_DEFINED
		JSON_INTEGER_CTOR(wchar_t)
#endif /* _NATIVE_WCHAR_T_DEFINED */
		JSON_INTEGER_CTOR(unsigned short)
		JSON_INTEGER_CTOR(signed short)
		JSON_INTEGER_CTOR(unsigned int)
		JSON_INTEGER_CTOR(signed int)
#if (defined (__GNUC__) && !defined(__x86_64__)) || (defined(_WIN32) && !defined(_WIN64))
		JSON_INTEGER_CTOR(unsigned long)
		JSON_INTEGER_CTOR(signed long)
#endif
		JSON_INTEGER_CTOR(uint64_t)
		JSON_INTEGER_CTOR(int64_t)
#undef JSON_INTEGER_CTOR
		/** Constructor from float. */
#define JSON_FLOAT_CTOR(type)		ValueT(type f) : _type(FLOAT), _f(f) {}
		JSON_FLOAT_CTOR(float)
		JSON_FLOAT_CTOR(double)
		JSON_FLOAT_CTOR(long double)
#undef JSON_FLOAT_CTOR

		/** Constructor from pointer to char(C-string).  */
		ValueT(const char_t* s, bool escape = true) : _type(STRING), _e(escape), _s(0) {_s = new tstring(s);}
		/** Constructor from pointer to char(C-string).  */
		ValueT(const char_t* s, size_t l, bool escape = true) : _type(STRING), _e(escape), _s(0) {_s = new tstring(s, l);}
		/** Constructor from STD string  */
		ValueT(const tstring& s, bool escape = true) : _type(STRING), _e(escape), _s(0) {_s = new tstring(s);}
		/** Constructor from pointer to Object. */
		ValueT(const ObjectT<char_t>& o) : _type(OBJECT), _o(0) {_o = new ObjectT<char_t>(o);}
		/** Constructor from pointer to Array. */
		ValueT(const ArrayT<char_t>& a) : _type(ARRAY), _a(0) {_a = new ArrayT<char_t>(a);}
#ifdef __XPJSON_SUPPORT_MOVE__
		/** Move constructor. */
		ValueT(ValueT<char_t>&& v) : _type(v._type) {assign(JSON_MOVE(v));}
		/** Move constructor from STD string  */
		ValueT(tstring&& s, bool escape = true) : _type(STRING), _s(0), _e(escape) {_s = new tstring(JSON_MOVE(s));}
		/** Move constructor from pointer to Object. */
		ValueT(ObjectT<char_t>&& o) : _type(OBJECT), _o(0) {_o = new ObjectT<char_t>(JSON_MOVE(o));}
		/** Move constructor from pointer to Array. */
		ValueT(ArrayT<char_t>&& a) : _type(ARRAY), _a(0) {_a = new ArrayT<char_t>(JSON_MOVE(a));}
#endif

		~ValueT(void) {clear();}

		/** Assign function. */
		void assign(const ValueT<char_t>& v);
		/** Assign function from bool. */
		inline void assign(bool b) {clear(BOOLEAN); _b = b;}
		/** Assign function from integer. */
		template<class T>
		inline typename detail::json_enable_if<detail::json_is_integral<T>::value>::type
		assign(T i) {clear(INTEGER); _i = i;}
		/** Assign function from float. */
		template<class T>
		inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value>::type
		assign(T f) {clear(FLOAT); _f = f;}
		/** Assign function from pointer to char(C-string).  */
		inline void assign(const char_t* s, bool escape = true) {clear(STRING); _s->assign(s); _e = escape;}
		/** Assign function from pointer to char(C-string).  */
		inline void assign(const char_t* s, size_t l, bool escape = true) {clear(STRING); _s->assign(s, l); _e = escape;}
		/** Assign function from STD string  */
		inline void assign(const tstring& s, bool escape = true) {clear(STRING); *_s = s; _e = escape;}
		/** Assign function from pointer to Object. */
		inline void assign(const ObjectT<char_t>& o) {clear(OBJECT); *_o = o;}
		/** Assign function from pointer to Array. */
		inline void assign(const ArrayT<char_t>& a) {clear(ARRAY); *_a = a;}
#ifdef __XPJSON_SUPPORT_MOVE__
		/** Assign function. */
		void assign(ValueT<char_t>&& v);
 		// Fix: use swap rather than operator= to avoid bug under VS2010
		/** Assign function from STD string  */
		inline void assign(tstring&& s, bool escape = true) {clear(STRING); _s->clear(); _s->swap(JSON_MOVE(s)); _e = escape;}
		/** Assign function from pointer to Object. */
		inline void assign(ObjectT<char_t>&& o) {clear(OBJECT); _o->clear(); _o->swap(JSON_MOVE(o));}
		/** Assign function from pointer to Array. */
		inline void assign(ArrayT<char_t>&& a) {clear(ARRAY); _a->clear(); _a->swap(JSON_MOVE(a));}
#endif

		/** Assignment operator. */
		inline ValueT<char_t>& operator=(const ValueT<char_t>& v) {if(this != &v) {assign(v);} return *this;}
#define JSON_ASSIGNMENT(arg)		{assign(arg);return *this;}
		/** Assignment operator from int/float/bool. */
		template<class T>
		inline typename detail::json_enable_if<detail::json_is_arithmetic<T>::value, ValueT<char_t>&>::type
		operator=(T a) JSON_ASSIGNMENT(a)
		/** Assignment operator from pointer to char(C-string).  */
		inline ValueT<char_t>& operator=(const char_t* s) JSON_ASSIGNMENT(s)
		/** Assignment operator from STD string  */
		inline ValueT<char_t>& operator=(const tstring& s) JSON_ASSIGNMENT(s)
		/** Assignment operator from pointer to Object. */
		inline ValueT<char_t>& operator=(const ObjectT<char_t>& o) JSON_ASSIGNMENT(o)
		/** Assignment operator from pointer to Array. */
		inline ValueT<char_t>& operator=(const ArrayT<char_t>& a) JSON_ASSIGNMENT(a)
#ifdef __XPJSON_SUPPORT_MOVE__
		/** Assignment operator. */
		inline ValueT<char_t>& operator=(ValueT<char_t>&& v) {if(this != &v) {assign(JSON_MOVE(v));} return *this;}
		/** Assignment operator from STD string  */
		inline ValueT<char_t>& operator=(tstring&& s) JSON_ASSIGNMENT(JSON_MOVE(s))
		/** Assignment operator from pointer to Object. */
		inline ValueT<char_t>& operator=(ObjectT<char_t>&& o) JSON_ASSIGNMENT(JSON_MOVE(o))
		/** Assignment operator from pointer to Array. */
		inline ValueT<char_t>& operator=(ArrayT<char_t>&& a) JSON_ASSIGNMENT(JSON_MOVE(a))
#endif
#undef JSON_ASSIGNMENT

		/** Type query. */
		inline Type type(void) const {return _type;}

		/** Cast operator for bool */
		inline operator bool(void) const
		{
			JSON_CHECK_TYPE(_type, BOOLEAN);
			return _b;
		}
		/** Cast operator for integer */
#define JSON_INTEGER_OPERATOR(type)		\
	inline operator type(void) const {JSON_CHECK_TYPE(_type, INTEGER);return _i;}
		JSON_INTEGER_OPERATOR(unsigned char)
		JSON_INTEGER_OPERATOR(signed char)
#ifdef _NATIVE_WCHAR_T_DEFINED
		JSON_INTEGER_OPERATOR(wchar_t)
#endif /* _NATIVE_WCHAR_T_DEFINED */
		JSON_INTEGER_OPERATOR(unsigned short)
		JSON_INTEGER_OPERATOR(signed short)
		JSON_INTEGER_OPERATOR(unsigned int)
		JSON_INTEGER_OPERATOR(signed int)
#if (defined (__GNUC__) && !defined(__x86_64__)) || (defined(_WIN32) && !defined(_WIN64))
		JSON_INTEGER_OPERATOR(unsigned long)
		JSON_INTEGER_OPERATOR(signed long)
#endif
		JSON_INTEGER_OPERATOR(uint64_t)
		JSON_INTEGER_OPERATOR(int64_t)
#undef JSON_INTEGER_OPERATOR
		/** Cast operator for float */
#define JSON_FLOAT_OPERATOR(type)		\
	inline operator type(void) const {JSON_CHECK_TYPE(_type, FLOAT);return _f;}
		JSON_FLOAT_OPERATOR(float)
		JSON_FLOAT_OPERATOR(double)
		JSON_FLOAT_OPERATOR(long double)
#undef JSON_FLOAT_OPERATOR
		/** Cast operator for STD string */
		inline operator tstring(void) const
		{
			JSON_CHECK_TYPE(_type, STRING);
			return *_s;
		}
		/** Cast operator for Object */
		inline operator ObjectT<char_t>(void) const
		{
			JSON_CHECK_TYPE(_type, OBJECT);
			return *_o;
		}
		/** Cast operator for Array */
		inline operator ArrayT<char_t>(void) const
		{
			JSON_CHECK_TYPE(_type, ARRAY);
			return *_a;
		}

		/** Fetch boolean reference */
		inline bool& b(void)
		{
			if(_type == NIL) {_type = BOOLEAN; _b = false;}
			JSON_CHECK_TYPE(_type, BOOLEAN);
			return _b;
		}
		/** Fetch boolean value */
		inline bool b(void) const
		{
			JSON_CHECK_TYPE(_type, BOOLEAN);
			return _b;
		}
		/** Fetch integer reference*/
		inline int64_t& i(void)
		{
			if(_type == NIL) {_type = INTEGER; _i = 0;}
			JSON_CHECK_TYPE(_type, INTEGER);
			return _i;
		}
		/** Fetch integer value*/
		inline int64_t i(void) const
		{
			JSON_CHECK_TYPE(_type, INTEGER);
			return _i;
		}
		/** Fetch float reference */
		inline double& f(void)
		{
			if(_type == NIL) {_type = FLOAT; _f = 0;}
			JSON_CHECK_TYPE(_type, FLOAT);
			return _f;
		}
		/** Fetch float value */
		inline long double f(void) const
		{
			JSON_CHECK_TYPE(_type, FLOAT);
			return _f;
		}
		/** Fetch string reference */
		inline tstring& s(void)
		{
			if(_type == NIL) {_type = STRING; _s = new tstring;}
			_e = true; // the string may be modified by caller
			JSON_CHECK_TYPE(_type, STRING);
			return *_s;
		}
		/** Fetch string const-reference */
		inline const tstring& s(void) const
		{
			JSON_CHECK_TYPE(_type, STRING);
			return *_s;
		}
		/** Fetch object reference */
		inline ObjectT<char_t>& o(void)
		{
			if(_type == NIL) {_type = OBJECT; _o = new ObjectT<char_t>;}
			JSON_CHECK_TYPE(_type, OBJECT);
			return *_o;
		}
		/** Fetch object const-reference */
		inline const ObjectT<char_t>& o(void) const
		{
			JSON_CHECK_TYPE(_type, OBJECT);
			return *_o;
		}
		/** Fetch array reference */
		inline ArrayT<char_t>& a(void)
		{
			if(_type == NIL) {_type = ARRAY; _a = new ArrayT<char_t>;}
			JSON_CHECK_TYPE(_type, ARRAY);
			return *_a;
		}
		/** Fetch array const-reference */
		inline const ArrayT<char_t>& a(void) const
		{
			JSON_CHECK_TYPE(_type, ARRAY);
			return *_a;
		}
		/** Support [] operator for object. */
		inline ValueT<char_t>& operator[](const char_t* key)
		{
			if(_type == NIL) {_type = OBJECT; _o = new ObjectT<char_t>;}
			JSON_CHECK_TYPE(_type, OBJECT);
			return (*_o)[key];
		}
		/** Support [] operator for object. */
		inline ValueT<char_t>& operator[](const tstring& key)
		{
			if(_type == NIL) {_type = OBJECT; _o = new ObjectT<char_t>;}
			JSON_CHECK_TYPE(_type, OBJECT);
			return (*_o)[key];
		}
		/** Support [] operator for array. */
		template<class T>
		inline typename detail::json_enable_if<detail::json_is_integral<T>::value, ValueT<char_t>&>::type
		operator[](T pos)
		{
			if(_type == NIL) {_type = ARRAY; _a = new ArrayT<char_t>;}
			JSON_ASSERT_CHECK(pos >= 0, std::underflow_error, "Array index underflow");
			JSON_CHECK_TYPE(_type, ARRAY);
			if (pos >= _a->size()) _a->resize(pos + 1);
			return (*_a)[pos];
		}

		/** Support get value of key with elegant cast, return default_value if key not exist. */
		template<class T> T get(const tstring& key, const T& default_value) const;

		/** Clear current value. */
		void clear(Type	type = NIL);

		/** Write value to stream. */
		void write(tstring& out) const;

		/**
			Read object/array from stream.
			Return char_t count(offset) parsed.
			If error occurred, throws an exception.
		*/
		size_t read(const char_t* in, size_t len);

	protected:

		/**
			Read types from stream.
			Return char_t count(offset) parsed.
			If error occurred, throws an exception.
		*/
		size_t read_nil(const char_t* in, size_t len);
		size_t read_boolean(const char_t* in, size_t len);
		size_t read_number(const char_t* in, size_t len);
		/* NOTE: MUST with quotes.*/
		size_t read_string(const char_t* in, size_t len);

		/** Indicate current value type. */
		Type _type : 4;
		bool _e : 4; /* Used for string, indicates needs to be escaped or encoded. */
		char _[7]; /* padding */
		union {
			bool    _b;
			int64_t _i;
			double  _f;
			tstring        * _s;
			ObjectT<char_t>* _o;
			ArrayT<char_t> * _a;
		};
	};

	typedef ValueT<char>    Value;
	typedef ValueT<wchar_t> ValueW;

	template<class char_t>
	struct WriterT
	{
		static inline void write(const ValueT<char_t>& v, JSON_TSTRING(char_t)& out) {v.write(out);}
		static void write(const ObjectT<char_t>& o, JSON_TSTRING(char_t)& out);
		static void write(const ArrayT<char_t>& a, JSON_TSTRING(char_t)& out);
	};

	typedef WriterT<char>    Writer;
	typedef WriterT<wchar_t> WriterW;

	template<class char_t>
	struct ReaderT
	{
		static inline size_t read(ValueT<char_t>& v, const char_t* in, size_t len) {return v.read(in, len);}
	};

	typedef ReaderT<char>    Reader;
	typedef ReaderT<wchar_t> ReaderW;

	/* Compare functions */
	template<class char_t> bool operator==(const ObjectT<char_t>& lhs, const ObjectT<char_t>& rhs);
	template<class char_t> bool operator==(const ArrayT<char_t>& lhs, const ArrayT<char_t>& rhs);
	template<class char_t> bool operator==(const ValueT<char_t>& lhs, const ValueT<char_t>& rhs);

	template<class char_t> inline bool operator!=(const ObjectT<char_t>& lhs, const ObjectT<char_t>& rhs) {return !operator==(lhs, rhs);}
	template<class char_t> inline bool operator!=(const ArrayT<char_t>& lhs, const ArrayT<char_t>& rhs) {return !operator==(lhs, rhs);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& lhs, const ValueT<char_t>& rhs) {return !operator==(lhs, rhs);}

	template<class char_t> inline bool operator==(const ValueT<char_t>& v, bool b) {return v.type() == BOOLEAN && b == v.b();}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_integral<T>::value, bool>::type
	operator==(const ValueT<char_t>& v, T i) {return v.type() == INTEGER && i == v.i();}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value, bool>::type
	operator==(const ValueT<char_t>& v, T f) {return v.type() == FLOAT && fabs(f - v.f()) < JSON_EPSILON;}
	template<class char_t> inline bool operator==(const ValueT<char_t>& v, const JSON_TSTRING(char_t)& s) {return v.type() == STRING && s == v.s();}
	template<class char_t> inline bool operator==(const ValueT<char_t>& v, const ObjectT<char_t>& o) {return v.type() == OBJECT && o == v.o();}
	template<class char_t> inline bool operator==(const ValueT<char_t>& v, const ArrayT<char_t>& a) {return v.type() == ARRAY && a == v.a();}

	template<class char_t> inline bool operator==(bool b, const ValueT<char_t>& v) {return operator==(v, b);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_integral<T>::value, bool>::type
	operator==(T i, const ValueT<char_t>& v) {return operator==(v, i);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value, bool>::type
	operator==(T f, const ValueT<char_t>& v) {return operator==(v, f);}
	template<class char_t> inline bool operator==(const JSON_TSTRING(char_t)& s, const ValueT<char_t>& v) {return operator==(v, s);}
	template<class char_t> inline bool operator==(const ObjectT<char_t>& o, const ValueT<char_t>& v) {return operator==(v, o);}
	template<class char_t> inline bool operator==(const ArrayT<char_t>& a, const ValueT<char_t>& v) {return operator==(v, a);}

	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, bool b) {return !operator==(v, b);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_integral<T>::value, bool>::type
	operator!=(const ValueT<char_t>& v, T i) {return !operator==(v, i);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value, bool>::type
	operator!=(const ValueT<char_t>& v, T f) {return !operator==(v, f);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, const JSON_TSTRING(char_t)& s) {return !operator==(v, s);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, const ObjectT<char_t>& o) {return !operator==(v, o);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, const ArrayT<char_t>& a) {return !operator==(v, a);}

	template<class char_t> inline bool operator!=(bool b, const ValueT<char_t>& v) {return !operator==(v, b);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_integral<T>::value, bool>::type
	operator!=(T i, const ValueT<char_t>& v) {return !operator==(v, i);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value, bool>::type
	operator!=(T f, const ValueT<char_t>& v) {return !operator==(v, f);}
	template<class char_t> inline bool operator!=(const JSON_TSTRING(char_t)& s, const ValueT<char_t>& v) {return !operator==(v, s);}
	template<class char_t> inline bool operator!=(const ObjectT<char_t>& o, const ValueT<char_t>& v) {return !operator==(v, o);}
	template<class char_t> inline bool operator!=(const ArrayT<char_t>& a, const ValueT<char_t>& v) {return !operator==(v, a);}
}

namespace JSON
{
	const char* get_type_name(int type)
	{
		switch(type) {
			case NIL:     return "Null";
			case BOOLEAN: return "Boolean";
			case INTEGER: return "Integer";
			case FLOAT:   return "Floating";
			case STRING:  return "String";
			case OBJECT:  return "Object";
			case ARRAY:   return "Array";
		}
		return "Unknown";
	}

	template<class char_t>
	ValueT<char_t>::ValueT(Type type)
		: _type(type)
	{
		switch(_type) {
			case BOOLEAN: _b = false; break;
			case INTEGER: _i = 0;     break;
			case FLOAT:   _f = 0;     break;
			case STRING:  _e = true; _s = new tstring; break;
			case OBJECT:  _o = new ObjectT<char_t>;  break;
			case ARRAY:   _a = new ArrayT<char_t>;  break;
			default:      break;
		}
	}

	template<class char_t>
	ValueT<char_t>::ValueT(const ValueT<char_t>& v)
		: _type(v._type)
	{
		switch(_type) {
			case NIL:     _type = NIL; break;
			case BOOLEAN: _b = v._b;   break;
			case INTEGER: _i = v._i;   break;
			case FLOAT:   _f = v._f;   break;
			case STRING:  _e = v._e; _s = new tstring(*v._s); break;
			case OBJECT:  _o = new ObjectT<char_t>(*v._o); break;
			case ARRAY:   _a = new ArrayT<char_t>(*v._a); break;
		}
	}

	template<class char_t>
	void ValueT<char_t>::assign(const ValueT<char_t>& v)
	{
		clear(v._type);
		switch(_type) {
			case NIL:     _type = NIL; break;
			case BOOLEAN: _b = v._b;   break;
			case INTEGER: _i = v._i;   break;
			case FLOAT:   _f = v._f;   break;
			case STRING:  _e = v._e; *_s = *v._s; break;
			case OBJECT:  *_o = *v._o; break;
			case ARRAY:   *_a = *v._a; break;
		}
	}

#ifdef __XPJSON_SUPPORT_MOVE__
	template<class char_t>
	void ValueT<char_t>::assign(ValueT<char_t>&& v)
	{
		clear(v._type);
		switch(_type) {
			case NIL:     _type = NIL; break;
			case BOOLEAN: _b = v._b;   break;
			case INTEGER: _i = v._i;   break;
			case FLOAT:   _f = v._f;   break;
			case STRING:  _e = v._e; swap(_s, v._s); break;
			case OBJECT:  swap(_o, v._o); break;
			case ARRAY:   swap(_a, v._a); break;
		}
		v._type = NIL;
	}
#endif

	template<class char_t>
	void ValueT<char_t>::clear(Type	type /*= NIL*/)
	{
		if(_type != type) {
			switch(_type) {
				case STRING: delete _s; break;
				case OBJECT: delete _o; break;
				case ARRAY:  delete _a; break;
				default: break;
			}
			switch(type) {
				case STRING: _s = new tstring;         break;
				case OBJECT: _o = new ObjectT<char_t>; break;
				case ARRAY:  _a = new ArrayT<char_t>;  break;
				default: break;
			}
			_type = type;
		}
		else {
			switch(_type) {
				case STRING: _s->clear(); break;
				case OBJECT: _o->clear(); break;
				case ARRAY:  _a->clear(); break;
				default: break;
			}
		}
	}

	namespace detail
	{
		namespace
		{
			template<class T, class char_t>
			inline void internal_to_string(const T& v, JSON_TSTRING(char_t)& out, int(*fmter)(char_t*,size_t,const char_t*,...), const char_t* fmt)
			{
				// double 24 bytes, int64_t 20 bytes
				static const size_t bufSize = 25;
				const size_t len = out.length();
				out.resize(len + bufSize);
				int ret = fmter(&out[0] + len, bufSize, fmt, v);
				if(ret == bufSize || ret < 0) JSON_ASSERT_CHECK(false, std::runtime_error, "Format error.");
				out.resize(len + ret);
			}

#define JSON_TO_STRING(type, char_t, fmter, fmt) \
	template<> inline void to_string<type, char_t>(const type& v, JSON_TSTRING(char_t)& out) {internal_to_string<type, char_t>(v, out, fmter, fmt);}

			template<class T, class char_t>
			void to_string(const T& v, JSON_TSTRING(char_t)& out);

			JSON_TO_STRING(int64_t, char,    snprintf, "%" PRId64)
			JSON_TO_STRING(int64_t, wchar_t, swprintf, L"%" LPRId64)
			JSON_TO_STRING(double,  char,    snprintf, "%.16g")
			JSON_TO_STRING(double,  wchar_t, swprintf, L"%.16g")
#undef JSON_TO_STRING

			template<class T, class char_t>
			inline JSON_TSTRING(char_t) to_string(const T& v)
			{
				JSON_TSTRING(char_t) out;
				to_string(v, out);
				return JSON_MOVE(JSON_TSTRING(char_t)(out));
			}

			char int_to_hex(int n) {return n["0123456789abcdef"];}

			template<class char_t>
			void to_hex(int ch, JSON_TSTRING(char_t)& out)
			{
				out += int_to_hex((ch >> 4) & 0xF);
				out += int_to_hex(ch & 0xF);
			}

			template<int size, class char_t> void encode_unicode(char_t ch, JSON_TSTRING(char_t)& out);
			template<> void encode_unicode<1, char>(char ch, JSON_TSTRING(char)& out)
			{
				out += '\\'; out += 'u'; out += '0'; out += '0';
				to_hex(ch, out);
			}

			// For UTF16 Encoding
			template<> void encode_unicode<2, wchar_t>(wchar_t ch, JSON_TSTRING(wchar_t)& out)
			{
				out += '\\'; out += 'u';
				to_hex((ch >> 8) & 0xFF, out);
				to_hex(ch & 0xFF, out);
			}

			// For UTF32 Encoding
			template<> void encode_unicode<4, wchar_t>(wchar_t ch, JSON_TSTRING(wchar_t)& out)
			{
				if(ch > 0xFFFF) {
					ch = static_cast<int>(ch) - 0x10000;
					encode_unicode<2, wchar_t>(static_cast<unsigned short>(0xD800 |(ch >> 10)), out);
					encode_unicode<2, wchar_t>(static_cast<unsigned short>(0xDC00 |(ch & 0x03FF)), out);
				}
				else encode_unicode<2, wchar_t>(static_cast<unsigned short>(ch), out);
			}

			void encode(const char* in, size_t len, JSON_TSTRING(char)& out)
			{
				while(len--) {
					switch(*in) {
						case '\"': out += "\\\""; break;
						case '\\': out += "\\\\"; break;
						case '/':  out += "\\/";  break;
						case '\b': out += "\\b";  break;
						case '\f': out += "\\f";  break;
						case '\n': out += "\\n";  break;
						case '\r': out += "\\r";  break;
						case '\t': out += "\\t";  break;
						case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:
						case 11:case 14:case 15:case 16:case 17:case 18:case 19:
							encode_unicode<sizeof(char), char>(*in, out); break;
						default:   out += *in; break;
					}
					++in;
				}
			}

			void encode(const wchar_t* in, size_t len, JSON_TSTRING(wchar_t)& out)
			{
				while(len--) {
					if(*in > 0x7F) encode_unicode<sizeof(wchar_t), wchar_t>(*in, out);
					else {
						switch(*in) {
							case '\"': out += L"\\\""; break;
							case '\\': out += L"\\\\"; break;
							case '/':  out += L"\\/";  break;
							case '\b': out += L"\\b";  break;
							case '\f': out += L"\\f";  break;
							case '\n': out += L"\\n";  break;
							case '\r': out += L"\\r";  break;
							case '\t': out += L"\\t";  break;
							case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:
							case 11:case 14:case 15:case 16:case 17:case 18:case 19:
								encode_unicode<sizeof(wchar_t), wchar_t>(*in, out); break;
							default:   out += *in;     break;
						}
					}
					++in;
				}
			}

			int hex_to_int(int ch)
			{
				if('0' <= ch && ch <= '9') return (ch - '0');
				else if('a' <= ch && ch <= 'f') return (ch - 'a' + 10);
				else if('A' <= ch && ch <= 'F') return (ch - 'A' + 10);
				JSON_ASSERT_CHECK1(false, "Decode error: invalid character=0x%x.", ch);
			}

			template<class char_t>
			unsigned short hex_to_ushort(const char_t* in, size_t len)
			{
				JSON_DECODE_CHECK(len >= 4);
				unsigned char highByte = (hex_to_int(in[0]) << 4) | hex_to_int(in[1]);
				unsigned char lowByte = (hex_to_int(in[2]) << 4) | hex_to_int(in[3]);
				return (highByte << 8) | lowByte;
			}

			template<class char_t> void decode_unicode_append(unsigned int ui, JSON_TSTRING(char_t)& out);
			template<> void decode_unicode_append<wchar_t>(unsigned int ui, JSON_TSTRING(wchar_t)& out) {out += ui;}
			template<> void decode_unicode_append<char>(unsigned int ui, JSON_TSTRING(char)& out)
			{
				const size_t len = out.length();
				if(ui <= 0x0000007F) {
					out.resize(len + 1);
					out[len] = (ui & 0x7F);
				}
				else if(ui >= 0x00000080 && ui <= 0x000007FF) {
					out.resize(len + 2);
					out[len + 1] = (ui & 0x3F)        | 0x80;
					out[len    ] = ((ui >> 6) & 0x1F) | 0xC0;
				}
				else if(ui >= 0x00000800 && ui <= 0x0000FFFF) {
					out.resize(len + 3);
					out[len + 2] = (ui & 0x3F)         | 0x80;
					out[len + 1] = ((ui >>  6) & 0x3F) | 0x80;
					out[len    ] = ((ui >> 12) & 0x0F) | 0xE0;
				}
				else if(ui >= 0x00010000 && ui <= 0x001FFFFF) {
					out.resize(len + 4);
					out[len + 3] = (ui & 0x3F)         | 0x80;
					out[len + 2] = ((ui >>  6) & 0x3F) | 0x80;
					out[len + 1] = ((ui >> 12) & 0x3F) | 0x80;
					out[len    ] = ((ui >> 18) & 0x07) | 0xF0;
				}
				else if(ui >= 0x00200000 && ui <= 0x03FFFFFF) {
					out.resize(len + 5);
					out[len + 4] = (ui & 0x3F)         | 0x80;
					out[len + 3] = ((ui >>  6) & 0x3F) | 0x80;
					out[len + 2] = ((ui >> 12) & 0x3F) | 0x80;
					out[len + 1] = ((ui >> 18) & 0x3F) | 0x80;
					out[len    ] = ((ui >> 24) & 0x03) | 0xF8;
				}
				else if(ui >= 0x04000000 && ui <= 0x7FFFFFFF) {
					out.resize(len + 6);
					out[len + 5] = (ui & 0x3F)         | 0x80;
					out[len + 4] = ((ui >>  6) & 0x3F) | 0x80;
					out[len + 3] = ((ui >> 12) & 0x3F) | 0x80;
					out[len + 2] = ((ui >> 18) & 0x3F) | 0x80;
					out[len + 1] = ((ui >> 24) & 0x3F) | 0x80;
					out[len    ] = ((ui >> 30) & 0x01) | 0xFC;
				}
			}

			template<class char_t>
			size_t decode_unicode(const char_t* in, size_t len, JSON_TSTRING(char_t)& out)
			{
				unsigned int ui = hex_to_ushort(in, len);
				if(ui >= 0xD800 && ui < 0xDC00) {
					JSON_DECODE_CHECK(len >= 6 && in[4] == '\\' && in[5] == 'u');
					ui = (ui & 0x3FF) << 10;
					ui += (hex_to_ushort(in + 6, len - 6) & 0x3FF) + 0x10000;
					decode_unicode_append<char_t>(ui, out);
					return 10;
				}
				decode_unicode_append<char_t>(ui, out);
				return 4;
			}

			template<class char_t>
			void decode(const char_t* in, size_t len, JSON_TSTRING(char_t)& out)
			{
				for(size_t pos = 0; pos < len; ++pos) {
					switch(in[pos]) {
						case '\\':
							JSON_PARSE_CHECK(pos + 1 < len);
							++pos;
							switch(in[pos]) {
								case '\"': out += '\"'; break;
								case '\\': out += '\\'; break;
								case '/':  out += '/';  break;
								case 'b':  out += '\b'; break;
								case 'f':  out += '\f'; break;
								case 'n':  out += '\n'; break;
								case 'r':  out += '\r'; break;
								case 't':  out += '\t'; break;
								case 'u':  pos += decode_unicode<char_t>(in + pos + 1, len - pos - 1, out); break;
								default: JSON_PARSE_CHECK(false);
							}
							break;
						default: out += in[pos]; break;
					}
				}
			}

			template<class char_t> const char_t* boolean_true(void);
			template<> inline const char* boolean_true<char>(void) {return "true";}
			template<> inline const wchar_t* boolean_true<wchar_t>(void) {return L"true";}

			template<class char_t> inline size_t boolean_true_length(void) {return 4;}
			template<class char_t> inline size_t boolean_true_raw_length(void) {return 4 * sizeof(char_t);}

			template<class char_t> const char_t* boolean_false(void);
			template<> inline const char* boolean_false<char>(void) {return "false";}
			template<> inline const wchar_t* boolean_false<wchar_t>(void) {return L"false";}

			template<class char_t> inline size_t boolean_false_length(void) {return 5;}
			template<class char_t> inline size_t boolean_false_raw_length(void) {return 5 * sizeof(char_t);}

			template<class char_t> const char_t* nil_null(void);
			template<> inline const char* nil_null<char>(void) {return "null";}
			template<> inline const wchar_t* nil_null<wchar_t>(void) {return L"null";}

			template<class char_t> inline size_t nil_null_length(void) {return 4;}
			template<class char_t> inline size_t nil_null_raw_length(void) {return 4 * sizeof(char_t);}

			template<class char_t> int64_t ttoi64(const char_t* in, char_t** end);
			template<> int64_t ttoi64<char>(const char* in, char** end) {return strtoll(in, end, 10);}
			template<> int64_t ttoi64<wchar_t>(const wchar_t* in, wchar_t** end) {return wcstoll(in, end, 10);}

			template<class char_t> double ttod(const char_t* in, char_t** end);
			template<> double ttod<char>(const char* in, char** end) {return strtod(in, end);}
			template<> double ttod<wchar_t>(const wchar_t* in, wchar_t** end) {return wcstod(in, end);}

			template<class char_t> bool check_need_conv(char_t ch);
			template<> inline bool check_need_conv<char>(char ch) {return ch == '\\' || ch < 0x20;}
			template<> inline bool check_need_conv<wchar_t>(wchar_t ch) {return ch == '\\' || ch < 0x20 || ch > 0x7F;}

			/*
			 * May suffer performance degradation, use `find` or `count + []` then call o() to get reference instead.
			 */
			template<class char_t, class T>
			typename json_enable_if<json_is_same<ObjectT<char_t>, T>::value, T>::type
			internal_type_casting(const JSON::ValueT<char_t>& v, const T& value)
			{
				switch(v.type()) {
					case NIL:    break;
					case OBJECT: return T(v.o());
					default: JSON_ASSERT_CHECK1(false, "Type-casting error: from (%s) type to object.", get_type_name(v.type()));
				}
				return T(value);
			}

			/*
			 * May suffer performance degradation, use `find` or `count + []` then call a() to get reference instead.
			 */
			template<class char_t, class T>
			typename json_enable_if<json_is_same<ArrayT<char_t>, T>::value, T>::type
			internal_type_casting(const JSON::ValueT<char_t>& v, const T& value)
			{
				switch(v.type()) {
					case NIL:   break;
					case ARRAY: return T(v.a());
					default: JSON_ASSERT_CHECK1(false, "Type-casting error: from (%s) type to array.", get_type_name(v.type()));
				}
				return T(value);
			}

			template<class char_t, class T>
			typename json_enable_if<json_is_arithmetic<T>::value, T>::type
			internal_type_casting(const JSON::ValueT<char_t>& v, const T& value)
			{
				switch(v.type()) {
					case NIL:     break;
					case BOOLEAN: return T(v.b());
					case INTEGER: return T(v.i());
					case FLOAT:   return T(v.f());
					case STRING:
						if(v.s() == boolean_true<char_t>()) return T(1);
						else if(v.s() == boolean_false<char_t>()) return T(0);
						else {
							char_t* end = 0;
							double d = ttod(v.s().c_str(), &end);
							JSON_ASSERT_CHECK1(end == &v.s()[0] + v.s().length(), "Type-casting error: (%s) to arithmetic.", detail::get_cstr(v.s().c_str(), v.s().length()).c_str());
							return T(d);
						}
						JSON_ASSERT_CHECK1(false, "Type-casting error: (%s) to arithmetic.", detail::get_cstr(v.s().c_str(), v.s().length()).c_str());
					default: JSON_ASSERT_CHECK1(false, "Type-casting error: from (%s) type to arithmetic.", get_type_name(v.type()));
				}
				return T(value);
			}

			template<class char_t, class T>
			typename json_enable_if<json_is_same<JSON_TSTRING(char_t), T>::value, T>::type
			internal_type_casting(const JSON::ValueT<char_t>& v, const T& value)
			{
				switch(v.type()) {
					case NIL:     break;
					case BOOLEAN: return T(v.b() ? detail::boolean_true<char_t>() : detail::boolean_false<char_t>());
					case INTEGER: return to_string<int64_t, char_t>(v.i());
					case FLOAT:   return to_string<double, char_t>(v.f());
					case STRING:  return T(v.s());
					default: JSON_ASSERT_CHECK1(false, "Type-casting error: from (%s) type to string.", get_type_name(v.type()));
				}
				return T(value);
			}
		}
	}

	template<class char_t> template<class T>
	T JSON::ValueT<char_t>::get(const tstring& key, const T& default_value) const
	{
		JSON_CHECK_TYPE(_type, OBJECT);
		typename ObjectT<char_t>::const_iterator it = _o->find(key);
		if(it != _o->end()) return JSON_MOVE((detail::internal_type_casting <char_t, T>(it->second, default_value)));
		return T(default_value);
	}

	template<class char_t>
	void ValueT<char_t>::write(tstring& out) const
	{
		switch(_type) {
			case NIL:     out += detail::nil_null<char_t>(); break;
			case INTEGER: detail::to_string(_i, out);        break;
			case FLOAT:   detail::to_string(_f, out);        break;
			case OBJECT:  WriterT<char_t>::write(*_o, out);   break;
			case ARRAY:   WriterT<char_t>::write(*_a, out);   break;
			case BOOLEAN:
				out += (_b ? detail::boolean_true<char_t>() : detail::boolean_false<char_t>());
				break;
			case STRING:
				out += '\"';
				if(_e) detail::encode(_s->c_str(), _s->length(), out);
				else out += *_s;
				out += '\"';
				break;
		}
	}

#define case_white_space	case ' ':case '\b':case '\f':case '\n':case '\r':case '\t'
#define case_number_1_9		case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define case_number_0_9		case '0':case_number_1_9
#define case_number_ending	case_white_space: case ',':case ']':case '}'

	template<class char_t>
	size_t ValueT<char_t>::read_string(const char_t* in, size_t len)
	{
		enum {NONE = 0, NORMAL};
		unsigned char state = NONE;
		size_t pos = 0;
		size_t start = 0;
		clear(STRING);
		_e = false;
		while(pos < len) {
			switch(state) {
				case NONE:
					switch(in[pos]) {
						case '\"': state = NORMAL; start = pos + 1; break;
						case_white_space:                           break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case NORMAL:
					if(!_e) _e = detail::check_need_conv<char_t>(in[pos]);
					if(in[pos] == '\"' && in[pos - 1] != '\\') {
						if(_e) detail::decode(in + start, pos - start, *_s);
						else _s->assign(in + start, pos - start);
						return pos + 1;
					}
					break;
			}
			++pos;
		}
		JSON_PARSE_CHECK(false);
	}

	template<class char_t>
	size_t ValueT<char_t>::read_number(const char_t* in, size_t len)
	{
		enum {NONE = 0,SIGN, ZERO, DIGIT, POINT, DIGIT_FRAC, EXP, EXP_SIGN, DIGIT_EXP};
		unsigned char state = NONE;
		size_t pos = 0;
		size_t start = 0;
		while(pos < len) {
			switch(state) {
				case NONE:
					switch(in[pos]) {
						case '-':         state = SIGN;  start = pos; break;
						case '0':         state = ZERO;  start = pos; break;
						case_number_1_9:  state = DIGIT; start = pos; break;
						case_white_space:                             break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case SIGN:
					switch(in[pos]) {
						case '0':        state = ZERO;  break;
						case_number_1_9: state = DIGIT; break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case ZERO:
					JSON_PARSE_CHECK(in[pos] != '0');
				case DIGIT:
					if(!isdigit(in[pos]) || state != DIGIT) {
						switch(in[pos]) {
							case '.': state = POINT; break;
							case_number_ending:      goto GOTO_END;
							default: JSON_PARSE_CHECK(false);
						}
					}
					break;
				case POINT:
					switch(in[pos]) {
						case_number_0_9: state = DIGIT_FRAC; break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case DIGIT_FRAC:
					switch(in[pos]) {
						case_number_0_9:    state = DIGIT_FRAC; break;
						case 'e': case 'E': state = EXP;        break;
						case_number_ending:                     goto GOTO_END;
					}
					break;
				case EXP:
					switch(in[pos]) {
						case_number_0_9:    state = DIGIT_EXP; break;
						case '+': case '-': state = EXP_SIGN;  break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case EXP_SIGN:
					switch(in[pos]) {
						case_number_0_9: state = DIGIT_EXP; break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case DIGIT_EXP:
					switch(in[pos]) {
						case_number_0_9:    break;
						case_number_ending: goto GOTO_END;
					}
					break;
			}
			++pos;
		}
GOTO_END:
		char_t* end = 0;
		switch(state) {
			case ZERO:
			case DIGIT:
				clear(INTEGER);
				_i = detail::ttoi64<char_t>(in + start, &end);
				JSON_PARSE_CHECK(end == in + pos);
				return pos;
			case DIGIT_FRAC:
			case DIGIT_EXP:
				clear(FLOAT);
				_f = detail::ttod<char_t>(in + start, &end);
				JSON_PARSE_CHECK(end == in + pos);
				return pos;
		}
		JSON_PARSE_CHECK(false);
	}

	template<class char_t>
	size_t ValueT<char_t>::read_nil(const char_t* in, size_t len)
	{
		size_t pos = 0;
		while(pos < len) {
			switch(in[pos]) {
				case 'n':
					JSON_PARSE_CHECK(len - pos >= detail::nil_null_length<char_t>());
					if(memcmp(in + pos, detail::nil_null<char_t>(), detail::nil_null_raw_length<char_t>()) == 0) {
						clear();
						return pos + detail::nil_null_length<char_t>();
					}
					break;
				case_white_space: break;
				default: JSON_PARSE_CHECK(false);
			}
			++pos;
		}
		JSON_PARSE_CHECK(false);
	}

	template<class char_t>
	size_t ValueT<char_t>::read_boolean(const char_t* in, size_t len)
	{
		size_t pos = 0;
		while(pos < len) {
			switch(in[pos]) {
				case 't':
					JSON_PARSE_CHECK(len - pos >= detail::boolean_true_length<char_t>());
					if(memcmp(in + pos, detail::boolean_true<char_t>(), detail::boolean_true_raw_length<char_t>()) == 0) {
						clear(BOOLEAN);
						_b = true;
						return pos + detail::boolean_true_length<char_t>();
					}
					break;
				case 'f':
					JSON_PARSE_CHECK(len - pos >= detail::boolean_false_length<char_t>());
					if(memcmp(in + pos, detail::boolean_false<char_t>(), detail::boolean_false_raw_length<char_t>()) == 0) {
						clear(BOOLEAN);
						_b = false;
						return pos + detail::boolean_false_length<char_t>();
					}
					break;
				case_white_space: break;
				default: JSON_PARSE_CHECK(false);
			}
			++pos;
		}
		JSON_PARSE_CHECK(false);
	}

#define OBJECT_ARRAY_PARSE_END(type) {									\
		JSON_PARSE_CHECK(pv.back()->_type == type);						\
		pv.pop_back();													\
		if(pv.empty()) return pos + 1;/* Object/Array parse finished. */\
		if(pv.back()->_type == OBJECT) state = OBJECT_PAIR_VALUE;		\
		else if(pv.back()->_type == ARRAY) state = ARRAY_ELEM;			\
	}

#define PUSH_VALUE_TO_STACK(type)										\
	if(pv.back()->_type == NIL) pv.back()->clear(type);					\
	else {																\
		pv.back()->_a->push_back(JSON_MOVE(ValueT<char_t>(type)));		\
		pv.push_back(&pv.back()->_a->back());							\
	}

	template<class char_t>
	size_t ValueT<char_t>::read(const char_t* in, size_t len)
	{
		// Indicate current parse state
		enum {NONE = 0,
			OBJECT_LBRACE,          /* { */
			OBJECT_PAIR_KEY_QUOTE,  /* {"," */
			OBJECT_PAIR_KEY,        /* "..." */
			OBJECT_PAIR_COLON,      /* "...": */
			OBJECT_PAIR_VALUE,      /* "...":"..." */
			OBJECT_COMMA,           /* {..., */
			ARRAY_LBRACKET,         /* [ */
			ARRAY_ELEM,             /* [...[...,... */
			ARRAY_COMMA             /* [..., */
		};
		unsigned char state = NONE;
		size_t pos = 0;
		union {
			size_t start;
			size_t(ValueT<char_t>::*fp)(const char_t*, size_t);
		} u;
		memset(&u, 0, sizeof(u));
		vector<ValueT<char_t>*> pv(1, this);
		while(pos < len) {
			switch(state) {
				case NONE:
					// Topmost value parse.
					switch(in[pos]) {
						case '{': state = OBJECT_LBRACE;  clear(OBJECT); break;
						case '[': state = ARRAY_LBRACKET; clear(ARRAY);  break;
						case_white_space:                                break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case OBJECT_LBRACE:
				case OBJECT_COMMA:
					switch(in[pos]) {
						case '\"': state = OBJECT_PAIR_KEY_QUOTE; u.start = pos + 1; break;
#if __XPJSON_SUPPORT_DANGLING_COMMA__
						case '}': OBJECT_ARRAY_PARSE_END(OBJECT) break;
#else
						case '}':
							if(state == OBJECT_LBRACE) OBJECT_ARRAY_PARSE_END(OBJECT)
							else JSON_PARSE_CHECK(false);
							break;
#endif
						case_white_space: break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case OBJECT_PAIR_KEY_QUOTE:
					switch(in[pos]) {
						case '\\':
							while(++pos < len) {
								if(in[pos] == '\"' && in[pos - 1] != '\\') {
									state = OBJECT_PAIR_KEY;
									JSON_TSTRING(char_t) key;
									detail::decode(in + u.start, pos - u.start, key);
									pv.push_back(&(*pv.back()->_o)[JSON_MOVE(key)]);
									break;
								}
							}
							JSON_PARSE_CHECK(state == OBJECT_PAIR_KEY);
							break;
						case '\"':
							state = OBJECT_PAIR_KEY;
							// Insert a value
							pv.push_back(&(*pv.back()->_o)[JSON_MOVE(JSON_TSTRING(char_t)(in + u.start, pos - u.start))]);
							u.start = 0;
							break;
						default: break;
					}
					break;
				case OBJECT_PAIR_KEY:
					switch(in[pos]) {
						case ':': state = OBJECT_PAIR_COLON; break;
						case_white_space:                    break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case OBJECT_PAIR_COLON:
				case ARRAY_LBRACKET:
				case ARRAY_COMMA:
					switch(in[pos]) {
						case '\"':                 u.fp = &ValueT::read_string;       break;
						case '-': case_number_0_9: u.fp = &ValueT::read_number;       break;
						case 't': case 'f':        u.fp = &ValueT::read_boolean;      break;
						case 'n':                  u.fp = &ValueT::read_nil;          break;
						case '{': state = OBJECT_LBRACE;  PUSH_VALUE_TO_STACK(OBJECT) break;
						case '[': state = ARRAY_LBRACKET; PUSH_VALUE_TO_STACK(ARRAY)  break;
						case ']':
#if __XPJSON_SUPPORT_DANGLING_COMMA__
							if(state != OBJECT_PAIR_COLON)
#else
							if(state == ARRAY_LBRACKET)
#endif
							  OBJECT_ARRAY_PARSE_END(ARRAY)
							else JSON_PARSE_CHECK(false);
							break;
						case_white_space: break;
						default: JSON_PARSE_CHECK(false);
					}
					if(u.fp) {
						// If top elem is array, push a elem.
						if(pv.back()->_type == ARRAY) {
							pv.back()->_a->push_back(JSON_MOVE(ValueT<char_t>()));
							pv.push_back(&pv.back()->_a->back());
						}
						// ++pos at last, so minus 1 here.
						pos += (pv.back()->*u.fp)(in + pos, len - pos) - 1;
						u.fp = 0;
						// pop nil/number/boolean/string Value
						pv.pop_back();
						JSON_PARSE_CHECK(!pv.empty());
						switch(pv.back()->_type) {
							case OBJECT: state = OBJECT_PAIR_VALUE; break;
							case ARRAY:  state = ARRAY_ELEM;        break;
							default: JSON_PARSE_CHECK(false);
						}
					}
					break;
				case OBJECT_PAIR_VALUE:
					switch(in[pos]) {
						case '}': OBJECT_ARRAY_PARSE_END(OBJECT)  break;
						case ',': state = OBJECT_COMMA;           break;
						case_white_space:                         break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
				case ARRAY_ELEM:
					switch(in[pos]) {
						case ']': OBJECT_ARRAY_PARSE_END(ARRAY)  break;
						case ',': state = ARRAY_COMMA;           break;
						case_white_space:                        break;
						default: JSON_PARSE_CHECK(false);
					}
					break;
			}
			++pos;
		}
		JSON_PARSE_CHECK(false);
	}

#undef case_white_space
#undef case_number_1_9
#undef case_number_0_9
#undef case_number_ending
#undef OBJECT_ARRAY_PARSE_END
#undef PUSH_VALUE_TO_STACK

	template<class char_t>
	void WriterT<char_t>::write(const ObjectT<char_t>& o, JSON_TSTRING(char_t)& out)
	{
		out += '{';
		typename ObjectT<char_t>::const_iterator it = o.begin();
		typename ObjectT<char_t>::const_iterator it_end = o.end();
		while(it != it_end) {
			out += '\"';
			detail::encode(it->first.c_str(), it->first.length(), out);
			out += '\"';
			out += ':';
			it->second.write(out);
			out += ',';
			++it;
		}
		if(out[out.length() - 1] != '{') out[out.length() - 1] = '}'; else out += '}';
	}

	template<class char_t>
	void WriterT<char_t>::write(const ArrayT<char_t>& a, JSON_TSTRING(char_t)& out)
	{
		out += '[';
		size_t i = 0;
		while(i < a.size()) {
			a[i++].write(out);
			out += ',';
		}
		if(out[out.length() - 1] != '[') out[out.length() - 1] = ']'; else out += ']';
	}

	template<class char_t>
	bool operator==(const ObjectT<char_t>& lhs, const ObjectT<char_t>& rhs)
	{
		if(lhs.size() != rhs.size()) return false;
		typename ObjectT<char_t>::const_iterator lit = lhs.begin();
		typename ObjectT<char_t>::const_iterator rit = rhs.begin();
		for(; lit != lhs.end(); ++lit, ++rit) {
			if(lit->first != rit->first) return false;
			if(lit->second != rit->second) return false;
		}
		return true;
	}

	template<class char_t>
	bool operator==(const ArrayT<char_t>& lhs, const ArrayT<char_t>& rhs)
	{
		if(lhs.size() != rhs.size()) return false;
		for(size_t i = 0; i < lhs.size(); ++i) {
			if(lhs[i] != rhs[i]) return false;
		}
		return true;
	}

	template<class char_t>
	bool operator==(const ValueT<char_t>& lhs, const ValueT<char_t>& rhs)
	{
		if(lhs.type() != rhs.type()) return false;
		switch(lhs.type()) {
			case NIL:     return true;
			case BOOLEAN: return lhs.b() == rhs.b();
			case INTEGER: return lhs.i() == rhs.i();
			case FLOAT:   return fabs(lhs.f() - rhs.f()) < JSON_EPSILON;
			case STRING:  return lhs.s() == rhs.s();
			case OBJECT:  return lhs.o() == rhs.o();
			case ARRAY:   return lhs.a() == rhs.a();
		}
		return true;
	}
}

#endif // __XPJSON_HPP__
