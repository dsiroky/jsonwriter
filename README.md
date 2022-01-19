[![jsonwriter](https://circleci.com/gh/dsiroky/jsonwriter/tree/main.svg?style=shield)](https://circleci.com/gh/dsiroky/jsonwriter/?branch=main)

# Fast JSON writer/serializer

* Fast. Really fast.
* Small, single header only. `#include <jsonwriter/writer.hpp>`
* No inherent memory allocations.
* No intermediate values storage.

Unlike generic libraries like [nlohmann](https://github.com/nlohmann/json) or
[RapidJSON](https://rapidjson.org/) this library provides **only
serialization**.

## Example

```
jsonwriter::Buffer out{};
jsonwriter::write(out, jsonwriter::Object{[](auto& object) {
    object["k1"] = "cd";
    object["k2"] = jsonwriter::Object{[](auto& nested_object) {
        nested_object["o1"] = {1, 2};
        nested_object["o2"] = false;
        nested_object["o3"] = "i\no";
    }};
    object["k3"] = false;
}});

std::cout << std::string_view{out.begin(), out.size()} << '\n';
```

See `test.cpp` for more use cases. For custom formatters look for `jsonwriter::Formatter`.

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
* https://github.com/fmtlib/fmt 7.0+
