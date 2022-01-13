# Fast JSON writer/serializer

* No inherent memory allocations.
* No intermediate values storage.

Unlike generic libraries like https://github.com/nlohmann/json this library
provides only serialization. The output buffer must have a contiguous storage,
like `std::vector`.

## Dependencies

* C++17
* https://github.com/fmtlib/fmt 7.0+

## Examples

See `test.cpp` file. For custom formatters look for `jsonwriter::Formatter`.
