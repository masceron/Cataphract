#include "fen.hpp"
#include "movegen.hpp"
#include "perft.hpp"

int main()
{
    initialize();
    std::string fen = "";
    while (fen != "quit") {
        std::cout << "Enter FEN:";
        std::getline(std::cin, fen);
        if (fen_parse(fen) == - 1) {
            std::cerr << "Invalid FEN.\n";
            continue;
        }
        std::cout << "Depth:";
        int depth;
        std::cin >> depth;
        while (!std::cin.good()) {
            std::cerr << "Invalid depth. Enter again:";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin >> depth;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        divide(depth);
    }
}