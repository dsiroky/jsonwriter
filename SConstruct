import os
import socket
import sys
import time

#==========================================================================
# build options

OPTIMIZATION_CHOICES = ["none", "devel", "full"]
AddOption("--optimization",
          dest="optimization",
          default="devel",
          nargs=1,
          choices=OPTIMIZATION_CHOICES,
          help="Optimization level [{}]. (default: %default)".format("|".join(OPTIMIZATION_CHOICES)))
AddOption("--prefix",
          dest="install_prefix",
          default="/usr/local",
          type="string",
          nargs=1,
          help="Installation prefix. (default: %default)")

#==========================================================================

PROJECT_ROOT = Dir("#").abspath

EnsurePythonVersion(3, 0)
EnsureSConsVersion(3, 0)
Decider("MD5-timestamp")
SetOption("max_drift", 1)

env = Environment(MSVC_VERSION="14.2", toolpath=[os.path.join(PROJECT_ROOT, "tools/scons")])

env["OPTIMIZATION"] = GetOption("optimization")

for cmd in ("CC", "CXX"):
    env[cmd] = os.environ.get(cmd) or env[cmd]

env["BUILD_DIR"] = "build"
env.SConsignFile("$BUILD_DIR/.sconsign.$HOST_TYPE")

env["CXXFILESUFFIX"] = ".cpp" # default is ".cc"

env["INSTALL_PREFIX"] = GetOption("install_prefix")

env["IS_MSVC"] = "msvc" in env["TOOLS"]

if env["IS_MSVC"]:
    env.AppendUnique(CCFLAGS=["/std:c++17", "/EHsc"])
else:
    env["CONFIGUREDIR"] = "$BUILD_DIR/sconf_temp"
    env["CONFIGURELOG"] = "$BUILD_DIR/config.log"
    c_warnings = []
    cxx_warnings = []
    if not env.GetOption("clean") and not GetOption("help"):
        print("Checking compiler warnings...")
        code = "void f(){}"
        conf = Configure(env.Clone())
        for warning in (
                "-Wabstract-vbase-init",
                "-Waddress-of-array-temporary",
                "-Warray-bounds",
                "-Warray-bounds-pointer-arithmetic",
                "-Wassign-enum",
                "-Wattributes",
                "-Wbool-conversion",
                "-Wbridge-cast",
                "-Wbuiltin-requires-header",
                "-Wc++11-narrowing",
                "-Wc++17-compat",
                "-Wcast-align",
                "-Wchar-subscripts",
                "-Wconditional-uninitialized",
                "-Wconstant-conversion",
                "-Wconstant-logical-operand",
                "-Wconstexpr-not-const",
                "-Wconsumed",
                "-Wdangling-else",
                "-Wdangling-field",
                "-Wdeprecated",
                "-Wdeprecated-increment-bool",
                "-Wduplicate-method-match",
                "-Wempty-body",
                "-Wenum-conversion",
                "-Wfloat-conversion",
                "-Wfloat-equal",
                "-Wint-conversion",
                "-Wmissing-braces",
                "-Wnon-virtual-dtor",
                "-Wold-style-cast",
                "-Wparentheses",
                "-Wpointer-sign",
                "-Wshadow",
                "-Wshorten-64-to-32",
                "-Wstrict-aliasing",
                "-Wswitch",
                "-Wswitch-default",
                "-Wswitch-enum",
                "-Wtautological-compare",
                "-Wthread-safety-analysis",
                "-Wuninitialized",
                "-Wunreachable-code",
                "-Wunused-function",
                "-Wunused-parameter",
                "-Wunused-result",
                "-Wunused-value",
                "-Wunused-variable",
                ):

            conf.env["CFLAGS"] = ["-Werror", warning]
            if conf.TryCompile(code, ".c"):
                c_warnings.append(warning)

            conf.env["CXXFLAGS"] = ["-Werror", warning]
            if conf.TryCompile(code, ".cpp"):
                cxx_warnings.append(warning)

        # faster than standard malloc, mainly for reducing overhead in benchmarks
        conf.env["LIBS"] = "tcmalloc_minimal"
        if conf.TryLink("int main(){return 0;}", ".cpp"):
            env.Append(LIBS=["tcmalloc_minimal"])

        assert len(c_warnings) > 0
        assert len(cxx_warnings) > 0

    env.AppendUnique(
        CPPDEFINES=[
            ],
        CCFLAGS=[
            "-pipe",
            "-pthread",
            "-fdiagnostics-color=always",
            "-ggdb3",

            "-fsigned-char",
            "-ffast-math",
            "-fomit-frame-pointer",
            "-fno-rtti",
            "-fvisibility=hidden",
            ],
        CXXFLAGS=[
            "-std=c++17",
            ],
        LINKFLAGS=[
            "-pthread",
            ],
        )

    if "clang" in env["CXX"]:
        env.AppendUnique(CCFLAGS=["-ferror-limit=5", ])
    else:
        env.AppendUnique(CCFLAGS=["-fmax-errors=5", ])

    if env["OPTIMIZATION"] == "none":
        env.AppendUnique(CCFLAGS=["-O0"])
    elif env["OPTIMIZATION"] == "devel":
        env.AppendUnique(CCFLAGS=["-Og"])
    elif env["OPTIMIZATION"] == "full":
        env.AppendUnique(
            CCFLAGS=["-O3"],
            LINKFLAGS=["-O3"],
        )
    else:
        assert False

    if env["OPTIMIZATION"] == "full":
        env.AppendUnique(
            CPPDEFINES=[
                "NDEBUG"
                ],
            CCFLAGS=[
                "-feliminate-unused-debug-types",
                "-fno-stack-protector",
                ],
        )
    else:
        env.AppendUnique(
            CPPDEFINES=[
                "_DEBUG"

                # libc/stdc++ hardening
                "_FORTIFY_SOURCE=2",
                "_GLIBCXX_ASSERTIONS",
                ],
            CCFLAGS=[
                "-fno-omit-frame-pointer",
                "-fno-optimize-sibling-calls",
                "-fstack-protector-all",
                ],
        )

# keep only common compilation flags, avoid include paths and warning flags
thirdparty_env = env.Clone()

env.AppendUnique(
    CPPPATH=[
        "#",
        "#3rdparty/benchmark/include",
        "#3rdparty/googletest/googletest/include",
        "#3rdparty/googletest/googlemock/include",
        "#3rdparty/fmt/include",
    ],
)

if not env["IS_MSVC"]:
    env.AppendUnique(
        CCFLAGS=[
            "-pedantic",
            "-Wall",
            "-Wextra",
        ],
    )

    # The warnings configuration must be done before setting other flags colliding with checks. Warnings
    # are added only to the main environment.
    env.AppendUnique(CFLAGS=c_warnings,
                     CXXFLAGS=cxx_warnings)

    if "clang" in env["CC"]:
        env.AppendUnique(
            CCFLAGS=[
                "-Wconversion",
            ])

Export("env", "thirdparty_env")
env.SConscript("SConscript", variant_dir="$BUILD_DIR", duplicate=0)
