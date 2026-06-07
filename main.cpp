#include "uci.hpp"
#include "engine.hpp"

int main(const int argc, char* argv[])
{
    start();

    if (argc > 1 && std::string_view(argv[1]) == "bench")
    {
        std::string bench_args;
        for (int i = 1; i < argc; ++i)
        {
            bench_args += argv[i];
            if (i < argc - 1) bench_args += " ";
        }

        UCI::bench(bench_args);
        return 0;
    }

    UCI::process();
}