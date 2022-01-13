# Fast JSON writer/serializer

* Small, header only.
* No inherent memory allocations.
* No intermediate values storage.

Unlike generic libraries like https://github.com/nlohmann/json this library
provides **only serialization**. The output buffer must have a contiguous storage,
like `std::vector`.

## Benchmarks

```
BM_jsonwriter_simple_small_struct                74.3 ns         74.3 ns      9433057
BM_jsonwriter_large_strings                     68828 ns        68791 ns        10046

BM_rapidjson_simple_small_struct                  261 ns          261 ns      2687401
BM_rapidjson_simple_small_struct_dump_only        182 ns          182 ns      3818769
BM_rapidjson_large_strings                     111103 ns       111062 ns         6486
BM_rapidjson_large_strings_dump_only            95702 ns        95670 ns         7599
```

## Dependencies

* C++17
* https://github.com/fmtlib/fmt 7.0+

## Examples

See `test.cpp` file. For custom formatters look for `jsonwriter::Formatter`.
