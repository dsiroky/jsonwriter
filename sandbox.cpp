#include <iostream>
#include "jsonwriter/writer.hpp"

#include "benchmark_common.hpp"

int main(int argc, char* argv[])
{
    size_t s{};
    for (int i{0}; i<50000;++i){
    jsonwriter::Buffer out{};
    jsonwriter::write(out, large_string_list);
    s+=out.size();
    }
    std::cout << s << '\n';
    return 0;
}
