#include <iostream>
#include <vector>
#include <cmath>

#ifdef USE_DOUBLE
using real_t = double;
#else
using real_t = float;
#endif

int main(){
    const std::size_t N = 10000000;
    const real_t two_pi = static_cast<real_t>(2.0 * M_PI);

    std::vector<real_t> data(N);

    real_t sum = 0;

    for (std::size_t i = 0; i < N; ++i){
        real_t value = std::sin(two_pi * static_cast<real_t>(i) / static_cast<real_t>(N));
        sum += value;
    }

    std::cout << "Type: "
#ifdef USE_DOUBLE
              << "double\n";
#else
              << "float\n";
#endif
    std::cout << "Sum = " << sum << std::endl;

    return 0;
}