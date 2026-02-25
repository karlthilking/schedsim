#include <array>
#include <random>
#include <algorithm>
#include <cstdlib>
#include <cassert>

int
main(int argc, char *argv[])
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1024.0f, 1024.0f);

    std::array<std::array<float, 16>, 16> A;
    std::array<std::array<float, 16>, 16> B;

    std::for_each(begin(A), end(A), [&](std::array<float, 16> &a){
        std::generate(begin(a), end(a), [&]{
            return dist(gen);
        });
    });
    std::for_each(begin(B), end(B), [&](std::array<float, 16> &b){
        std::generate(begin(b), end(b), [&]{
            return dist(gen);
        });
    });
    
    int N = (argc > 1) ? std::stoi(argv[1]) : 42000;
    while (N--) {
        std::array<std::array<float, 16>, 16> C{};
        assert(std::all_of(begin(C), end(C), [&](std::array<float, 16> &c){
            return std::all_of(begin(c), end(c), [&](float f){
                return f == 0.0f;
            });
        }));
        for (int i = 0; i < 16; ++i)
            for (int k = 0; k < 16; ++k)
                for (int j = 0; j < 16; ++j)
                    C[i][j] += A[i][k] * B[k][j];
    }
    exit(0);
}
