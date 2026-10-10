#ifndef VOXELSPIRE_CORE_RANDOM_HPP
#define VOXELSPIRE_CORE_RANDOM_HPP

#include <cstdint>
#include <limits>
#include <type_traits>
#include "../fizmo.hpp"

namespace voxelspire {

template <typename T>
using RandomInteger = typename std::enable_if<std::is_integral<T>::value && !std::is_same<typename std::remove_cv<T>::type, bool>::value, T>::type;

class SecureRandom {
public:
    static constexpr int           UNIT_BITS  = 53;
    static constexpr std::uint64_t UNIT_MASK  = (std::uint64_t(1) << UNIT_BITS) - 1;
    static constexpr double        UNIT_SCALE = 1.0 / static_cast<double>(std::uint64_t(1) << UNIT_BITS);

    template <typename T>
    static RandomInteger<T> integer(T lo = T(0), T hi = std::numeric_limits<T>::max()) { return fizmo::random_int<T>(lo, hi); }

    template <typename T>
    static RandomInteger<T> integer_or(T lo, T hi, T fallback) noexcept { return fizmo::random_int_nothrow<T>(lo, hi, fallback); }

    static double unit() { return static_cast<double>(integer<std::uint64_t>(0, UNIT_MASK)) * UNIT_SCALE; }
    static double range(double lo, double hi) { return lo + (hi - lo) * unit(); }
    static bool   chance(double p) { return unit() < p; }

    static std::uint64_t seed() { return integer<std::uint64_t>(1, std::numeric_limits<std::uint64_t>::max()); }
};

class SeededRandom {
public:
    static constexpr std::uint64_t FALLBACK_SEED = 0x9E3779B97F4A7C15ull;

    SeededRandom();
    explicit SeededRandom(std::uint64_t seed) noexcept { reseed(seed); }

    static SeededRandom at(std::uint64_t seed, std::int64_t x, std::int64_t y, std::int64_t z = 0, std::uint64_t salt = 0) noexcept;

    void reseed(std::uint64_t seed) noexcept {
        m_state = mix(seed);
        if (m_state == 0) m_state = FALLBACK_SEED;
    }

    std::uint64_t next() noexcept;

    template <typename T>
    RandomInteger<T> integer(T lo = T(0), T hi = std::numeric_limits<T>::max()) noexcept {
        if (lo > hi) { const T t = lo; lo = hi; hi = t; }
        if (lo == hi) return lo;
        using U = typename std::make_unsigned<T>::type;
        const U span = static_cast<U>(static_cast<U>(hi) - static_cast<U>(lo));
        U value;

        if (span == std::numeric_limits<U>::max()) {
            value = static_cast<U>(next());
        } else {
            const U bound = static_cast<U>(span + U(1));
            const U threshold = static_cast<U>(static_cast<U>(U(0) - bound) % bound);
            do { value = static_cast<U>(next()); } while (value < threshold);
            value = static_cast<U>(value % bound);
        }

        return static_cast<T>(static_cast<U>(static_cast<U>(lo) + value));
    }

    double unit() noexcept;
    double range(double lo, double hi) noexcept { return lo + (hi - lo) * unit(); }
    double signed_unit() noexcept { return unit() * 2.0 - 1.0; }
    bool   chance(double p) noexcept { return unit() < p; }

    static std::uint64_t mix(std::uint64_t x) noexcept;

private:
    std::uint64_t m_state = FALLBACK_SEED;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_RANDOM_HPP