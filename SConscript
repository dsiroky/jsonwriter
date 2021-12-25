Import("env", "thirdparty_env")

gtest_env = thirdparty_env.Clone()
gtest_env.AppendUnique(CPPPATH=["3rdparty/googletest/googletest/include", "3rdparty/googletest/googletest"])
gtest_lib = gtest_env.StaticLibrary("gtest", "3rdparty/googletest/googletest/src/gtest-all.cc")

gbenchmark_env = thirdparty_env.Clone()
gbenchmark_env.AppendUnique(
        CPPDEFINES=["NDEBUG"],
        CCFLAGS=["-O3"],
        CPPPATH=["3rdparty/benchmark/include"]
    )
gbenchmark_lib = gbenchmark_env.StaticLibrary("gbenchmark", Glob("3rdparty/benchmark/src/*.cc",
                                                                 exclude="3rdparty/benchmark/src/benchmark_main.cc"))

fmt_env = thirdparty_env.Clone()
fmt_env.AppendUnique(CPPPATH=["3rdparty/fmt/include"])
fmt_lib = fmt_env.StaticLibrary("fmt", Glob("3rdparty/fmt/src/*.cc"))

env.AppendUnique(LIBS=[fmt_lib, gtest_lib, gbenchmark_lib])
env.Program("test", "test.cpp")
env.Program("benchmark", "benchmark.cpp")
