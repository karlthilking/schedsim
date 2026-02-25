#include <random>
#include <vector>
#include <string>
#include <cstdlib>

int
main(int argc, char *argv[])
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, 4095);

    std::vector<std::string> v(4096, "01010");
    int N = (argc > 1) ? std::stoi(argv[1]) : 1 << 12;
    while (N--) {
        for (int n = 0; n < 1 << 10; ++n) {
            size_t i0 = dist(gen);
            size_t i1 = dist(gen);
            size_t i2 = dist(gen);
            size_t i3 = dist(gen);
            std::string s0 = v[i0];
            std::string s1 = v[i1];
            std::string s2 = v[i2];
            std::string s3 = v[i3];
            s0[0] = s1[0];
            s1[0] = s2[0];
            s2[0] = s3[0];
            s3[0] = s1[0];
        }
    }
    exit(0);
}
