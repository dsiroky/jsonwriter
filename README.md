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
jsonwriter::write(out, [](auto& object) {
    object["k1"] = "cd";
    object["k2"] = [](auto& nested_object) {
        nested_object["o1"] = {1, 2};
        nested_object["o2"] = false;
        nested_object["o3"] = "i\no";
    };
    object["k3"] = false;
});

std::cout << std::string_view{out.begin(), out.size()} << '\n';
```

See `test.cpp` for more use cases. For custom formatters look for `jsonwriter::Formatter`.

## Benchmarks

gcc 11, -O3, Intel Core i7-8700K

```
BM_jsonwriter_simple_small_struct                52.7 ns
BM_jsonwriter_simple_small_struct_list         102461 ns
BM_jsonwriter_large_strings                     65423 ns

BM_rapidjson_simple_small_struct                  246 ns
BM_rapidjson_simple_small_struct_dump_only        197 ns
BM_rapidjson_simple_small_struct_list          217037 ns
BM_rapidjson_large_strings                     152939 ns
BM_rapidjson_large_strings_dump_only           142851 ns
```

## Dependencies

* C++17
* https://github.com/fmtlib/fmt 7.0+
