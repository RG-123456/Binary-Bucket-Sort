# Binary Bucket Sort

[中文说明](README.zh-CN.md)

An experimental, header-only C++17 hybrid sort for `std::vector<std::uint64_t>`.
It selects counting sort for compact value ranges and otherwise partitions
values by their high binary bits before sorting each bucket with `std::sort`.

The project combines established sorting techniques and evaluates this specific
implementation on several integer distributions. Performance depends on the
input distribution, implementation, compiler, and hardware.

## Quick start

Requirements are a C++17 compiler and its standard library. The supplied build
script uses Bash and defaults to `g++`. It was tested with GCC 14.2 and libstdc++ 14
on Linux.

Run these commands from the project directory.

```bash
bash build.sh
./build/demo
./build/sort_tests
./build/benchmark
```

The demo prints `0 3 5 5 9 12 17 19`. The benchmark writes raw observations,
summary statistics, and metadata into `results/local/`.

To compile the demo directly with GCC or a compatible compiler, use

```bash
g++ -std=c++17 -O3 -DNDEBUG -Iinclude examples/demo.cpp -o demo
```

The sorting library has no third-party runtime dependencies. Python and
Matplotlib are optional tools for recreating the benchmark figure.

## Use in your code

Add `include/` to your compiler's include path.

```cpp
#include "binary_bucket_sort.hpp"
#include <cstdint>
#include <vector>

int main() {
    std::vector<std::uint64_t> values{19, 5, 17, 12, 3, 9, 5, 0};
    binary_bucket_sort::sort(values);
    // values now contains {0, 3, 5, 5, 9, 12, 17, 19}.
}
```

The function modifies the supplied vector, preserves duplicates, and supports
empty input and the full unsigned 64-bit range. Its API accepts unsigned integers
only. It allocates auxiliary storage in the counting and bucket branches. The
bucket branch uses `std::sort`, whose ordering of equivalent records is unstable.
This API sorts scalar values and provides no record-stability guarantee.

## Algorithm

Let `n` be the number of values, `minimum` their minimum, and
`difference = maximum - minimum`.

1. For at most 512 values, use `std::sort` directly.
2. Otherwise, obtain the minimum and maximum in one traversal.
3. If `difference + 1 <= 4 * n` and `difference + 1 <= 1,000,000`,
   count occurrences at offsets from the minimum, then write the values back.
4. Otherwise, retain up to eight high bits of `value - minimum` as a bucket ID.
5. Sort each bucket with `std::sort`, then concatenate buckets in increasing ID order.

The thresholds are experimental defaults. They can be edited in the header and
should be remeasured for a different workload.

For a range requiring 40 bits, the bucket expression is

```cpp
bucket_id = (value - minimum) >> 32;
```

Original values are stored in the buckets. Subtracting the same minimum preserves
order, and right-shifting a nonnegative value is monotone. Thus every value in a
lower-numbered bucket precedes every value in a higher-numbered bucket. Sorting
inside each bucket completes the ordering.

## Complexity

These bounds assume fixed-width integer operations take constant time.

| Branch | Time | Auxiliary space |
| --- | --- | --- |
| Small input | `O(n log n)` | Standard-library implementation dependent |
| Counting, range size `R` | `O(n + R)` | `O(R)` |
| High-bit buckets, sizes `s_i`, `B = 256` | `O(n + B + sum(s_i log(s_i + 1)))` | `O(n + B)` for bucket storage, plus bucket-sort workspace |

The bucket branch has a worst-case time bound of `O(n log n)`. When most values
land in the same bucket, partitioning adds work with little reduction in the
subsequent comparison sort. An approximately even split reduces the comparison
work to roughly `n log(n / B)`, in addition to partitioning and copying.

## Reproducible benchmark

The executable compares three single-threaded C++ implementations on identical
`std::vector<std::uint64_t>` inputs.

| Name | Implementation |
| --- | --- |
| Binary Bucket Sort | This header, including its counting-sort branch |
| `std::sort` | The compiler's standard-library implementation |
| LSD radix, 8-bit | The supplied implementation, using a contiguous buffer and 256 counters per pass |

The radix baseline normalizes by the minimum and processes the necessary bits
in eight-bit passes. Its code is included in `benchmarks/benchmark.cpp`.

The default workload uses 100,000 integers, seeds 20261117–20261119, and seven
timed repetitions per seed. Each algorithm runs once as a warmup for each input.
Execution order is shuffled in every repetition. Every result is checked against
`std::sort`, including duplicate counts.

Timing starts after making a fresh input copy and ends when the sorting function
returns. Range scans, bucket construction, internal allocation, and internal
deallocation are included. Data generation, input copies, result verification,
and CSV serialization are outside the timed region. There is no CPU pinning or
frequency control. Timings describe repeated runs in a hosted environment.

The table reports the median pooled across 21 observations per method and
distribution. The CSV also reports the sorted observations at indices `m/4` and
`3*m/4` as p25 and p75. Results depend on the compiler and standard library;
`std::shuffle` may produce different orders across implementations.

| Dataset | Generation |
| --- | --- |
| Uniform 40-bit | Random values in `[0, 2^40)` |
| Dense | `2^39` plus a random offset in `[0, 1000]` |
| Clustered with outliers | 99,900 values near `2^39`, plus 100 random 40-bit outlier draws |
| Repeated | Random choices from 100 preselected 40-bit values |
| Already sorted | The uniform dataset sorted ascending |
| Nearly sorted | The sorted uniform dataset after 100 random index swaps |

### Included reference run

The packaged source was compiled with GCC 14.2, libstdc++ 14, C++17, and
`-O3 -DNDEBUG`. The bundled raw data contains 378 timed sorts. All matched the
reference output. Values below are milliseconds.

| Dataset | Binary Bucket Sort | `std::sort` | LSD radix |
| --- | ---: | ---: | ---: |
| Uniform 40-bit | 3.173 | 5.032 | **0.945** |
| Dense | **0.242** | 3.694 | 0.538 |
| Clustered with outliers | 5.222 | 4.681 | **0.941** |
| Repeated | 0.952 | 2.740 | **0.847** |
| Already sorted | **0.648** | 0.750 | 0.858 |
| Nearly sorted | 0.970 | **0.733** | 0.850 |

![C++ sorting benchmark](results/reference/comparison.png)

The dense-input advantage comes from counting sort. In this run, radix sort is
faster on uniform, clustered, and repeated inputs. The hybrid outperforms
`std::sort` on four of the six distributions. These observations apply to the
included implementations and workload; a general performance ranking needs
measurements across sizes, distributions, machines, and other optimized libraries.

Raw results, summary statistics, and environment information are included below.

- [Individual observations](results/reference/runs.csv)
- [Summary statistics](results/reference/summary.csv)
- [Run metadata](results/reference/metadata.txt)
- [Build environment](results/reference/environment.txt)

To rerun into a separate directory and draw a new figure, use

```bash
./build/benchmark results/local
python3 -m pip install matplotlib
python3 tools/plot_results.py results/local/summary.csv results/local/comparison.png
```

The benchmark overwrites the named result files in its output directory. To
change the workload, edit `data_size`, `seed_count`, `repeats`, and `first_seed`
near the top of `benchmarks/benchmark.cpp`, then rebuild. `data_size` must be at
least 100 for the outlier generator.

## Correctness checks

`tests/sort_tests.cpp` contains 116 comparisons against `std::sort`, covering
empty input, duplicates, sorted and reversed data, full-width random values,
`UINT64_MAX`, dense ranges near the upper bound, both sides of the counting
thresholds, and all bit widths from 1 through 64. Checks use explicit failures
and remain active in Release builds.

```bash
./build/sort_tests
```

GCC and Clang users with sanitizer support can additionally run

```bash
g++ -std=c++17 -O1 -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Iinclude tests/sort_tests.cpp \
    -o build/sort_tests_sanitized
./build/sort_tests_sanitized
```

The reference run passed the address and undefined-behavior checks with
`ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer was unavailable under the hosted
environment's tracing configuration, so memory-leak detection remains unverified.

## Project files

| Path | Purpose |
| --- | --- |
| `include/binary_bucket_sort.hpp` | Header-only sorting implementation |
| `examples/demo.cpp` | Minimal usage example |
| `benchmarks/benchmark.cpp` | Reproducible C++ benchmark and radix baseline |
| `tests/sort_tests.cpp` | Correctness checks |
| `tools/plot_results.py` | Optional figure generation |
| `results/reference/` | Included measurements and figure |
| `build.sh` | GCC-compatible build commands |

## Author and license

Runde Gao. Distributed under the MIT License. [LICENSE](LICENSE)
