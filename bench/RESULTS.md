# Benchmark Results

Raw benchmark data recorded from Catch2 test runs.

---

## Environment

- **CPU:** 13th Gen Intel(R) Core(TM) i5-13500HX (14 cores, 20 logical threads)
- **OS:** Linux x86_64
- **Compiler:** GCC 16.2.1 (`-O3 -DNDEBUG -std=gnu++23`)
- **Harness:** Catch2 v3.8.1 (100 samples per benchmark)
- **Command:** `make bench`

---

## Summary Tables

### 1. High-Throughput Independent Tasks (20,000 tasks, 20 workers)

| Architecture | Mean Time | 95% Confidence Interval | Std Deviation | Throughput |
| :--- | :--- | :--- | :--- | :--- |
| **Work-Stealing Pool** | **21.50 ms** | [21.16 ms, 21.84 ms] | 1.74 ms | ~930,000 tasks/sec |
| **Mutex Shared-Queue Pool** | **29.72 ms** | [29.35 ms, 30.23 ms] | 2.18 ms | ~672,000 tasks/sec |

---

### 2. Thread Scaling on CPU Compute (200 compute-bound tasks)

| Worker Count | Work-Stealing Pool | Mutex Pool | Work-Stealing Speedup |
| :---: | :---: | :---: | :---: |
| **1 Worker** | 152.07 ms | 156.05 ms | 1.00x |
| **2 Workers** | 76.66 ms | 77.66 ms | 1.98x |
| **4 Workers** | 32.94 ms | 31.44 ms | 4.61x |
| **8 Workers** | 10.75 ms | 11.82 ms | 14.14x |

---

### 3. Comparison with `std::async` (200 tasks)

| Implementation | Mean Time | 95% Confidence Interval | Std Deviation | Speedup vs `std::async` |
| :--- | :--- | :--- | :--- | :--- |
| **Work-Stealing Pool** | **1.54 ms** | [1.51 ms, 1.58 ms] | 0.18 ms | 2.04x |
| **`std::async(std::launch::async)`** | **3.14 ms** | [3.03 ms, 3.27 ms] | 0.59 ms | 1.00x |

---

## Raw Catch2 Output

```text
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
benchmarks is a Catch2 v3.8.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark: High-throughput independent tasks
-------------------------------------------------------------------------------
bench/benchmarks.cpp:24
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Work-Stealing Pool (20 workers)                100             1     2.21468 s
                                         21.499 ms    21.1568 ms    21.8446 ms
                                         1.7431 ms    1.52142 ms     2.0682 ms

Mutex Shared-Queue Pool (20
workers)                                       100             1      2.7813 s
                                        29.7209 ms    29.3531 ms    30.2273 ms
                                        2.18117 ms    1.68125 ms    2.93218 ms


-------------------------------------------------------------------------------
Benchmark: Thread count scaling on CPU compute
-------------------------------------------------------------------------------
bench/benchmarks.cpp:53
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Work-Stealing: 1 workers                       100             1     15.6416 s
                                        152.065 ms    149.492 ms    153.666 ms
                                        10.1394 ms    7.21693 ms    17.4596 ms

Mutex Pool: 1 workers                          100             1     15.9196 s
                                        156.053 ms    154.744 ms    157.206 ms
                                        6.25881 ms    5.21349 ms    7.50303 ms

Work-Stealing: 2 workers                       100             1     7.65059 s
                                        76.6637 ms    75.6632 ms    77.4923 ms
                                        4.61656 ms    3.71051 ms    6.37024 ms

Mutex Pool: 2 workers                          100             1     7.53428 s
                                        77.6645 ms    76.4967 ms    78.7254 ms
                                        5.67661 ms    4.83291 ms    6.75672 ms

Work-Stealing: 4 workers                       100             1     3.91306 s
                                        32.9447 ms     31.673 ms    34.1652 ms
                                         6.3266 ms    5.51536 ms    7.53215 ms

Mutex Pool: 4 workers                          100             1     3.06985 s
                                        31.4393 ms     29.885 ms     32.847 ms
                                        7.52075 ms    6.71957 ms    8.35867 ms

Work-Stealing: 8 workers                       100             1     1.24479 s
                                         10.755 ms    10.4091 ms    11.2033 ms
                                        1.99886 ms    1.55785 ms    2.80436 ms

Mutex Pool: 8 workers                          100             1     1.05124 s
                                        11.8191 ms    11.3762 ms    12.3084 ms
                                        2.37113 ms    2.06213 ms    2.92963 ms


-------------------------------------------------------------------------------
Benchmark: Comparison with std::async
-------------------------------------------------------------------------------
bench/benchmarks.cpp:86
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Work-Stealing Pool                             100             1    173.241 ms
                                        1.53623 ms    1.50767 ms    1.57876 ms
                                        175.249 us    131.132 us    287.954 us

std::async (std::launch::async)                100             1    382.595 ms
                                        3.14025 ms    3.03403 ms     3.2683 ms
                                          592.2 us    507.282 us      688.2 us
```
