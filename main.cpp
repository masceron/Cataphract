#include <ranges>
#include "uci.hpp"
#include "engine.hpp"

int main(const int argc, char* argv[])
{
    start();

    if (argc > 1 && std::string_view(argv[1]) == "bench")
    {
        const std::string bench_args = std::span(argv, argc)
            | std::views::drop(1)
            | std::views::transform([](const char* ptr)
            {
                return std::string_view(ptr);
            })
            | std::views::join_with(' ')
            | std::ranges::to<std::string>();

        UCI::bench(bench_args);
        return 0;
    }

    UCI::process();
}
