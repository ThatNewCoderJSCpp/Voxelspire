#define FIZMO
#define ALL_FIZMO
#include <fizmo/includes.hpp>

#include <iostream>

int main() {
    std::cout << "fizmo included and linked OK\n";
    std::cout << "C++ standard: " << __cplusplus << '\n';

#if defined(_WIN32)
    std::cout << "Platform: Windows\n";
#else
    std::cout << "Platform: Linux\n";
#endif

    return 0;
}