#include <random>
#include <type_traits>

class generator {
public:
    template<typename T>
    static T
    rand(T lo, T hi) requires std::is_arithmetic_v<T>
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static uniform_real_distribution<> dist(0.0f, 1.0f);
        return static_cast<T>((hi - lo) * dist(gen) + lo);
    }
};
