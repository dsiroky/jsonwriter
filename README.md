# Fast JSON writer/serializer

* Fast. Really fast.
* Small, single header only. `#include <jsonwriter/writer.hpp>`
* No inherent memory allocations.
* No intermediate values storage.

Unlike generic libraries like [nlohmann](https://github.com/nlohmann/json) or
[RapidJSON](https://rapidjson.org/) this library provides **only
serialization**.

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

## Examples

See `test.cpp` file. For custom formatters look for `jsonwriter::Formatter`.
