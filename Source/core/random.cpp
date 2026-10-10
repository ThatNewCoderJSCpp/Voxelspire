#include "core/random.hpp"

namespace voxelspire {

SeededRandom::SeededRandom() : SeededRandom(SecureRandom::integer_or<std::uint64_t>(1, std::numeric_limits<std::uint64_t>::max(), FALLBACK_SEED)) {}

SeededRandom SeededRandom::at(std::uint64_t seed, std::int64_t x, std::int64_t y, std::int64_t z, std::uint64_t salt) noexcept {
    std::uint64_t h = mix(seed ^ salt);
    h = mix(h ^ static_cast<std::uint64_t>(x));
    h = mix(h ^ static_cast<std::uint64_t>(y));
    h = mix(h ^ static_cast<std::uint64_t>(z));
    return SeededRandom(h);
}

std::uint64_t SeededRandom::next() noexcept {
    std::uint64_t x = m_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    m_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

double SeededRandom::unit() noexcept { return static_cast<double>(next() >> (64 - SecureRandom::UNIT_BITS)) * SecureRandom::UNIT_SCALE; }

std::uint64_t SeededRandom::mix(std::uint64_t x) noexcept {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

} // namespace voxelspire
