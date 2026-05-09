# argus

<a name="readme-top"></a>

<!-- PROJECT SHIELDS -->

[![C++23][cpp-shield]][cpp-url]
[![MIT License][license-shield]][license-url]

## Table of Contents

- [About The Project](#about-the-project)
- [Features](#features)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Dependencies](#dependencies)
- [Building](#building)
    - [Quick Start](#quick-start)
    - [Available Presets](#available-presets)
    - [Manual configure](#manual-configure)
- [CMake Options](#cmake-options)
- [Installing Dependencies](#installing-dependencies)
    - [Ubuntu / Debian](#ubuntu-debian)
    - [Arch Linux](#arch-linux)
    - [Fedora / RHEL](#fedora-rhel)
    - [macOS (Homebrew)](#macos-homebrew)
    - [AdaptiveCpp](#adaptivecpp)
    - [Windows](#windows)
- [Makefile](#makefile)
- [Testing](#testing)
- [Contact](#contact)
- [License](#license)

<a name="about-the-project"></a>

## About The Project

**argus** is a software tool that takes LiDAR parameters (viewing angle, range, scanning pattern) and target scene characteristics as input and outputs the optimal mirror position and tilt angle for maximum blind spot coverage.

<a name="features"></a>

## Features

- Proper raytracing calculation using SYCL (via AdaptiveCpp)
- C API bindings
- Python bindings (via pybind11)
- Optional renderer visualization (via Raylib)
- Extensible modular architecture

<a name="architecture"></a>

## Architecture

```
bindings (capi, python)
       |
    renderer
       |
      algo
       |
     common
       |
      core
```

| Module       | Description                                                         |
| ------------ | ------------------------------------------------------------------- |
| **core**     | Low-level primitives: assertions, logging, containers, stacktraces  |
| **common**   | Shared types, config management, scene loading                      |
| **algo**     | SYCL raytracing computation via AdaptiveCpp, optimization algorithm |
| **renderer** | Visualization frontend (optional: can link algo)                    |
| **bindings** | C API and Python bindings exposing algorithm results                |

<a name="prerequisites"></a>

## Prerequisites

| Tool   | Minimum Version        |
| ------ | ---------------------- |
| CMake  | 3.25                   |
| GCC    | 13                     |
| Clang  | 17                     |
| Ninja  | (recommended)          |
| ccache | (strongly recommended) |

<a name="dependencies"></a>

## Dependencies

| Dependency  | Required Version | Notes                        |
| ----------- | ---------------- | ---------------------------- |
| AdaptiveCpp | ~25.10.0         | SYCL implementation (system) |
| argparse    | ^3.0             | CLI argument parsing         |
| assimp      | ^6.0.0           | 3D model loading             |
| Boost       | >= 1.85          | stacktrace, unordered, etc.  |
| doctest     | ^2.0.0           | Testing framework            |
| glaze       | ^7.0             | JSON serialization           |
| pybind11    | ^3.0.0           | Python bindings              |
| spdlog      | ^1.12            | Logging library              |

The project uses CPM (CMake Package Manager) to automatically download missing dependencies that are not found on the system. Dependencies marked as **system** (AdaptiveCpp) must be installed manually.

[↑ Back to Top](#readme-top)

<a name="building"></a>

## Building

<a name="quick-start"></a>

### Quick start

```bash
# Configure
cmake --preset linux-gcc-relwithdebinfo \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

# Build
cmake --build --preset linux-gcc-relwithdebinfo

# Run tests
ctest --preset linux-gcc-relwithdebinfo
```

<a name="available-presets"></a>

### Available presets

Presets follow the pattern `{os}-{compiler}-{build_type}`:

| OS      | Compiler           | Build Types                    |
| ------- | ------------------ | ------------------------------ |
| Linux   | gcc, clang         | debug, relwithdebinfo, release |
| Windows | msvc, clang-cl, vs | debug, relwithdebinfo, release |
| macOS   | clang              | debug, relwithdebinfo, release |

<a name="manual-configure"></a>

### Manual configure

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

cmake --build build
```

[↑ Back to Top](#readme-top)

<a name="cmake-options"></a>

## CMake Options

| Option                                                  | Default | Description                              |
| ------------------------------------------------------- | ------- | ---------------------------------------- |
| `DEVELOPER_MODE`                                        | OFF     | Enable sanitizers and developer checks   |
| `ARGUS_ENABLE_UNITY_BUILD`                              | OFF     | Enable Unity/Jumbo builds                |
| `ARGUS_ENABLE_WARNINGS_AS_ERRORS`                       | ON\*    | Treat warnings as errors                 |
| `ARGUS_DOWNLOAD_PACKAGES`                               | ON      | Download missing dependencies via CPM    |
| `ARGUS_FORCE_DOWNLOAD_PACKAGES`                         | OFF     | Force CPM download even if system exists |
| `ARGUS_CHECK_PACKAGE_VERSIONS`                          | ON      | Enforce dependency version requirements  |
| `ARGUS_BUILD_TESTS`                                     | ON\*    | Build all test suites                    |
| `ARGUS_BUILD_BENCHMARKS`                                | ON\*    | Build benchmarks                         |
| `ARGUS_BUILD_ALGO`                                      | ON      | Build the algo module                    |
| `ARGUS_ALGO_BUILD_CLI`                                  | ON\*    | Build the algo CLI                       |
| `ARGUS_BUILD_BINDINGS`                                  | ON\*    | Build bindings (CAPI + Python)           |
| `ARGUS_BUILD_BINDINGS_CAPI`                             | ON      | Build C API bindings                     |
| `ARGUS_BUILD_BINDINGS_PYTHON`                           | ON      | Build Python bindings                    |
| `ARGUS_BUILD_RENDERER`                                  | ON\*    | Build the renderer                       |
| `ARGUS_BUILD_RENDERER_WITH_ALGO`                        | ON      | Link algo into renderer                  |
| `ARGUS_{CORE,COMMON,ALGO,RENDERER}_ENABLE_SANITIZERS`   | OFF     | Enable sanitizers for debug builds       |
| `ARGUS_{CORE,COMMON,ALGO,RENDERER}_SANITIZER_ADDRESS`   | OFF     | Enable AddressSanitizer                  |
| `ARGUS_{CORE,COMMON,ALGO,RENDERER}_SANITIZER_UNDEFINED` | OFF     | Enable UndefinedBehaviorSanitizer        |
| `ARGUS_{CORE,COMMON,ALGO,RENDERER}_SANITIZER_THREAD`    | OFF     | Enable ThreadSanitizer                   |
| `ARGUS_{CORE,COMMON,ALGO,RENDERER}_SANITIZER_MEMORY`    | OFF     | Enable MemorySanitizer (Clang only)      |

\* Defaults to `ON` when built as top-level project.

<a name="installing-dependencies"></a>

## Installing Dependencies

<a name="ubuntu-debian"></a>

### Ubuntu / Debian

```bash
sudo apt install cmake ninja-build ccache \
  libargparse-dev libassimp-dev libboost-dev \
  doctest-dev pybind11-dev libspdlog-dev \

# AdaptiveCpp (see dedicated section below)
```

<a name="arch-linux"></a>

### Arch Linux

```bash
sudo pacman -S cmake ninja ccache argparse assimp boost doctest glaze pybind11 spdlog

# AdaptiveCpp using the AUR (paru, yay, pakku), or see dedicated section below to install from source
paru -S adaptivecpp
```

<a name="fedora-rhel"></a>

### Fedora / RHEL

```bash
sudo dnf install cmake ninja-build ccache \
  argparse-devel assimp-devel boost-devel  \
  doctest-devel pybind11-devel spdlog-devel

# AdaptiveCpp (see dedicated section below to install from source)
```

<a name="macos-homebrew"></a>

### macOS (Homebrew)

```bash
brew install cmake ninja ccache argparse assimp boost doctest glaze pybind11 spdlog

# AdaptiveCpp from brew, or see dedicated section below to install from source
brew install adaptivecpp
```

<a name="adaptivecpp"></a>

### AdaptiveCpp

AdaptiveCpp is a SYCL implementation that serves as the core compute backend. It has additional dependencies: LLVM >= 15 (<= 20), Python 3, Boost.

**Standard installation (Linux):**

For a standard installation that has the most important features enabled, you will additionally need to install an official LLVM release >= 15.

For example install LLVM 20:

```bash
# Install LLVM (e.g., from apt.llvm.org)
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 20 all
```

```bash
# Install build dependencies before building AdaptiveCpp (python3, boost, cmake)

# Build and install AdaptiveCpp
git clone https://github.com/AdaptiveCpp/AdaptiveCpp
cd AdaptiveCpp
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make install
```

If any encountered errors during the build, refer to the [official installation guide](https://adaptivecpp.github.io/AdaptiveCpp/installing/) for troubleshooting.

**Advanced / custom backends:**

AdaptiveCpp supports multiple compilation flows. To enable specific backends (CUDA, ROCm, etc.), additional flags and dependencies are needed. See the [official installation guide](https://adaptivecpp.github.io/AdaptiveCpp/installing/) for full details.

<a name="windows"></a>

### Windows

Dependencies will be installed automatically via CPM

For AdaptiveCpp on Windows (experimental), see the [official wiki](https://adaptivecpp.github.io/AdaptiveCpp/installing/).

[↑ Back to Top](#readme-top)

<a name="makefile"></a>

## Makefile

A convenience `Makefile` is provided for formatting and linting:

```bash
make help              # Show available targets

make format            # Format C/C++ files with clang-format
make format-check      # Check formatting without modifying
make lint              # Lint C/C++ files with clang-tidy
```

Variables:

| Variable           | Description                           |
| ------------------ | ------------------------------------- |
| `FILES`            | Space-separated files/dirs to process |
| `EXCLUDE`          | Space-separated files/dirs to exclude |
| `CLANG_FORMAT_CFG` | Optional path to `.clang-format`      |
| `CLANG_TIDY_CFG`   | Optional path to `.clang-tidy`        |
| `BUILD_DIRS`       | Space-separated build dirs for lint   |

<a name="testing"></a>

## Testing

Tests use the [doctest](https://github.com/doctest/doctest) framework. Run via Ctest:

```bash
# Using preset
ctest --preset linux-gcc-debug

# Or directly
ctest --test-dir build/linux-gcc-debug
```

<a name="contact"></a>

## Contact

**RexarX** - who727cares@gmail.com

**Project Link:** [https://github.com/RexarX/HeliosEngine](https://github.com/RexarX/HeliosEngine)

<a name="license"></a>

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.

[↑ Back to Top](#readme-top)

<!-- MARKDOWN LINKS & IMAGES -->

[license-shield]: https://img.shields.io/github/license/RexarX/argus.svg?style=for-the-badge
[license-url]: https://github.com/RexarX/argus/blob/main/LICENSE
[cpp-shield]: https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=for-the-badge&logo=c%2B%2B
[cpp-url]: https://en.cppreference.com/w/cpp/23
