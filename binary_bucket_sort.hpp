#ifndef BINARY_BUCKET_SORT_HPP
#define BINARY_BUCKET_SORT_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace binary_bucket_sort {

namespace detail {

// Experimental defaults used by the accompanying benchmark.
inline constexpr std::size_t small_sort_threshold = 512;
inline constexpr std::uint64_t maximum_counting_range = 1'000'000;
inline constexpr unsigned bucket_bits = 8;

inline unsigned bit_length(std::uint64_t value) noexcept {
    unsigned length = 0;
    while (value != 0) {
        ++length;
        value >>= 1;
    }
    return length;
}

}  // namespace detail

// Sort a vector of unsigned 64-bit integers in ascending order.
// The input vector is modified. Counting and bucket branches allocate memory.
inline void sort(std::vector<std::uint64_t>& data) {
    if (data.size() <= detail::small_sort_threshold) {
        std::sort(data.begin(), data.end());
        return;
    }

    const auto bounds = std::minmax_element(data.begin(), data.end());
    const std::uint64_t minimum = *bounds.first;
    const std::uint64_t difference = *bounds.second - minimum;

    // difference / 4 < size is equivalent to difference < 4 * size,
    // while avoiding overflow in the multiplication.
    if (difference < detail::maximum_counting_range &&
        difference / 4 < data.size()) {
        std::vector<std::size_t> counts(
            static_cast<std::size_t>(difference) + 1, 0);

        for (const std::uint64_t value : data) {
            ++counts[static_cast<std::size_t>(value - minimum)];
        }

        std::size_t output = 0;
        for (std::size_t offset = 0; offset < counts.size(); ++offset) {
            for (std::size_t i = 0; i < counts[offset]; ++i) {
                data[output++] = minimum + offset;
            }
        }
        return;
    }

    const unsigned length = detail::bit_length(difference);
    const unsigned shift = length > detail::bucket_bits
        ? length - detail::bucket_bits
        : 0;
    std::array<std::vector<std::uint64_t>, 256> buckets;

    // Normalization changes only the bucket calculation. Store original values.
    for (const std::uint64_t value : data) {
        const std::size_t bucket = (value - minimum) >> shift;
        buckets[bucket].push_back(value);
    }

    // Bucket IDs increase with value, so sorted buckets concatenate in order.
    auto output = data.begin();
    for (auto& bucket : buckets) {
        std::sort(bucket.begin(), bucket.end());
        output = std::copy(bucket.begin(), bucket.end(), output);
    }
}

}  // namespace binary_bucket_sort

#endif  // BINARY_BUCKET_SORT_HPP
