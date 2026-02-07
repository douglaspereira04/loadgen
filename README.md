# loadgen
YCSB-style request workload generator for benchmarking kvstores.

## Documentation

- **Getting started**: build with `build.sh` and run the optional `gen` executable to emit workloads based on the `samples/workloads/*.toml` definitions.
- **Workload format & configuration**: defined in the `workload` section of the TOML input parsed by `workload::RequestGenerator` (`src/request/request_generator.h`).
- **Distributions**: `zipfian_int_distribution`, `scrambled_zipfian_int_distribution`, and `skewed_latest_int_distribution` live under `src/request` and mirror the hot/key access patterns produced by Repart-KV workloads.
- **Repository layout**: described in the section below.

## Build

### Using `build.sh`

`build.sh` wraps CMake configuration, formats the source tree with `clang-format`, and builds the static library (and optional `gen` executable). It also exposes a few flags:

```bash
./build.sh              # configure/build loadgen-core in Release mode (default)
./build.sh -d           # switch to Debug mode
./build.sh -g           # build the optional workload generator executable (gen)
./build.sh -h           # list options
```

The script always runs `clang-format` on `.h/.hpp/.cpp` files outside of `build/`, `.git/`, and `external/`, so ensure `clang-format` is installed if you rely on the script.

### Manual build

Alternatively, configure CMake directly and build only what you need:

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_LOADGEN_GEN=ON ..
cmake --build . -j"$(nproc)"
```

Link against `libloadgen-core.a` or build the optional `gen` tool by enabling `BUILD_LOADGEN_GEN` in this step.

### FetchContent-friendly integration

Long-lived targets are exported through the `loadgen::core` alias, making this project compatible with `FetchContent`.

```cmake
include(FetchContent)

FetchContent_Declare(
    loadgen
    GIT_REPOSITORY https://github.com/douglaspereira04/loadgen.git
    GIT_TAG main
)

FetchContent_MakeAvailable(loadgen)

target_link_libraries(myapp PRIVATE loadgen::core)
```

## Generating workloads

The optional `gen` executable reads a TOML configuration file and generates requests to the `output.requests.export_path` value:

```bash
./build/bin/gen samples/workloads/ycsb_a.toml
```

When `gen` is skipped, you can still consume `workload::RequestGenerator` in your own test harness by linking directly against `loadgen-core` and invoking `generate_to_file()` with any TOML path.

## Workload configuration

Each workload TOML file defines the phases and randomness seeds that control the produced requests. Common keys include:

- `workload.n_records`, `workload.n_operations`: bounds for the loading and operational phases.
- `workload.key_seed`, `workload.operation_seed`, `workload.scan_seed`: deterministic seeds for data, operation mix, and scan length generation.
- `workload.data_distribution`: `UNIFORM`, `ZIPFIAN`, or `LATEST` (`str_to_dist` selects the corresponding RNG).
- `workload.read_proportion`, `workload.update_proportion`, `workload.insert_proportion`, `workload.scan_proportion`: weights normalized by the generator to define the operation mix.
- `workload.scan_length_distribution`: distribution used for scan sizes.
- `workload.gen_values`, `workload.value_min_size`, `workload.value_max_size`: controls value generation; when `gen_values` is true, `CharGenerator` and `len_generator_` produce strings within the configured size range.
- `output.requests.export_path`: path where `generate_to_file()` dumps the CSV-style workload (mirroring YCSB operations; see the “Workload format” section below).

`request_generator.cpp` splits execution into `LOADING` (write keys `0 … n_records-1`) and `OPERATIONS`. During operations, reads/updates/single-key writes draw keys from `data_generator_`, scans sample ranges, and writes advance the internal `acknowledged_counter<long> insert_key_sequence_` to keep the “latest” distribution consistent.

## Workload format

The exported workload is a CSV file whose rows map to YCSB-style operations:

```
0,<key>                 # READ
1,<key>[,<value>]       # WRITE (value optional if gen_values=false)
2,<start_key>,<limit>   # SCAN (lower bound + result limit)
```

Use `samples/workloads/*.toml` to explore how proportions and seeds influence the generated CSV.

## Distributions

The `src/request` directory implements the statistical distributions Repart-KV relies on:

- `zipfian_int_distribution`: classic Zipfian distribution with caching for `lastvalue`, used for heavy-tailed access patterns.
- `scrambled_zipfian_int_distribution`: reorders Zipfian outputs with FNV-1a hashing to avoid hotspotting while preserving cumulative weights.
- `skewed_latest_int_distribution`: builds on a `zipfian_int_distribution` plus an `acknowledged_counter` so read/update keys skew toward the most recently inserted records, similar to Repart-KV latest workloads.

Copy constructors now preserve `lastvalue` (preventing `-Wuninitialized` warnings) and all distributions start with predictable seeds, ensuring deterministic repeats of Repart-KV-style traces.

## Repository layout

```
loadgen/
  build/                      # CMake build outputs & binaries
  samples/
    workloads/                # Example YCSB A / D / E TOML files
  external/
    toml11/                   # TOML parser used by RequestGenerator
  src/
    request/                   # RNG helpers, ACK counter, request generator
    types/                     # shared YCSB operation/type helpers
  build.sh                    # Build script wrapping CMake + formatting
  CMakeLists.txt              # Root project definition
```

## Samples

The `samples/workloads` directory contains canonical scenarios inspired by YCSB:

- `ycsb_a.toml`: 50/50 read/update mix with Zipfian keys.
- `ycsb_d.toml`: read-heavy (95%) profile with scans enabled.
- `ycsb_e.toml`: read+insert mix to stress insertion-heavy workloads.

Each TOML file sets `output.requests.export_path` so `gen` writes a CSV file you can feed into your datastore benchmark.

## Linking & embedding

Link `loadgen-core` into your own toolchain (as shown in `src/CMakeLists.txt`). The library exposes the `workload::RequestGenerator` API. You can instantiate it with any TOML path, configure the seeds for reproducibility, and call `generate_to_file()` to persist the resulting trace or host it in memory.

## Dependencies

- **Required**: CMake ≥ 3.20, a C++11 toolchain, `clang-format` (for `build.sh`), and the bundled `toml11` submodule.
- **Optional**: `BUILD_LOADGEN_GEN` for the standalone generator binary; otherwise, you can embed the generator API directly.
