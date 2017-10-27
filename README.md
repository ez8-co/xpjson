
### What is xpjson?

- A minimal Xross-Platform/Xtreme-Performance JSON serialization & deserialization library in C++.

### Why use xpjson?

- Easy-to-use: it's STL-based, and designed to be used without extra studies.
- Easy-to-assemble: only a tiny header file needs to be included.
- No dependencies: unless STL and std c libraries.
- Compatiblites: 
  - Encoding-friendly: support popular encodings, like ASCII, UTF8, GBK, GB2312, BIG 5, UTF16, UTF32, UCS-2, UCS-4.
  - OS-friendly: portable for popular platforms, like linux and windows.
  - Compiler-friendly: upward support old version of C++, and forward support for new features.
- High-performance & high-concurrency aimed.
- 100K scenario validated till now in commercial distributed application service.

### Examples

- You can rapidly start by according to following example.

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

- But it's not a optimized one compared with the following example. 

```cpp
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

- The author is writing and updating everyday...

- Just wait in patient.

### Benchmark

- <UNDER CONSTRUCTION>

### Performance-purpose Details

- Can skip detection that whether string needs to be converted or escaped
  - Enable detecttion as default.
    - Use a flag to skip when put string.
    - Auto set during deserialization if skip is possible.
  - Invalid after modifications (or possible modifications like get reference operation as memory watch is not ready now).
  - Which should not check every character during serialization and may gain SIMD instruments optimization benefits of memxxx APIs (about 30% bonous as benchmark said).
- No useless pretty print (indent, CRLF, space and other formats).
  - Gaudy feature I think, because performance, size of packet and binaries is most important, not low-frequency debug dump.
  - Use online json-validation web or web browser console to show them in pretty formats instead.
- As less temporary variables and condition branches as possible.
- Auto enable move operations if compiler supports to reduce memory copy.
- Scan only once during parse.
- Type-traits for value input and elegant cast between types.
- High-concurrency support. No global mutex lock (compare with bxxst).
- Transfer as-is, as less en(de)coding operations as possible.
- Hardcode en(de)coding, without depends of library like iconv or system APIs.

### TODO

- Data size optimization.
- New *readv* method by passing *iovec* param.
- Small string optimization (SSO) support.
- DMA (direct memory access) support for string.
- Reader & Writer for file.

### WIP

- Support escaped key.
- Escape unprintable characters.

### Misc

- Please feel free to use xpjson.
- Looking forward to your suggestions.
- You can show your project or company here by creating a issue or let we know.