# Binary Bucket Sort

[English](README.md)

一个用于 `std::vector<std::uint64_t>` 的 C++17 混合排序实验项目。
数值范围较小时使用计数排序，其他情况按二进制高位分桶，再在每个桶内调用
`std::sort`。

项目结合已有的排序技术，通过可复现的实验研究这种组合在不同整数分布上的表现。
实际速度取决于数据分布、实现方式、编译器和硬件。

## 快速运行

需要 C++17 编译器及标准库。构建脚本需要 Bash，默认使用 `g++`。
本项目已在 Linux、GCC 14.2、libstdc++ 14 下验证。

进入项目根目录，执行以下命令。

```bash
bash build.sh
./build/demo
./build/sort_tests
./build/benchmark
```

示例输出为 `0 3 5 5 9 12 17 19`。基准程序会将每次运行的原始记录、汇总统计和
元数据写入 `results/local/`。

也可以直接编译示例。

```bash
g++ -std=c++17 -O3 -DNDEBUG -Iinclude examples/demo.cpp -o demo
```

排序代码只依赖 C++ 标准库。Python 和 Matplotlib 仅用于可选的图表生成。

## 在自己的代码中使用

将 `include/` 加入编译器头文件搜索路径。

```cpp
#include "binary_bucket_sort.hpp"
#include <cstdint>
#include <vector>

int main() {
    std::vector<std::uint64_t> values{19, 5, 17, 12, 3, 9, 5, 0};
    binary_bucket_sort::sort(values);
    // 排序后为 {0, 3, 5, 5, 9, 12, 17, 19}。
}
```

函数修改传入的数组，保留重复值，支持空数组和完整的64位无符号整数范围。
接口仅接受无符号整数。计数和分桶分支会分配额外空间。
当前接口处理整数值，未提供用于带附加字段记录的稳定排序接口。

## 算法流程

设数组长度为 `n`，最小值为 `minimum`，最大差距为
`difference = maximum - minimum`。

1. 数量不超过512时，直接调用 `std::sort`。
2. 否则一次遍历获得最小值与最大值。
3. 当值域大小 `difference + 1` 同时不超过 `4 * n` 和1,000,000时，
   对每种数值计数，再按顺序写回。
4. 其他情况提取 `value - minimum` 的高位，分成最多256个桶。
5. 各桶分别调用 `std::sort`，按桶号从小到大拼接。

512、4倍、100万个计数桶和8位分桶是实验参数，其他数据规模或分布下需要重新评估。
这些参数可在头文件中修改。

例如最大差距需要40位时，桶号计算为

```cpp
bucket_id = (value - minimum) >> 32;
```

桶内保存原始数值。所有数字减去相同的最小值会保持大小顺序，非负整数右移也是
单调的。因此，小桶号中的数值先于大桶号中的数值，桶内排好后拼接即可得到完整顺序。

## 复杂度

以下分析假设固定宽度整数运算耗时为常数。

| 分支 | 时间复杂度 | 额外空间 |
| --- | --- | --- |
| 小数组直接排序 | `O(n log n)` | 由标准库实现决定 |
| 计数排序，值域大小为 `R` | `O(n + R)` | `O(R)` |
| 高位分桶，每桶大小为 `s_i`，`B = 256` | `O(n + B + sum(s_i log(s_i + 1)))` | 桶存储为 `O(n + B)`，另加桶内排序工作空间 |

高位分桶分支最坏仍为 `O(n log n)`。当大量数字进入同一个桶时，分桶增加的工作
可能超过它节省的比较次数。若各桶较均匀，比较工作量大致降低到 `n log(n / B)`，
此外仍有分桶和复制的成本。

## 基准测试

三种实现统一使用 C++、单线程和 `std::vector<std::uint64_t>`。

| 方法 | 实现 |
| --- | --- |
| Binary Bucket Sort | 本项目头文件，包含计数排序分支 |
| `std::sort` | 当前编译器提供的标准库排序 |
| LSD基数排序 | 每轮处理8位，使用连续缓冲区和256个计数项 |

基数排序也先减去最小值以减少需要处理的位数，其实现位于
`benchmarks/benchmark.cpp`。

默认每组100,000个数，随机种子为20261117至20261119，每个种子计时7轮。
每种方法先预热一次，每轮随机调整方法执行顺序。所有结果都与 `std::sort` 完整
比较，包含数值和重复次数。

计时从复制好的输入开始，到排序函数返回结束。找最大最小值、建桶、内部内存
分配和释放都计时。数据生成、输入复制、正确性检查和CSV写入不计时。
本次使用托管运行环境，没有固定CPU核心或控制频率。

每项结果汇总21次测量，表中给出中位数。CSV中的p25和p75取排序后第 `m/4`、
`3*m/4` 个位置。不同编译器或标准库中的 `std::shuffle` 可能产生不同顺序。

| 数据类型 | 生成方式 |
| --- | --- |
| 均匀40位整数 | 从 `[0, 2^40)` 生成随机值 |
| 密集值域 | `2^39` 加上0至1000的随机偏移 |
| 集中并有离群值 | 99,900个数集中在 `2^39` 附近，另加入100个随机40位值 |
| 大量重复 | 从预先生成的100个40位数中随机取值 |
| 已经有序 | 将均匀数据提前升序排列 |
| 近乎有序 | 在已排序数据中随机交换下标100次 |

### 本项目附带的实测记录

打包代码使用 GCC 14.2、libstdc++ 14、C++17 和 `-O3 -DNDEBUG` 编译。
原始记录包含378次计时排序，全部结果正确。下表单位为毫秒。

| 数据类型 | 我们的混合排序 | `std::sort` | LSD基数排序 |
| --- | ---: | ---: | ---: |
| 均匀40位整数 | 3.173 | 5.032 | **0.945** |
| 密集值域 | **0.242** | 3.694 | 0.538 |
| 集中并有离群值 | 5.222 | 4.681 | **0.941** |
| 大量重复 | 0.952 | 2.740 | **0.847** |
| 已经有序 | **0.648** | 0.750 | 0.858 |
| 近乎有序 | 0.970 | **0.733** | 0.850 |

![C++排序实测对比](results/reference/comparison.png)

密集数据的优势来自计数排序分支。本次基数排序在均匀、集中和重复数据中最快，
我们的混合实现则在六类分布中的四类超过直接调用 `std::sort`。
这些结论适用于附带的实现和测试条件。更广泛的结论需要增加规模、分布、硬件及
其他优化实现的对照。

原始数据和环境记录随项目提供。

- [每次运行的数据](results/reference/runs.csv)
- [汇总统计](results/reference/summary.csv)
- [运行元数据](results/reference/metadata.txt)
- [编译及运行环境](results/reference/environment.txt)

可以重新运行并生成自己的图表。

```bash
./build/benchmark results/local
python3 -m pip install matplotlib
python3 tools/plot_results.py results/local/summary.csv results/local/comparison.png
```

程序会覆盖指定输出目录中同名的结果文件。要调整数据量和重复次数，可编辑
`benchmarks/benchmark.cpp` 顶部的 `data_size`、`seed_count`、`repeats` 和
`first_seed`，然后重新编译。离群值生成器要求 `data_size` 至少为100。

## 正确性检查

`tests/sort_tests.cpp` 包含116项对照检查，覆盖空数组、重复值、升序和逆序、
全64位随机整数、最大无符号整数、接近上限的密集区间、计数分支阈值两侧，
以及1至64位的全部位宽。检查在Release构建下也会执行。

```bash
./build/sort_tests
```

支持sanitizer的GCC或Clang环境还可以执行地址和未定义行为检查。

```bash
g++ -std=c++17 -O1 -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Iinclude tests/sort_tests.cpp \
    -o build/sort_tests_sanitized
./build/sort_tests_sanitized
```

本次环境中，地址及未定义行为检查在 `ASAN_OPTIONS=detect_leaks=0` 下通过。
环境的跟踪配置使LeakSanitizer无法运行，因此本次未验证内存泄漏检测。

## 文件结构

| 文件 | 用途 |
| --- | --- |
| `include/binary_bucket_sort.hpp` | 可直接使用的头文件 |
| `examples/demo.cpp` | 使用示例 |
| `benchmarks/benchmark.cpp` | 基准测试和基数排序对照 |
| `tests/sort_tests.cpp` | 正确性检查 |
| `tools/plot_results.py` | 可选图表生成脚本 |
| `results/reference/` | 随项目附带的实测数据与图表 |
| `build.sh` | GCC兼容的编译命令 |

## 作者与许可

Runde Gao。项目采用MIT许可证。[LICENSE](LICENSE)
