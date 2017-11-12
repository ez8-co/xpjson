
### What is xpjson?

[![license](https://img.shields.io/badge/license-MIT-brightgreen.svg?style=flat)](https://github.com/ez8-co/xpjson/blob/master/LICENSE)

- A minimal Xross-Platform/Xtreme-Performance JSON serialization & deserialization library in C++.

### Why use xpjson?

- **Easy-to-use**: it's STL-based, and designed to be used without extra studies.
- **Easy-to-assemble**: only a tiny header file needs to be included.
- **No dependencies**: unless STL and std c libraries.
- According to [**RFC 7159 - The JavaScript Object Notation (JSON) Data Interchange Format**](https://tools.ietf.org/html/rfc7159).
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
            "name": "Zhang Wei",
            "sex": "male"
        },
        "skills": [
            "C++",
            "golang",
            "python"
        ]
    }
}
```

#### serialization example

- You can rapidly start serialization by according to following example.

``` cpp
#include "../xpjson.hpp"
#include <iostream>

int main()
{
  JSON::Value v;
  v["orca"]["personalities"]["name"] = "Zhang Wei";
  v["orca"]["personalities"]["sex"] = "male";
  v["orca"]["personalities"]["company"] = "ez8.co";
  v["orca"]["skills"][0] = "C++";
  v["orca"]["skills"][1] = "golang";
  v["orca"]["skills"][2] = "python";

  string out;
  v.write(out);

  cout << out << endl;
  return 0;
}
```

- The output is :

```
~$ ./xpjson_example1
{"orca":{"personalities":{"age":28,"company":"ez8.co","name":"Zhang Wei","sex":"male"},"skills":["C++","golang","python"]}}
```

- But it's not a optimized one compared with the following example. 

``` cpp
#include "../xpjson.hpp"
#include <iostream>

int main()
{
  JSON::Value v;
  JSON::Object& orca = v["orca"].o();

  JSON::Object& personalities = orca["personalities"].o();
  personalities["name"] = "Zhang Wei";
  personalities["sex"] = "male";
  personalities["company"] = "ez8.co";

  JSON::Array& skills = orca["skills"].a();
  skills.reserve(3);
  // You should use resize or push_back for Array, coz it's vector.
  skills.resize(2);
  skills[0] = "C++";
  skills[1] = "golang";

  // Use JSON_MOVE to gain perf opt benefits under C++11 if possible.
  skills.push_back(JSON_MOVE(string("python")));

  string out;
  // reserve 100 bytes, reduce reallocation and copy cost of time
  out.reserve(100);
  v.write(out);

  cout << out << endl;
  return 0;
}
```

#### deserialization example

- And you can deserialize the stream by according to following example.

``` cpp
#include "../xpjson.hpp"
#include <iostream>
#include <cassert>

int main()
{
  string in("{\"orca\":{\"personalities\":{\"age\":28,\"company\":\"ez8.co\",\"name\":\"Zhang Wei\",\"sex\":\"male\"},\"skills\":[\"C++\",\"golang\",\"python\"]}}");
  JSON::Value v;
  size_t ret = v.read(in.c_str(), in.length());
  assert(ret == in.length());

  JSON::Object& orca = v["orca"].o();
  assert(orca.size() == 2);

  JSON::Object& personalities = orca["personalities"].o();
  JSON::Array& skills = orca["skills"].a();

  cout << "Personalities:" << endl;
  for(JSON::Object::const_iterator it=personalities.begin(); it!=personalities.end(); ++it) {
    string out;
    it->second.to_string(out);
    cout << it->first << " -> " << out << endl;
  }

  cout << endl << "Skills:" << endl;
  for(JSON::Array::const_iterator it=skills.begin(); it!=skills.end(); ++it) {
    cout << it->s() << endl;
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
name -> Zhang Wei
sex -> male

Skills:
C++
golang
python
```

### Benchmark

- \<UNDER CONSTRUCTION\>

### Performance-purpose Details

- Can **skip detection** that whether string needs to be encoded or escaped.
  - Enable detecttion as default.
    - Use a flag to skip when put string.
    - Auto set during deserialization if skip is possible.
  - Invalid after modifications (or possible modifications like get reference operation as memory watch is not ready now).
  - Which should not check every character during serialization and may gain SIMD intrinsics benefits of memxxx APIs (about 30% bonous as benchmark said).
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

### TODO

- Reader & Writer for file / stream.
- New *readv* method by passing *iovec* param.
- Optimization of using CPU intrinsics set.

### WIP

- Small string optimization (SSO) support.
- Direct memory access (DMA) support for string.

### Misc

- Please feel free to use xpjson.
- Looking forward to your suggestions.
- If your project is using xpjson, you can show your project or company here by creating a issue or let we know.