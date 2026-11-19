[![circleci](https://circleci.com/gh/dsiroky/jsonwriter/tree/main.svg?style=shield)](https://circleci.com/gh/dsiroky/jsonwriter/?branch=main)
[![appveyor](https://ci.appveyor.com/api/projects/status/60y6xtuwiswnawqv/branch/main?svg=true)](https://ci.appveyor.com/project/dsiroky/jsonwriter/branch/main)

# Fast JSON writer/serializer

* Fast. Really fast.
* Small, header only.
* No inherent memory allocations.
* No intermediate values storage.

Unlike generic libraries like [nlohmann](https://github.com/nlohmann/json) or
[RapidJSON](https://rapidjson.org/) this library provides **only
serialization**.

## Example

```
#include <iostream>
#include <jsonwriter/writer.hpp>

int main() {
    jsonwriter::SimpleBuffer out{};
    jsonwriter::write(out, jsonwriter::Object{[](auto& object) {
        object["k1"] = "cd";
        object["k2"] = jsonwriter::Object{[](auto& nested_object) {
            nested_object["o1"] = {1, 2};
            nested_object["o2"] = false;
            nested_object["o3"] = "i\no";
            nested_object["o4"] = jsonwriter::empty_list;
            nested_object["o5"] = jsonwriter::empty_object;
        }};
        object["k3"] = true;
    }});

    std::cout << std::string_view{out.begin(), out.size()} << '\n';
    return 0;
}
```

See `test.cpp` for more use cases. For custom formatters look for `jsonwriter::Formatter` or `void write(jsonwriter::Buffer& buffer)` member function.

## Non-finite values

JSON cannot represent infinity or NaN. By default serializing such a value
throws `std::domain_error`:

```
jsonwriter::write(out, std::numeric_limits<double>::infinity()); // throws
```

The behavior is a compile time switch. Define `JSONWRITER_NONFINITE` to one of
the following before including `jsonwriter/writer.hpp`:

| Value                        | Behavior                                      |
| ---------------------------- | --------------------------------------------- |
| `JSONWRITER_NONFINITE_THROW` | throws `std::domain_error` (default)          |
| `JSONWRITER_NONFINITE_NULL`  | serializes the value as `null`                |
| `JSONWRITER_NONFINITE_ABORT` | aborts the program (useful when exceptions are disabled) |

```
#define JSONWRITER_NONFINITE JSONWRITER_NONFINITE_NULL
#include <jsonwriter/writer.hpp>
```

Define `JSONWRITER_NONFINITE` consistently in all translation units that
include the header; mixing settings within one program is an ODR violation.

When the writer throws, the contents of the buffer are unspecified — don't
keep writing into it or reuse its output.

## Benchmarks

gcc 15.2, -O3, AMD Ryzen 7 PRO 7840HS

```
BM_jsonwriter_simple_small_static_struct                    23.6 ns
BM_jsonwriter_simple_small_static_struct_list              23945 ns
BM_jsonwriter_simple_small_struct                            102 ns
BM_jsonwriter_large_strings                                39279 ns

BM_rapidjson_simple_small_static_struct                      266 ns
BM_rapidjson_simple_small_static_struct_dump_only            200 ns
BM_rapidjson_simple_small_static_struct_list              666164 ns
BM_rapidjson_simple_small_struct                             730 ns
BM_rapidjson_large_strings                                142939 ns
BM_rapidjson_large_strings_dump_only                      132419 ns
```

## Dependencies

* C++17
