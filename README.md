
### What is xpjson?

[![license](https://img.shields.io/badge/license-MIT-brightgreen.svg?style=flat)](https://github.com/ez8-co/xpjson/blob/master/LICENSE)
[![Build Status](https://travis-ci.org/ez8-co/xpjson.svg?branch=master)](https://travis-ci.org/ez8-co/xpjson)
[![codecov](https://codecov.io/gh/ez8-co/xpjson/branch/master/graph/badge.svg)](https://codecov.io/gh/ez8-co/xpjson)

- A minimal Xross-Platform JSON read & write library in C++.

### Why use xpjson?

- **Easy-to-use**: it's STL-based, and designed to be used without extra studies.
- **Easy-to-assemble**: only a tiny header file needs to be included.
- **No dependencies**: unless STL and std c libraries.
- According to [**RFC 8259 - The JavaScript Object Notation (JSON) Data Interchange Format**](https://tools.ietf.org/html/rfc8259).
- **Compatiblities**: 
  - **Encoding-friendly**: support popular encodings, like ASCII, UTF8, GBK, GB2312, BIG 5, UTF16, UTF32, UCS-2, UCS-4.
  - **OS-friendly**: portable for popular platforms, like linux and windows.
  - **Compiler-friendly**: upward support old version of C++, and forward support for new features.
- **High-performance & high-concurrency** aimed.
- **100K scenario** validated till now in commercial distributed application service.
- **Elegant casting** between types, like string to integer, string to float, integer to string, and etc.
- **NULL** in string value supports.
- Every JSON value occupies fixed **16 bytes size** under both 32-bit and 64-bit system.

### Examples

``` json
{
    "orca": {
        "personalities": {
            "age": 28,
            "company": "ez8.co",
            "gender": "male",
            "name": "Zhang Wei"
        },
        "skills": [
            "C++",
            "golang",
            "python"
        ]
    }
}
```

#### write example

- You can rapidly start writing by according to following example.

``` cpp
#include "xpjson.hpp"
#include <iostream>

int main()
{
  JSON::Value v;
  v["orca"]["personalities"]["age"] = 28;
  v["orca"]["personalities"]["name"] = "Zhang Wei";
  v["orca"]["personalities"]["gender"] = "male";
  v["orca"]["personalities"]["company"] = "ez8.co";
  v["orca"]["skills"][0] = "C++"; // auto-expand array(for JSON::Value ONLY)
  v["orca"]["skills"][1] = "golang";
  v["orca"]["skills"][2] = "python";

  std::string out;
  v.write(out);

  std::cout << out << std::endl;
  return 0;
}
```

- The output is :

```
~$ ./xpjson_example1
{"orca":{"personalities":{"age":28,"company":"ez8.co","gender":"male","name":"Zhang Wei"},"skills":["C++","golang","python"]}}
```

- But it's not a optimized one compared with the following example. 

``` cpp
#include "xpjson.hpp"
#include <iostream>

int main()
{
  JSON::Value v;
  JSON::Object& orca = v["orca"].o();

  JSON::Object& personalities = orca["personalities"].o();
  personalities["age"] = 28;
  personalities["name"] = "Zhang Wei";
  personalities["gender"] = "male";
  personalities["company"] = "ez8.co";

  JSON::Array& skills = orca["skills"].a();
  skills.resize(2);
  skills[0] = "C++";
  skills[1] = "golang";
  skills[2] = "python";

  // Use JSON_MOVE to gain perf-opt benefits under C++11 if available.
  skills.push_back(JSON_MOVE(string("python")));

  std::string out;
  // reserve 100 bytes, reduce reallocation and copy cost of time
  out.reserve(100);
  v.write(out);

  std::cout << out << std::endl;
  return 0;
}
```

#### read example

- And you can read the stream by according to following example.

``` cpp
#include "xpjson.hpp"
#include <iostream>
#include <cassert>

int main()
{
  std::string in("{\"orca\":{\"personalities\":{\"age\":28,\"company\":\"ez8.co\",\"gender\":\"male\",\"name\":\"Zhang Wei\"},\"skills\":[\"C++\",\"golang\",\"python\"]}}");
  JSON::Value v;
  size_t ret = v.read(in);
  assert(ret == in.length());

  JSON::Object& orca = v["orca"].o();
  assert(orca.size() == 2);

  JSON::Object& personalities = orca["personalities"].o();
  JSON::Array& skills = orca["skills"].a();

  std::cout << "Personalities:" << std::endl;
  for(JSON::Object::const_iterator it=personalities.begin(); it!=personalities.end(); ++it) {
    std::string out;
    it->second.to_string(out);
    std::cout << it->first << " -> " << out << std::endl;
  }

  std::cout << endl << "Skills:" << std::endl;
  for(JSON::Array::const_iterator it=skills.begin(); it!=skills.end(); ++it) {
    std::cout << it->s() << std::endl;
  }

  return 0;
}
```

- The output is :

```
~$ ./xpjson_example3
Personalities:
age -> 28
company -> ez8.co
gender -> male
name -> Zhang Wei

Skills:
C++
golang
python
```

### REMARKS

- You MUST catch exception while using following methods that may throw exception.

```cpp
try {
	string in("{}");
	JSON::Value v;
	v.read(v, in.c_str(), in.length());
}
catch(std::exception &e) {
	printf("Error : %s.", e.what());
	// or do what you want
}
```

|Class Name|Method Name|
|-|-|
|JSON::Reader|read|
|JSON::Value|read|
|JSON::Value|read_nil|
|JSON::Value|read_boolean|
|JSON::Value|read_number|
|JSON::Value|read_string|
|JSON::Value|b|
|JSON::Value|i|
|JSON::Value|f|
|JSON::Value|s|
|JSON::Value|o|
|JSON::Value|a|

### Optional COW(Copy-On-Write) Feature

- You should care about the life cycle of input string or buffer by yourself when using `COW` feature.
- Pass `true` to `cow` switch to enable `COW` feature manually.
- `COW` feature may be automatically inherited by assignment or movement or reference.

|Class Name|Method Name|
|-|-|
|JSON::Reader|read|
|JSON::Value|read|
|JSON::Value|read_nil|
|JSON::Value|read_boolean|
|JSON::Value|read_number|
|JSON::Value|read_string|
|JSON::Value|ValueT(string/buffer related)|
|JSON::Value|assign(string/buffer related)|

### Benchmark

- \<UNDER CONSTRUCTION\>

### Performance-purpose Details

- Can **skip detection** that whether string needs to be encoded or escaped.
  - Enable detecttion as default.
    - Use a flag to skip when put string.
    - Auto set during `read` if skip is possible.
  - Invalid after modifications (or possible modifications like get reference operation as memory watch is not ready now).
  - Which should not check every character during `write` and may gain SIMD intrinsics benefits of memxxx APIs (about 30% bonous as benchmark said).
- **No useless pretty print** (indent, CRLF, space and other formats).
  - Gaudy feature I think, because performance, size of packet and binaries is most important, not low-frequency debug dump.
  - Use online json-validation web or web browser console to show them in pretty formats instead.
  - Less extra conditions (about 10% bonous as benchmark said).
- As **less temporary variables and condition branches** as possible.
- Auto enable **move operations** if compiler supports to reduce memory copy.
- Scan **only once** during parse.
- Type-traits for **value input and elegant** cast between types.
- High-concurrency support. **No global mutex lock** (compare with bxxst).
- Transfer as-is, as **less en(de)coding operations** as possible.
- **Hardcode en(de)coding**, without depends of library like iconv or system APIs.
- **SSO(Small-String-Optimization)** & **COW(Copy-On-Write)** support.

### TODO

- Reader & Writer for file / stream.
- New `readv` method by passing *iovec* param.
- Optimization of using CPU intrinsics set.

### Misc

- Please feel free to use xpjson.
- Looking forward to your suggestions.
- If your project is using xpjson, you can show your project or company here by creating a issue or let me know.
