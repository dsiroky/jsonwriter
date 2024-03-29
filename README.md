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

## Benchmarks

gcc 11, -O3, Intel Core i7-8700K

```
BM_jsonwriter_simple_small_static_struct                    72.4 ns
BM_jsonwriter_simple_small_static_struct_list             115950 ns
BM_jsonwriter_simple_small_struct                            278 ns
BM_jsonwriter_large_strings                                71194 ns

BM_rapidjson_simple_small_static_struct                      245 ns
BM_rapidjson_simple_small_static_struct_dump_only            189 ns
BM_rapidjson_simple_small_static_struct_list              215528 ns
BM_rapidjson_simple_small_struct                             799 ns
BM_rapidjson_large_strings                                152619 ns
BM_rapidjson_large_strings_dump_only                      142445 ns
```

## Dependencies

* C++17
