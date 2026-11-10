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
| **Work-Stealing Pool** | **20.80 ms** | [20.38 ms, 21.23 ms] | 2.19 ms | ~962,000 tasks/sec |
| **Mutex Shared-Queue Pool** | **28.86 ms** | [28.75 ms, 28.97 ms] | 0.55 ms | ~693,000 tasks/sec |

---

### 2. Thread Scaling on CPU Compute (10,000 compute-bound tasks)

| Worker Count | Work-Stealing Pool | WS Scaling (vs 1W) | Mutex Pool | Mutex Scaling (vs 1W) | WS vs Mutex Speedup |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **1 Worker** | 76.20 ms | 1.00x (Baseline) | 76.63 ms | 1.00x (Baseline) | 1.01x |
| **2 Workers** | 38.65 ms | **1.97x** | 39.99 ms | 1.92x | 1.03x |
| **4 Workers** | 20.36 ms | **3.74x** | 21.25 ms | 3.61x | 1.04x |
| **8 Workers** | 10.88 ms | **7.01x** | 13.80 ms | 5.55x | 1.27x |

---

### 3. Thread Scaling on Non-Uniform Tasks (10,000 compute-bound tasks)

Workload generated using a heavy-tailed exponential distribution (~5,000 mean iterations, range 100 to ~44,000 iterations).

| Worker Count | Work-Stealing Pool | WS Scaling (vs 1W) | Mutex Pool | Mutex Scaling (vs 1W) | WS vs Mutex Speedup |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **1 Worker** | 75.80 ms | 1.00x (Baseline) | 77.25 ms | 1.00x (Baseline) | 1.02x |
| **2 Workers** | 38.85 ms | **1.95x** | 39.77 ms | 1.94x | 1.02x |
| **4 Workers** | 20.29 ms | **3.74x** | 21.26 ms | 3.63x | 1.05x |
| **8 Workers** | 11.15 ms | **6.80x** | 13.57 ms | 5.69x | 1.22x |

---

### 4. Comparison with `std::async` (1,000 tasks)

| Implementation | Mean Time | 95% Confidence Interval | Std Deviation | Speedup vs `std::async` |
| :--- | :--- | :--- | :--- | :--- |
| **Work-Stealing Pool (Persistent)** | **1.12 ms** | [1.09 ms, 1.15 ms] | 0.14 ms | **10.04x** |
| **`std::async(std::launch::async)`** | **11.21 ms** | [11.11 ms, 11.34 ms] | 0.55 ms | 1.00x |

---

## Raw Catch2 Output

```text
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
benchmarks is a Catch2 v3.8.1 host application.
Run with -? for options

-------------------------------------------------------------------------------
Benchmark: High-throughput independent tasks
-------------------------------------------------------------------------------
/mnt/shared/common/Projects/personal_projects/thread-pool/bench/benchmarks.cpp:23
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Work-Stealing Pool (20 workers)                100             1     1.96742 s
                                        20.7991 ms    20.3786 ms     21.231 ms
                                        2.18637 ms    1.93837 ms      2.533 ms

Mutex Shared-Queue Pool (20
workers)                                       100             1     2.86018 s
                                        28.8554 ms    28.7534 ms    28.9683 ms
                                        551.096 us      478.2 us    637.911 us


-------------------------------------------------------------------------------
Benchmark: Thread count scaling on CPU compute
-------------------------------------------------------------------------------
/mnt/shared/common/Projects/personal_projects/thread-pool/bench/benchmarks.cpp:52
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Work-Stealing: 1 workers                       100             1     7.59613 s
                                        76.2029 ms    76.1109 ms    76.2715 ms
                                         400.88 us    273.792 us    690.497 us

Mutex Pool: 1 workers                          100             1     7.65618 s
                                        76.6306 ms    76.4154 ms    76.7156 ms
                                        652.356 us    295.784 us    1.38129 ms

Work-Stealing: 2 workers                       100             1     3.86513 s
                                        38.6472 ms    38.6125 ms    38.7037 ms
                                        222.105 us    158.001 us    366.026 us

Mutex Pool: 2 workers                          100             1     3.98917 s
                                        39.9871 ms    39.9346 ms    40.0727 ms
                                        337.349 us    239.404 us    533.021 us

Work-Stealing: 4 workers                       100             1     2.04194 s
                                        20.3636 ms    20.3491 ms    20.3921 ms
                                        99.3432 us    61.2881 us    188.464 us

Mutex Pool: 4 workers                          100             1     2.12336 s
                                        21.2453 ms    21.1772 ms    21.2698 ms
                                        194.426 us    82.1497 us    423.284 us

Work-Stealing: 8 workers                       100             1     1.09471 s
                                        10.8761 ms    10.7736 ms     10.994 ms
                                        561.581 us    487.668 us     639.91 us

Mutex Pool: 8 workers                          100             1     1.35481 s
                                        13.8005 ms     13.721 ms    13.8763 ms
                                        396.967 us     352.19 us    461.375 us


-------------------------------------------------------------------------------
Benchmark: Non-uniform tasks on CPU compute
-------------------------------------------------------------------------------
/mnt/shared/common/Projects/personal_projects/thread-pool/bench/benchmarks.cpp:105
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Work-Stealing: 1 workers                       100             1      7.6332 s
                                        75.7995 ms    75.7298 ms    75.8952 ms
                                        411.512 us     286.81 us    608.323 us

Mutex Pool: 1 workers                          100             1     7.73053 s
                                         77.252 ms    77.1612 ms    77.3719 ms
                                        528.191 us    417.313 us    693.012 us

Work-Stealing: 2 workers                       100             1     3.88898 s
                                        38.8483 ms    38.7487 ms    38.9734 ms
                                         565.21 us     464.21 us    752.333 us

Mutex Pool: 2 workers                          100             1     4.02004 s
                                        39.7713 ms    39.7405 ms    39.8108 ms
                                         176.04 us     139.73 us     233.15 us

Work-Stealing: 4 workers                       100             1     2.02871 s
                                        20.2948 ms    20.2005 ms    20.3362 ms
                                        303.389 us    124.104 us    546.702 us

Mutex Pool: 4 workers                          100             1     2.13625 s
                                        21.2589 ms    21.0883 ms     21.314 ms
                                        448.162 us    162.446 us    982.027 us

Work-Stealing: 8 workers                       100             1      1.1169 s
                                        11.1465 ms    11.0524 ms     11.261 ms
                                        529.162 us    446.249 us    620.478 us

Mutex Pool: 8 workers                          100             1     1.32815 s
                                        13.5728 ms    13.5066 ms    13.6449 ms
                                        352.412 us     305.36 us    412.019 us


-------------------------------------------------------------------------------
Benchmark: Comparison with std::async
-------------------------------------------------------------------------------
/mnt/shared/common/Projects/personal_projects/thread-pool/bench/benchmarks.cpp:85
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Work-Stealing Pool                             100             1    103.662 ms
                                        1.11658 ms    1.09108 ms    1.14737 ms
                                        142.495 us    123.495 us     187.41 us

std::async (std::launch::async)                100             1      1.1411 s
                                        11.2093 ms    11.1143 ms    11.3352 ms
                                        554.176 us    438.536 us    801.585 us
```
