/*
  xpjson -- A minimal Xross-Platform JSON read & write library in C++.

  Copyright (c) 2010-2017 <http://ez8.co> <orca.zhang@yahoo.com>
  This library is released under the MIT License.

  Please see LICENSE file or visit https://github.com/ez8-co/xpjson for details.
 */
#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <deque>
#include <map>
#include <cmath>
#include <cfloat>
#include <stdexcept>
#include <algorithm>

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
#		if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40600
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

#	define PRId64       "I64d"
#	define LPRId64 		L"I64d"
#else
#	include <inttypes.h>
#	define LPRId64 		L"lld"
#endif

#ifdef _MSC_VER
// disable performance degradation warnings on casting from arithmetic type to bool
#	pragma warning(disable:4800)
// disable deprecated interface warnings
#	pragma warning(disable:4996)
#	define XPJSON_LIKELY(x)         (x)
#	define XPJSON_UNLIKELY(x)       (x)
#else
#	define XPJSON_LIKELY(x)         (__builtin_expect((x), 1))
#	define XPJSON_UNLIKELY(x)       (__builtin_expect((x), 0))
#endif

#ifdef _WIN32
#	define JSON_TSTRING(type)			std::basic_string<type, std::char_traits<type>, std::allocator<type> >
#else
#	define JSON_TSTRING(type)			std::basic_string<type>
#endif

#define JSON_ASSERT_CHECK(expression, exception_type, what)	\
	if(XPJSON_UNLIKELY(!(expression))) {throw exception_type(what);}
#define JSON_ASSERT_CHECK1(expression, fmt, arg1)		\
	if(XPJSON_UNLIKELY(!(expression))) {char what[0x100] = {0}; sprintf(what, fmt"(line:%d)", arg1, __LINE__); throw std::logic_error(what);}
#define JSON_ASSERT_CHECK2(expression, fmt, arg1, arg2)	\
	if(XPJSON_UNLIKELY(!(expression))) {char what[0x100] = {0}; sprintf(what, fmt"(line:%d)", arg1, arg2, __LINE__); throw std::logic_error(what);}
#define JSON_CHECK_TYPE(type, except) JSON_ASSERT_CHECK2(type == except, "Type error: except(%s), actual(%s).", get_type_name(except), get_type_name(type))
#define JSON_PARSE_CHECK(expression)  JSON_ASSERT_CHECK2(expression, "Parse error: in=%.50s pos=%zu.", detail::get_cstr(in, len).c_str (), pos)
#define JSON_DECODE_CHECK(expression) JSON_ASSERT_CHECK1(expression, "Decode error: in=%.50s.", detail::get_cstr(in, len).c_str ())

#ifdef __XPJSON_SUPPORT_MOVE__
#	define JSON_MOVE(statement)		std::move(statement)
#else
#	define JSON_MOVE(statement)		(statement)
#endif

#define JSON_EPSILON				FLT_EPSILON

typedef enum ESCAPE_TYPE {
	AUTO_DETECT = -1,
	DONT_ESCAPE = 0,
	NEED_ESCAPE = 1
} ESCAPE_TYPE;

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
		template<> inline JSON_TSTRING(char) get_cstr<char>(const char* str, size_t len) {return JSON_MOVE(JSON_TSTRING(char)(str, len));}
		template<> inline JSON_TSTRING(char) get_cstr<wchar_t>(const wchar_t* str, size_t len)
		{
			JSON_TSTRING(char) out;
			out.resize(len * sizeof(wchar_t));
			size_t l = 0;
			while(len--) l += wctomb(&out[l], *str++);
			out.resize(l);
			return JSON_MOVE(out);
		}

		template<class char_t> size_t tcslen(const char_t* str);
		template<> inline size_t tcslen<char>(const char* str) {return strlen(str);}
		template<> inline size_t tcslen<wchar_t>(const wchar_t* str) {return wcslen(str);}

		template<class char_t> int tcsncmp(const char_t* t, const char_t* s, size_t n);
		template<> inline int tcsncmp<char>(const char* t, const char* s, size_t n) {return strncmp(t, s, n);}
		template<> inline int tcsncmp<wchar_t>(const wchar_t* t, const wchar_t* s, size_t n) {return wcsncmp(t, s, n);}

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
template<> inline void to_string<type, char_t>(type v, JSON_TSTRING(char_t)& out) {internal_to_string<type, char_t>(v, out, fmter, fmt);}

		template<class T, class char_t>
		void to_string(T v, JSON_TSTRING(char_t)& out);

		JSON_TO_STRING(int64_t, char,    snprintf, "%" PRId64)
		JSON_TO_STRING(int64_t, wchar_t, swprintf, L"%" LPRId64)
		JSON_TO_STRING(double,  char,    snprintf, "%.16g")
		JSON_TO_STRING(double,  wchar_t, swprintf, L"%.16g")
#undef JSON_TO_STRING

		template<> inline void to_string<const char*, char>(const char* v, JSON_TSTRING(char)& out) { out += v; }

		template<> inline void to_string<const char*, wchar_t>(const char* v, JSON_TSTRING(wchar_t)& out) { while(*v) out += (wchar_t)*v++; }

		template<class T, class char_t>
		inline JSON_TSTRING(char_t) to_string(const T& v)
		{
			JSON_TSTRING(char_t) out;
			to_string(v, out);
			return JSON_MOVE(JSON_TSTRING(char_t)(out));
		}

		inline char int_to_hex(int n) {return n["0123456789abcdef"];}

		template<class char_t>
		inline void to_hex(int ch, JSON_TSTRING(char_t)& out)
		{
			out += int_to_hex((ch >> 4) & 0xF);
			out += int_to_hex(ch & 0xF);
		}

		template<int size, class char_t> void encode_unicode(char_t ch, JSON_TSTRING(char_t)& out);
		template<> inline void encode_unicode<1, char>(char ch, JSON_TSTRING(char)& out)
		{
			out += '\\'; out += 'u'; out += '0'; out += '0';
			to_hex(ch, out);
		}

		// For UTF16 Encoding
		template<> inline void encode_unicode<2, wchar_t>(wchar_t ch, JSON_TSTRING(wchar_t)& out)
		{
			out += '\\'; out += 'u';
			to_hex((ch >> 8) & 0xFF, out);
			to_hex(ch & 0xFF, out);
		}

		// For UTF32 Encoding
		template<> inline void encode_unicode<4, wchar_t>(wchar_t ch, JSON_TSTRING(wchar_t)& out)
		{
			if(ch > 0xFFFF) {
				ch = static_cast<int>(ch) - 0x10000;
				encode_unicode<2, wchar_t>(static_cast<unsigned short>(0xD800 |(ch >> 10)), out);
				encode_unicode<2, wchar_t>(static_cast<unsigned short>(0xDC00 |(ch & 0x03FF)), out);
			}
			else encode_unicode<2, wchar_t>(static_cast<unsigned short>(ch), out);
		}

		inline void encode(const char* in, size_t len, JSON_TSTRING(char)& out)
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
					default:
						if (unsigned(*in) < ' ')
							encode_unicode<sizeof(char), char>(*in, out);
						else
							out += *in;
						break;
				}
				++in;
			}
		}

		inline void encode(const wchar_t* in, size_t len, JSON_TSTRING(wchar_t)& out)
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
						default:
							if (unsigned(*in) < ' ')
								encode_unicode<sizeof(wchar_t), wchar_t>(*in, out);
							else
								out += *in;
							break;
					}
				}
				++in;
			}
		}

		inline int hex_to_int(int ch)
		{
			if('0' <= ch && ch <= '9') return (ch - '0');
			else if('a' <= ch && ch <= 'f') return (ch - 'a' + 10);
			else if('A' <= ch && ch <= 'F') return (ch - 'A' + 10);
			JSON_ASSERT_CHECK1(false, "Decode error: invalid character=0x%x.", ch);
		}

		template<class char_t>
		inline unsigned short hex_to_ushort(const char_t* in, size_t len)
		{
			JSON_DECODE_CHECK(len >= 4);
			unsigned char highByte = (hex_to_int(in[0]) << 4) | hex_to_int(in[1]);
			unsigned char lowByte = (hex_to_int(in[2]) << 4) | hex_to_int(in[3]);
			return (highByte << 8) | lowByte;
		}

		template<class char_t> void decode_unicode_append(unsigned int ui, JSON_TSTRING(char_t)& out);
		template<> inline void decode_unicode_append<wchar_t>(unsigned int ui, JSON_TSTRING(wchar_t)& out) {out += ui;}
		template<> inline void decode_unicode_append<char>(unsigned int ui, JSON_TSTRING(char)& out)
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
		inline size_t decode_unicode(const char_t* in, size_t len, JSON_TSTRING(char_t)& out)
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
		inline void decode(const char_t* in, size_t len, JSON_TSTRING(char_t)& out)
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

		template<class char_t> bool check_need_conv(char_t ch);
		template<> inline bool check_need_conv<char>(char ch) {return ch < 0x20 || ch == '\\' || ch == '\"' || ch == '/';}
		template<> inline bool check_need_conv<wchar_t>(wchar_t ch) {return ch < 0x20 || ch > 0x7F || ch == '\\' || ch == '\"' || ch == '/';}
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
	class ArrayT : public std::deque<ValueT<char_t> > {};

	typedef ArrayT<char>    Array;
	typedef ArrayT<wchar_t> ArrayW;

	/** A JSON value. Can have either type in ValueTypes. */
	template<class char_t>
	class ValueT
	{
	public:
		typedef JSON_TSTRING(char_t) tstring;

		/** Default constructor(type = NIL). */
		ValueT() : _type(NIL) {}
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
		ValueT(const char_t* s, int escape = AUTO_DETECT, bool cow = false) : _type(NIL) {assign(s, detail::tcslen(s), escape, cow);}
		/** Constructor from pointer to char(C-string).  */
		ValueT(const char_t* s, size_t l, int escape = AUTO_DETECT, bool cow = false) : _type(NIL) {assign(s, l, escape, cow);}
		/** Constructor from STD string  */
		ValueT(const tstring& s, int escape = AUTO_DETECT, bool cow = false) : _type(NIL) {assign(s.data(), s.size(), escape, cow);}
		/** Constructor from pointer to Object. */
		ValueT(const ObjectT<char_t>& o) : _type(OBJECT), _o(0) {_o = new ObjectT<char_t>(o);}
		/** Constructor from pointer to Array. */
		ValueT(const ArrayT<char_t>& a) : _type(ARRAY), _a(0) {_a = new ArrayT<char_t>(a);}
#ifdef __XPJSON_SUPPORT_MOVE__
		/** Move constructor. */
		ValueT(ValueT<char_t>&& v) : _type(NIL) {assign(JSON_MOVE(v));}
		/** Move constructor from STD string  */
		ValueT(tstring&& s, int escape = AUTO_DETECT) : _type(NIL) {assign(JSON_MOVE(s), escape);}
		/** Move constructor from pointer to Object. */
		ValueT(ObjectT<char_t>&& o) : _type(OBJECT), _o(0) {_o = new ObjectT<char_t>(JSON_MOVE(o));}
		/** Move constructor from pointer to Array. */
		ValueT(ArrayT<char_t>&& a) : _type(ARRAY), _a(0) {_a = new ArrayT<char_t>(JSON_MOVE(a));}
#endif

		~ValueT() {clear();}

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
		inline void assign(const char_t* s, int escape = AUTO_DETECT, bool cow = false) {assign(s, detail::tcslen(s), escape, cow);}
		/** Assign function from pointer to char(C-string).  */
		inline void assign(const char_t* s, size_t l, int escape = AUTO_DETECT, bool cow = false);
		/** Assign function from STD string  */
		inline void assign(const tstring& s, int escape = AUTO_DETECT, bool cow = false) {assign(s.data(), s.size(), escape, cow);}
		/** Assign function from pointer to Object. */
		inline void assign(const ObjectT<char_t>& o) {clear(OBJECT); *_o = o;}
		/** Assign function from pointer to Array. */
		inline void assign(const ArrayT<char_t>& a) {clear(ARRAY); *_a = a;}
#ifdef __XPJSON_SUPPORT_MOVE__
		/** Assign function. */
		void assign(ValueT<char_t>&& v);
		/** Assign function from STD string  */
		inline void assign(tstring&& s, int escape = AUTO_DETECT);
		/** Assign function from pointer to Object. */
		inline void assign(ObjectT<char_t>&& o) {clear(OBJECT); *_o = JSON_MOVE(o);}
		/** Assign function from pointer to Array. */
		inline void assign(ArrayT<char_t>&& a) {clear(ARRAY); *_a = JSON_MOVE(a);}
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
		inline Type type() const {return (Type)_type;}

		/** Cast operator for bool */
		inline operator bool() const
		{
			JSON_CHECK_TYPE(_type, BOOLEAN);
			return _b;
		}
		/** Cast operator for integer */
#define JSON_INTEGER_OPERATOR(type)		\
	inline operator type() const {JSON_CHECK_TYPE(_type, INTEGER);return _i;}
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
	inline operator type() const {JSON_CHECK_TYPE(_type, FLOAT);return _f;}
		JSON_FLOAT_OPERATOR(float)
		JSON_FLOAT_OPERATOR(double)
		JSON_FLOAT_OPERATOR(long double)
#undef JSON_FLOAT_OPERATOR
		/** Cast operator for STD string */
		inline operator tstring() const
		{
			JSON_CHECK_TYPE(_type, STRING);
			if(_sso || _cow){
				_s = new tstring(c_str(), length());
				_sso = _cow = false;
			}
			return *_s;
		}
		/** Cast operator for Object */
		inline operator ObjectT<char_t>() const
		{
			JSON_CHECK_TYPE(_type, OBJECT);
			return *_o;
		}
		/** Cast operator for Array */
		inline operator ArrayT<char_t>() const
		{
			JSON_CHECK_TYPE(_type, ARRAY);
			return *_a;
		}

		/** Fetch boolean reference */
		inline bool& b()
		{
			if(_type == NIL) {_type = BOOLEAN; _b = false;}
			JSON_CHECK_TYPE(_type, BOOLEAN);
			return _b;
		}
		/** Fetch boolean value */
		inline bool b() const
		{
			JSON_CHECK_TYPE(_type, BOOLEAN);
			return _b;
		}
		/** Fetch integer reference*/
		inline int64_t& i()
		{
			if(_type == NIL) {_type = INTEGER; _i = 0;}
			JSON_CHECK_TYPE(_type, INTEGER);
			return _i;
		}
		/** Fetch integer value*/
		inline int64_t i() const
		{
			JSON_CHECK_TYPE(_type, INTEGER);
			return _i;
		}
		/** Fetch float reference */
		inline double& f()
		{
			if(_type == NIL) {_type = FLOAT; _f = 0;}
			JSON_CHECK_TYPE(_type, FLOAT);
			return _f;
		}
		/** Fetch float value */
		inline long double f() const
		{
			JSON_CHECK_TYPE(_type, FLOAT);
			return _f;
		}
		/** Fetch string reference */
		inline tstring& s()
		{
			if(_type == NIL) {_type = STRING; _sso = _cow = false; _s = new tstring;}
			JSON_CHECK_TYPE(_type, STRING);
			if(_sso || _cow){
				_s = new tstring(c_str(), length());
				_sso = _cow = false;
			}
			_e = true; // the string may be modified by caller
			return *_s;
		}
		/** Fetch string const-reference */
		inline const tstring& s() const
		{
			JSON_CHECK_TYPE(_type, STRING);
			if(_sso || _cow){
				_s = new tstring(c_str(), length());
				_sso = _cow = false;
			}
			return *_s;
		}
		/** Fetch object reference */
		inline ObjectT<char_t>& o()
		{
			if(_type == NIL) {_type = OBJECT; _o = new ObjectT<char_t>;}
			JSON_CHECK_TYPE(_type, OBJECT);
			return *_o;
		}
		/** Fetch object const-reference */
		inline const ObjectT<char_t>& o() const
		{
			JSON_CHECK_TYPE(_type, OBJECT);
			return *_o;
		}
		/** Fetch array reference */
		inline ArrayT<char_t>& a()
		{
			if(_type == NIL) {_type = ARRAY; _a = new ArrayT<char_t>;}
			JSON_CHECK_TYPE(_type, ARRAY);
			return *_a;
		}
		/** Fetch array const-reference */
		inline const ArrayT<char_t>& a() const
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

		/** Support get value with elegant cast. */
		template<class T> T get(const T& default_value) const;

		/** Support get value of key with elegant cast, return default_value if key not exist. */
		template<class T> T get(const tstring& key, const T& default_value) const;

		/** Clear current value. */
		void clear(unsigned char type = NIL);

		/** Write value to stream. */
		void write(tstring& out) const;

		void to_string(tstring& out) const;

		/**
			Read object/array from stream.
			Return char_t count(offset) parsed.
			If error occurred, throws an exception.
		*/
		size_t read(const char_t* in, size_t len, bool cow = false);
		size_t read(const char_t* in, bool cow = false)
		{
			return read(in, detail::tcslen(in), cow);
		}
		size_t read(const tstring& in, bool cow = false)
		{
			return read(in.data(), in.size(), cow);
		}

		const char_t* c_str() const
		{
			JSON_CHECK_TYPE(_type, STRING);
			if(_sso)
				return reinterpret_cast<const char_t*>(_sso_s);
			else if (_cow)
				return _d;
			else
				return _s->c_str();
		}

		size_t length() const
		{
			JSON_CHECK_TYPE(_type, STRING);
			if(_sso)
				return _sso_len;
			else if (_cow)
				return _cow_len;
			else
				return _s->length();
		}

		/**
			Read types from stream.
			Return char_t count(offset) parsed.
			If error occurred, throws an exception.
		*/
		size_t read_nil(const char_t* in, size_t len, bool cow = false);
		size_t read_boolean(const char_t* in, size_t len, bool cow = false);
		size_t read_number(const char_t* in, size_t len, bool cow = false);
		/* NOTE: MUST with quotes.*/
		size_t read_string(const char_t* in, size_t len, bool cow = false);

	protected:
		unsigned char _type       : 3;
		mutable bool _sso         : 1; // small-string-optimization
		union {
			struct {     // not sso
				mutable bool _cow : 1; // used for copy-on-write string
				bool _e           : 1; // used for string, indicates needs to be escaped or encoded.
				char              : 2; // reserved
			};
			unsigned int _sso_len : 4;
		};
		char _sso_s[3]; // sso string storage, 15 bytes can be used in fact
		unsigned int _cow_len;
		union {
			bool    _b;
			int64_t _i;
			double  _f;
			mutable tstring* _s;
			ObjectT<char_t>* _o;
			ArrayT<char_t> * _a;
			const char_t   * _d;
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
		static inline size_t read(ValueT<char_t>& v, const char_t* in, size_t len, bool cow = false) {return v.read(in, len, cow);}
		static inline size_t read(ValueT<char_t>& v, const char_t* in, bool cow = false) {return v.read(in, detail::tcslen(in), cow);}
		static inline size_t read(ValueT<char_t>& v, const JSON_TSTRING(char_t)& in, bool cow = false) {return v.read(in.data(), in.size(), cow);}
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
	template<class char_t> inline bool operator==(const ValueT<char_t>& v, const char_t* s) {return v.type() == STRING && detail::tcslen(s) == v.length() && !detail::tcsncmp(s, v.c_str(), v.length());}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_integral<T>::value, bool>::type
	operator==(const ValueT<char_t>& v, T i) {return v.type() == INTEGER && i == v.i();}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value, bool>::type
	operator==(const ValueT<char_t>& v, T f) {return v.type() == FLOAT && fabs(f - v.f()) < JSON_EPSILON;}
	template<class char_t> inline bool operator==(const ValueT<char_t>& v, const JSON_TSTRING(char_t)& s) {return v.type() == STRING && !s.compare(0, s.length(), v.c_str(), v.length());}
	template<class char_t> inline bool operator==(const ValueT<char_t>& v, const ObjectT<char_t>& o) {return v.type() == OBJECT && o == v.o();}
	template<class char_t> inline bool operator==(const ValueT<char_t>& v, const ArrayT<char_t>& a) {return v.type() == ARRAY && a == v.a();}

	template<class char_t> inline bool operator==(bool b, const ValueT<char_t>& v) {return operator==(v, b);}
	template<class char_t> inline bool operator==(const char_t* s, const ValueT<char_t>& v) {return operator==(v, s);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_integral<T>::value, bool>::type
	operator==(T i, const ValueT<char_t>& v) {return operator==(v, i);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value, bool>::type
	operator==(T f, const ValueT<char_t>& v) {return operator==(v, f);}
	template<class char_t> inline bool operator==(const JSON_TSTRING(char_t)& s, const ValueT<char_t>& v) {return operator==(v, s);}
	template<class char_t> inline bool operator==(const ObjectT<char_t>& o, const ValueT<char_t>& v) {return operator==(v, o);}
	template<class char_t> inline bool operator==(const ArrayT<char_t>& a, const ValueT<char_t>& v) {return operator==(v, a);}

	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, bool b) {return !operator==(v, b);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, const char_t* s) {return !operator==(v, s);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_integral<T>::value, bool>::type
	operator!=(const ValueT<char_t>& v, T i) {return !operator==(v, i);}
	template<class char_t, class T> inline typename detail::json_enable_if<detail::json_is_floating_point<T>::value, bool>::type
	operator!=(const ValueT<char_t>& v, T f) {return !operator==(v, f);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, const JSON_TSTRING(char_t)& s) {return !operator==(v, s);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, const ObjectT<char_t>& o) {return !operator==(v, o);}
	template<class char_t> inline bool operator!=(const ValueT<char_t>& v, const ArrayT<char_t>& a) {return !operator==(v, a);}

	template<class char_t> inline bool operator!=(bool b, const ValueT<char_t>& v) {return !operator==(v, b);}
	template<class char_t> inline bool operator!=(const char_t* s, const ValueT<char_t>& v) {return !operator==(v, s);}
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
			case STRING:
				_sso = true;
				_sso_len = 0;
				break;
			case OBJECT:  _o = new ObjectT<char_t>;  break;
			case ARRAY:   _a = new ArrayT<char_t>;  break;
			default:      break;
		}
	}

	template<class char_t>
	ValueT<char_t>::ValueT(const ValueT<char_t>& v)
		: _type(v._type)
	{
		if(this != &v) {
			switch(_type) {
				case NIL:     _type = NIL; break;
				case BOOLEAN: _b = v._b;   break;
				case INTEGER: _i = v._i;   break;
				case FLOAT:   _f = v._f;   break;
				case STRING:
					_sso = true;
					_sso_len = 0;
					assign(v.c_str(), v.length(), v._e, !v._sso && v._cow);
					break;
				case OBJECT:  _o = new ObjectT<char_t>(*v._o); break;
				case ARRAY:   _a = new ArrayT<char_t>(*v._a); break;
			}
		}
	}

	template<class char_t>
	void ValueT<char_t>::assign(const char_t* s, size_t l, int escape, bool cow)
	{
		clear(STRING);
		if(escape == AUTO_DETECT) {
			escape = DONT_ESCAPE;
			size_t pos = 0;
			while(pos < l) {
				if((escape = detail::check_need_conv<char_t>(s[pos])))
					break;
				++pos;
			}
		}
		if((_e = escape)) {
			if(_sso || _cow) {
				_sso = _cow = false;
				_s = new tstring(s, l);
			}
			else {
				_s->assign(s, l);
			}
		}
		else if(cow && l <= (unsigned)-1) {
			if(!_sso && !_cow) delete _s;
			_sso = false;
			_cow = true;
			_d = s;
			_cow_len = l;
		}
		else if(l <= (15 / sizeof(char_t))) {
			if(!_sso && !_cow) delete _s;
			_sso = true;
			_sso_len = l;
			memcpy(_sso_s, s, l * sizeof(char_t));
		}
		else {
			if(_sso || _cow) {
				_sso = _cow = false;
				_s = new tstring(s, l);
			}
			else {
				_s->assign(s, l);
			}
		}
	}

#ifdef __XPJSON_SUPPORT_MOVE__
	template<class char_t>
	void ValueT<char_t>::assign(tstring&& s, int escape)
	{
		if(escape == AUTO_DETECT) {
			escape = DONT_ESCAPE;
			size_t pos = 0;
			while(pos < s.length()) {
				if((escape = detail::check_need_conv<char_t>(s[pos])))
					break;
				++pos;
			}
		}
		clear(STRING);
		if(_sso || _cow) {
			_sso = _cow = false;
			_s = new tstring;
		}
		_s->swap(s);
		_e = escape;
	}
#endif

	template<class char_t>
	void ValueT<char_t>::assign(const ValueT<char_t>& v)
	{
		if(this != &v) {
			clear(v._type);
			switch(_type) {
				case NIL:     _type = NIL; break;
				case BOOLEAN: _b = v._b;   break;
				case INTEGER: _i = v._i;   break;
				case FLOAT:   _f = v._f;   break;
				case STRING:
					assign(v.c_str(), v.length(), v._e, !v._sso && v._cow);
					break;
				case OBJECT:  *_o = *v._o; break;
				case ARRAY:   *_a = *v._a; break;
			}
		}
	}

#ifdef __XPJSON_SUPPORT_MOVE__
	template<class char_t>
	void ValueT<char_t>::assign(ValueT<char_t>&& v)
	{
		if(this != &v) {
			clear(v._type);
			switch(_type) {
				case NIL:     _type = NIL; break;
				case BOOLEAN: _b = v._b;   break;
				case INTEGER: _i = v._i;   break;
				case FLOAT:   _f = v._f;   break;
				case STRING:
					if(!_sso && !_cow && !v._sso && !v._cow) {
						swap(_s, v._s);
						_e = v._e;
					}
					else {
						assign(v.c_str(), v.length(), v._e, !v._sso && v._cow);
					}
					break;
				case OBJECT:  swap(_o, v._o); break;
				case ARRAY:   swap(_a, v._a); break;
			}
			v.clear();
		}
	}
#endif

	template<class char_t>
	void ValueT<char_t>::clear(unsigned char type /*= NIL*/)
	{
		if(_type != type) {
			switch(_type) {
				case STRING: if(!_sso && !_cow) delete _s; break;
				case OBJECT: delete _o; break;
				case ARRAY:  delete _a; break;
				default: break;
			}
			switch(type) {
				case STRING:
					_sso = true;
					_sso_len = 0;
					break;
				case OBJECT: _o = new ObjectT<char_t>; break;
				case ARRAY:  _a = new ArrayT<char_t>;  break;
				default: break;
			}
			_type = type;
		}
		else {
			switch(_type) {
				case STRING: if(!_sso && !_cow) _s->clear(); break;
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
			template<class char_t, class T>
			typename json_enable_if<json_is_arithmetic<T>::value, T>::type
			internal_type_casting(const JSON::ValueT<char_t>& v, const T& value)
			{
				switch(v.type()) {
					case NIL:     break;
					case BOOLEAN: return T(v.b());
					case INTEGER: return T(v.i());
					case FLOAT:   return T(v.f());
					case STRING: {
							if(v.length() == 4 && v.c_str()[0] == 't' && v.c_str()[1] == 'r' && v.c_str()[2] == 'u' && v.c_str()[3] == 'e')
								return T(1);
							else if(v.length() == 5 && v.c_str()[0] == 'f' && v.c_str()[1] == 'a' && v.c_str()[2] == 'l' && v.c_str()[3] == 's' && v.c_str()[4] == 'e')
								return T(0);
							try {
								JSON::ValueT<char_t> vd;
								vd.read_number(v.c_str(), v.length(), false);
								return vd.template get<T>(value);
							}
							catch(std::exception& e) {
								return T(value);
							}
						}
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
					case BOOLEAN: return T(to_string<const char*, char_t>(v.b() ? "true" : "false"));
					case INTEGER: return to_string<int64_t, char_t>(v.i());
					case FLOAT:   return to_string<double, char_t>(v.f());
					case STRING:  return T(v.c_str(), v.length());
					default: JSON_ASSERT_CHECK1(false, "Type-casting error: from (%s) type to string.", get_type_name(v.type()));
				}
				return T(value);
			}
		}
	}

	template<class char_t> template<class T>
	T JSON::ValueT<char_t>::get(const T& default_value) const
	{
		return JSON_MOVE((detail::internal_type_casting <char_t, T>(*this, default_value)));
	}

	template<class char_t> template<class T>
	T JSON::ValueT<char_t>::get(const tstring& key, const T& default_value) const
	{
		if(_type != OBJECT) return T(default_value);
		typename ObjectT<char_t>::const_iterator it = _o->find(key);
		if(it != _o->end()) return JSON_MOVE((detail::internal_type_casting <char_t, T>(it->second, default_value)));
		return T(default_value);
	}

	template<class char_t>
	void ValueT<char_t>::write(tstring& out) const
	{
		switch(_type) {
			case NIL:     detail::to_string("null", out);break;
			case INTEGER: detail::to_string(_i, out);        break;
			case FLOAT:   detail::to_string(_f, out);        break;
			case OBJECT:  WriterT<char_t>::write(*_o, out);  break;
			case ARRAY:   WriterT<char_t>::write(*_a, out);  break;
			case BOOLEAN:
				detail::to_string(_b ? "true" : "false", out);
				break;
			case STRING:
				out += '\"';
				if(!_sso && !_cow && _e) detail::encode(_s->c_str(), _s->length(), out);
				else out.append(c_str(), length());
				out += '\"';
				break;
		}
	}

	template<class char_t>
	void ValueT<char_t>::to_string(tstring& out) const
	{
		if(_type == STRING) out = s();
		else {out.clear(); write(out);}
	}

#define case_white_space	case ' ':case '\n':case '\r':case '\t'
#define case_number_0_9		case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define case_number_ending	case_white_space: case ',':case ']':case '}'

	inline bool is_ws(char ch) {
		static const unsigned char ws_mask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
		static const unsigned char ws_bmap[8] = {0x00, 0x64, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00};
  		return !((unsigned char)ch & 0xC0) && (ws_bmap[(ch >> 3) & 0x07] & ws_mask[ch & 0x07]);
	}

	template<class char_t>
	size_t ValueT<char_t>::read_string(const char_t* in, size_t len, bool cow)
	{
		register bool e = false;
		register size_t pos = 0;

		while(pos < len && is_ws(in[pos])) ++pos;
		JSON_PARSE_CHECK(pos < len && in[pos] == '\"');
		register size_t start = ++pos;
		while(pos < len) {
			if(XPJSON_UNLIKELY(in[pos] == '\"')) {
				if(e) {
					clear(STRING);
					if(_sso || _cow) {
						_sso = _cow = false;
						_s = new tstring;
					}
					detail::decode(in + start, pos - start, *_s);
					_e = e;
				}
				else {
					assign(in + start, pos - start, e, cow);
				}
				return pos + 1;
			}
			else {
				if(!e) e = detail::check_need_conv<char_t>(in[pos]);
				if(e && in[pos] == '\\') ++pos;
			}
			++pos;
		}
		JSON_PARSE_CHECK(false);
	}

	template<class char_t>
	size_t ValueT<char_t>::read_number(const char_t* in, size_t len, bool)
	{
		register size_t pos = 0;
		register bool neg = false;
		register int sigfand = 0;
		register bool dec_overflow = false;
		register int dec = -1;
		register uint64_t i = 0;

		while(pos < len && is_ws(in[pos])) ++pos;

		if(in[pos] == '-') {
			neg = true;
			++pos;
		}

		if(XPJSON_UNLIKELY(in[pos] == '0')) {
			if(++pos >= len) {
				clear(INTEGER);
				_i = 0;
				return pos;
			}
		}
		else {
			while(unsigned(in[pos] - '0') < 10) {
				if(XPJSON_LIKELY(!dec_overflow && i <= 922337203685477580ULL)) {
					i = i * 10 + (in[pos] - '0');
					if(XPJSON_UNLIKELY(sigfand == 18 && i > 9223372036854775807ULL + neg)) dec_overflow = true;
				}
				else {
					dec_overflow = true;
				}
				++sigfand;
				if(XPJSON_UNLIKELY(++pos >= len)) break;
			}
			JSON_PARSE_CHECK(i);
		}

		if(XPJSON_UNLIKELY(dec_overflow)) {
			sigfand -= 19 - (i < 1000000000000000000ULL);
		}

		if(XPJSON_UNLIKELY(in[pos] == '.')) {
			JSON_PARSE_CHECK(dec++ == -1);
			++pos;
			if(XPJSON_LIKELY(i <= 922337203685477580ULL)) {
				while(unsigned(in[pos] - '0') < 10) {
					if(XPJSON_LIKELY(i <= 922337203685477580ULL)) {
						i = i * 10 + (in[pos] - '0');
						++dec;
					}
					else break;
					if(XPJSON_UNLIKELY(++pos >= len)) break;
				}
				JSON_PARSE_CHECK(dec);
			}
			while(unsigned(in[pos] - '0') < 10 && ++pos < len);
		}

		register int exp = 0;
		register bool exp_neg = false;
		if(pos < len && (in[pos] | 0x20) == 'e') {
			JSON_PARSE_CHECK(++pos < len);
			if(in[pos] == '-') {
				exp_neg = true;
				++pos;
			}
			else if(in[pos] == '+') {
				++pos;
			}
			JSON_PARSE_CHECK(pos < len && unsigned(in[pos] - '0') < 10);
			while(unsigned(in[pos] - '0') < 10) {
				exp = exp * 10 + (in[pos++] - '0');
			}
			if(XPJSON_UNLIKELY(dec_overflow)) {
				if(exp_neg) {
					exp = sigfand - exp;
					if(exp < 0)
						exp = -exp;
					else
						exp_neg = false;
				}
				else
					exp += sigfand;
			}
			else if(dec != -1)
				exp -= (exp_neg ? -dec : dec);
		}
		else {
			exp_neg = !dec_overflow;
			exp = XPJSON_UNLIKELY(dec_overflow) ? sigfand : dec;
		}

		if(XPJSON_UNLIKELY(pos < len)) {
			switch (in[pos]) {
				case_number_ending:
					break;
				default:
					JSON_PARSE_CHECK(false);
			}
		}

		if(dec == -1 && exp == -1) {
			clear(INTEGER);
			_i = (neg ? -i : i);
		}
		else {
			clear(FLOAT);
			if (XPJSON_UNLIKELY(exp > 0x1FF)) {
				// errno = ERANGE;
				_f = 0;
			}
			else {
				register double base = 1.0;
				static const double pow10[] = {10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256};
				for(int x = 0; exp && x < 9; exp >>= 1, ++x) {
					if(exp & 1) base *= pow10[x];
				}
				_f = (exp_neg ? (double)i / base : (double)i * base);
				if(neg) _f = -_f;
			}
		}
		return pos;
	}

	template<class char_t>
	size_t ValueT<char_t>::read_nil(const char_t* in, size_t len, bool)
	{
		register size_t pos = 0;
		while(pos < len && is_ws(in[pos])) ++pos;
		if(len - pos >= 4 && in[pos] == 'n' && in[pos + 1] == 'u' && in[pos + 2] == 'l' && in[pos + 3] == 'l') {
			clear();
			return pos + 4;
		}
		JSON_PARSE_CHECK(false);
	}

	template<class char_t>
	size_t ValueT<char_t>::read_boolean(const char_t* in, size_t len, bool)
	{
		register size_t pos = 0;
		while(pos < len && is_ws(in[pos])) ++pos;
		if(len - pos >= 4 && in[pos] == 't' && in[pos + 1] == 'r' && in[pos + 2] == 'u' && in[pos + 3] == 'e') {
			clear(BOOLEAN);
			_b = true;
			return pos + 4;
		}
		else if(len - pos >= 5 && in[pos] == 'f' && in[pos + 1] == 'a' && in[pos + 2] == 'l' && in[pos + 3] == 's' && in[pos + 4] == 'e') {
			clear(BOOLEAN);
			_b = false;
			return pos + 5;
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
	size_t ValueT<char_t>::read(const char_t* in, size_t len, bool cow/* = false*/)
	{
		// Indicate current parse state
		enum {OBJECT_LBRACE,          /* { */
			  OBJECT_PAIR_KEY_QUOTE,  /* {"," */
			  OBJECT_PAIR_KEY,        /* "..." */
			  OBJECT_PAIR_COLON,      /* "...": */
			  OBJECT_PAIR_VALUE,      /* "...":"..." */
			  OBJECT_COMMA,           /* {..., */
			  ARRAY_LBRACKET,         /* [ */
			  ARRAY_ELEM,             /* [...[...,... */
			  ARRAY_COMMA             /* [..., */
		};
		register unsigned char state = 0;
		register size_t pos = 0;
		union {
			size_t start;
			size_t(ValueT<char_t>::*fp)(const char_t*, size_t, bool);
		} u;
		memset(&u, 0, sizeof(u));
		std::deque<ValueT<char_t>*> pv(1, this);
		while(pos < len && is_ws(in[pos])) ++pos;
		// Topmost value parse.
		switch(in[pos++]) {
			case '{': state = OBJECT_LBRACE;  clear(OBJECT); break;
			case '[': state = ARRAY_LBRACKET; clear(ARRAY);  break;
			default: JSON_PARSE_CHECK(false);
		}
		while(pos < len) {
			switch(state) {
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
							while(pos < len) {
								if(in[pos] == '\"') {
									state = OBJECT_PAIR_KEY;
									JSON_TSTRING(char_t) key;
									detail::decode(in + u.start, pos - u.start, key);
									pv.push_back(&(*pv.back()->_o)[JSON_MOVE(key)]);
									u.start = 0;
									break;
								}
								else if(in[pos] == '\\') {
									if(++pos >= len) {
										break;
									}
								}
								++pos;
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
						pos += (pv.back()->*u.fp)(in + pos, len - pos, cow) - 1;
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
#undef case_number_0_9
#undef case_number_ending
#undef OBJECT_ARRAY_PARSE_END
#undef PUSH_VALUE_TO_STACK

	template<class char_t>
	void WriterT<char_t>::write(const ObjectT<char_t>& o, JSON_TSTRING(char_t)& out)
	{
		out += '{';
		for(typename ObjectT<char_t>::const_iterator it = o.begin(); it != o.end(); ++it) {
			out += '\"';
			detail::encode(it->first.c_str(), it->first.length(), out);
			out += '\"';
			out += ':';
			it->second.write(out);
			out += ',';
		}
		if(out[out.length() - 1] != '{') out[out.length() - 1] = '}'; else out += '}';
	}

	template<class char_t>
	void WriterT<char_t>::write(const ArrayT<char_t>& a, JSON_TSTRING(char_t)& out)
	{
		out += '[';
		for(size_t i = 0; i < a.size(); ++i) {
			a[i].write(out);
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
			if(lit->first != rit->first || lit->second != rit->second) return false;
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
			case STRING:  return lhs.length() == rhs.length() && !memcmp(lhs.c_str(), rhs.c_str(), lhs.length());
			case OBJECT:  return lhs.o() == rhs.o();
			case ARRAY:   return lhs.a() == rhs.a();
		}
		return true;
	}
}
