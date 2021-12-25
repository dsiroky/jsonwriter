# Fast JSON writer/serializer

* No inherent memory allocations.
* No intermediate values storage.

Unlike generic libraries like https://github.com/nlohmann/json this library
provides only serialization. The library doesn't do any value copying or memory
allocations. The output buffer should be either a fixed array or a growing
buffer via e.g. a back inserter.

## Dependencies

* C++17
* https://github.com/fmtlib/fmt 7.0
