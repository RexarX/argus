<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->

[![C++23][cpp-shield]][cpp-url]
[![MIT License][license-shield]][license-url]

<!-- PROJECT LOGO -->

<br />

<div align="center">

# argus

LiDAR and mirror parameter optimization

<b><a href="#getting-started">Get Started »</a></b> · <a href="#key-features">Features</a> · <a href="#modules">Modules</a> · <a href="#building">Build</a> · <a href="#using-as-a-dependency">Third-Party</a> · <a href="#documentation">Docs</a>

</div>

---

## Table of Contents

- <a href="#about-the-project">About The Project</a>
  - <a href="#key-features">Key Features</a>
  - <a href="#design-philosophy">Design Philosophy</a>
- <a href="#modules">Modules</a>
- <a href="#getting-started">Getting Started</a>
  - <a href="#requirements">Requirements</a>
  - <a href="#installing-dependencies">Installing Dependencies</a>
  - <a href="#building">Building</a>
  - <a href="#run-the-example">Run the Example</a>
- <a href="#using-as-a-dependency">Using as a Dependency</a>
  - <a href="#method-1-add_subdirectory">add_subdirectory</a>
  - <a href="#method-2-fetchcontent">FetchContent</a>
  - <a href="#method-3-cpm">CPM</a>
  - <a href="#method-4-installed-package-find_package">Installed Package (find_package)</a>
- <a href="#documentation">Documentation</a>
- <a href="#development">Development</a>
- <a href="#license">License</a>
- <a href="#contact">Contact</a>

---

## About The Project

**argus** takes LiDAR parameters (viewing angle, range, scanning pattern) and a target scene, looks for blind spots, and searches for a mirror pose that covers them. Intersection runs through a ray-tracing backend. The first backend is CPU [Embree](https://github.com/RenderKit/embree).

<a href="#readme-top">↑ Back to Top</a>

### Key Features

- **Embree on CPU** - `ARGUS_ENABLE_EMBREE_BACKEND` fetches Embree 4.4.1 binaries at configure time
- **C and Python bindings** - C API plus pybind11 3.x modules
- **Modular build** - enable algorithm, simulation, gui and bindings independently
- **Modern C++23**
- **GUI** - custom gui module for easy visualization of algorithm and simulation

### Design Philosophy

1. **Backends are CMake options** - Embree is `ARGUS_ENABLE_EMBREE_BACKEND`, on by default
2. **Fetch release binaries** - Embree is downloaded while configuring, not compiled with the project
3. **Fetch only what a build needs** - Embree is downloaded when its option is on, and pybind11 only when Python bindings are enabled
4. **One glue target** - `argus::argus` carries the enabled C++ modules; C translation units see only the C headers

<a href="#readme-top">↑ Back to Top</a>

---

## Modules

| Module            | Description                                          | Default                              |
| ----------------- | ---------------------------------------------------- | ------------------------------------ |
| `core`            | Foundation, shared geometry, and the problem context | ON                                   |
| `algo`            | Optimization algorithm                               | ON                                   |
| `common`          | Common functionality shared between `sim` and `gui`  | ON                                   |
| `sim`             | Simulation and the Embree backend                    | ON                                   |
| `gui`             | GUI that uses raylib with imgui                      | ON* (when building as a top project) |
| `argus`           | C++ umbrella header `argus.hpp`                      | ON                                   |
| `gui`             | Custom GUI module for easy visualization             | ON                                   |
| `bindings/c`      | C API for algo and sim                               | OFF                                  |
| `bindings/python` | Python package `argus`                               | OFF                                  |

```bash
cmake --preset linux-gcc-debug -DARGUS_BUILD_SIM=OFF -DARGUS_BUILD_PYTHON_BINDINGS=OFF
```

Enabled options are also compiler definitions: `ARGUS_ENABLE_EMBREE_BACKEND`, `ARGUS_ENABLE_ALGO` (`ARGUS_BUILD_ALGO`), and `ARGUS_ENABLE_SIM` (`ARGUS_BUILD_SIM`). The umbrella headers include algo and sim when those macros are set.

<a href="#readme-top">↑ Back to Top</a>

---

## Getting Started

### Requirements

| Tool             | Minimum                         | Recommended                     |
| ---------------- | ------------------------------- | ------------------------------- |
| **CMake**        | 3.30+                           | 3.31+                           |
| **C++ compiler** | GCC 14+, Clang 19+, MSVC 19.34+ | GCC 15+, Clang 21+              |
| **Python**       | 3.11+ (bindings and `scripts/`) | 3.11+ (bindings and `scripts/`) |

```bash
git clone https://github.com/RexarX/argus.git
cd argus
```

### Installing Dependencies

With `ARGUS_ENABLE_EMBREE_BACKEND=ON` (the default), CMake downloads the CPU archive from the [Embree v4.4.1 release](https://github.com/RenderKit/embree/releases/tag/v4.4.1) into `.cpm_cache/embree` and imports it. The first configure needs network access. Later configures reuse the archive. Turn the option off to skip the download.

Published CPU archives:

| Platform     | Archive                            |
| ------------ | ---------------------------------- |
| Windows x64  | `embree-4.4.1.x64.windows.zip`     |
| Linux x86_64 | `embree-4.4.1.x86_64.linux.tar.gz` |
| macOS x86_64 | `embree-4.4.1.x86_64.macosx.zip`   |
| macOS arm64  | `embree-4.4.1.arm64.macosx.zip`    |

Windows binaries are MSVC import libraries, so use the MSVC or clang-cl preset. Linux arm64 and Windows ARM64 are not in that release. The `.sycl` archives are not used.

pybind11 is required only when `ARGUS_BUILD_PYTHON_BINDINGS=ON`: `find_package(pybind11)` runs first, and [CPM](https://github.com/cpm-cmake/CPM.cmake) fetches v3.1.0 when no 3.x package (`>= 3.1`, `< 4`) is found.

| Dependency | Required     | How it is found                                             |
| ---------- | ------------ | ----------------------------------------------------------- |
| Embree     | 4.4.1 CPU    | Downloaded from the GitHub release into `.cpm_cache/embree` |
| pybind11   | 3.x `>= 3.1` | `find_package`, then CPM                                    |

<a href="#readme-top">↑ Back to Top</a>

### Building

Preset pattern: `{os}-{compiler}-{build_type}`

```bash
cmake --list-presets
cmake --list-presets build
```

#### Linux (GCC)

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

#### Linux (Clang)

```bash
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
```

#### Windows (MSVC + Ninja)

From a shell where `vcvars64.bat` has already run:

```bat
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

Or generate a Visual Studio solution:

```bat
cmake --preset windows-vs-debug
```

The MSVC preset keeps `cl.exe` as the host compiler and links the fetched Embree import library. `embree4.dll`, `tbb12.dll`, and `tbbmalloc.dll` are copied next to each example and next to `argus.<abi>.pyd`.

#### macOS (Clang)

```bash
cmake --preset macos-clang-debug
cmake --build --preset macos-clang-debug
```

#### Recommended developer flags

```bash
cmake --preset linux-gcc-debug \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DARGUS_DEVELOPER_MODE=ON
```

| Option                            | Default          | Notes                                                      |
| --------------------------------- | ---------------- | ---------------------------------------------------------- |
| `ARGUS_BUILD_ALGO`                | ON (top-level)   | Algorithm module                                           |
| `ARGUS_ENABLE_EMBREE_BACKEND`     | ON               | Fetch and link Embree 4.4.1 CPU binaries                   |
| `ARGUS_BUILD_SIM`                 | ON (top-level)   | Simulation module                                          |
| `ARGUS_BUILD_COMMON`              | ON               | Application helpers. Static Assimp mesh loading            |
| `ARGUS_BUILD_GUI`                 | ON (top-level)   | `argus_gui` raylib and ImGui window                        |
| `ARGUS_BUILD_C_BINDINGS`          | ON (top-level)   | C API                                                      |
| `ARGUS_BUILD_PYTHON_BINDINGS`     | ON (top-level)   | pybind11 modules                                           |
| `ARGUS_BUILD_EXAMPLES`            | ON (top-level)   | `basic_example`, `basic_c_example`, `basic_python_example` |
| `ARGUS_BUILD_SHARED`              | OFF              | One shared library instead of static archives              |
| `ARGUS_DEVELOPER_MODE`            | OFF              | Sanitizers in Debug                                        |
| `ARGUS_ENABLE_RELEASE_LTO`        | ON (top-level)   | Full LTO for Release                                       |
| `ARGUS_ENABLE_LTO_RELWITHDEBINFO` | OFF              | Thin / incremental LTO for RelWithDebInfo                  |
| `ARGUS_LINKER`                    | `AUTO` top-level | `AUTO`, `MOLD`, `LLD`, `RAD`, `DEFAULT`                    |
| `ARGUS_ENABLE_WARNINGS_AS_ERRORS` | OFF              |                                                            |
| `ARGUS_ENABLE_INSTALL`            | OFF              | Install rules and `argusConfig.cmake`                      |
| `ARGUS_PYBIND11_VERSION`          | `3.1.0`          | CPM tag when find_package misses                           |

### Run the Example

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target basic_example
./bin/examples/debug-linux-x64/basic_example
```

On Windows the same target is `basic_example` under `bin/examples/debug-windows-x64/`. `basic_c_example` is the C API driver.

```bash
cmake --build --preset linux-gcc-debug --target basic_python_example
```

That runs `examples/python/basic.py` with `PYTHONPATH` set to the built extension directory (`bin/python/debug-linux-x64/` on Linux, `bin/python/debug-windows-x64/` on Windows). You can also run the script manually with that directory on `PYTHONPATH`.

<a href="#readme-top">↑ Back to Top</a>

---

## Using as a Dependency

Consumers should turn off examples and bindings they do not need:

```cmake
set(ARGUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ARGUS_BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)
```

Configure downloads the Embree CPU binaries when `ARGUS_ENABLE_EMBREE_BACKEND` is ON. Set it to OFF to skip that download.

### Method 1: `add_subdirectory`

```cmake
cmake_minimum_required(VERSION 3.30)
project(MyTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(third_party/argus)

add_executable(my_tool src/main.cpp)
target_link_libraries(my_tool PRIVATE argus::argus)
```

### Method 2: `FetchContent`

```cmake
include(FetchContent)

FetchContent_Declare(
    argus
    GIT_REPOSITORY https://github.com/RexarX/argus.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

set(ARGUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ARGUS_BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(argus)

add_executable(my_tool src/main.cpp)
target_link_libraries(my_tool PRIVATE argus::argus)
```

### Method 3: CPM

```cmake
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME argus
    GITHUB_REPOSITORY RexarX/argus
    GIT_TAG main
    OPTIONS
        "ARGUS_BUILD_EXAMPLES OFF"
        "ARGUS_BUILD_PYTHON_BINDINGS OFF"
)

add_executable(my_tool src/main.cpp)
target_link_libraries(my_tool PRIVATE argus::argus)
```

### Method 4: Installed Package (`find_package`)

```bash
cmake --preset linux-gcc-debug -DARGUS_ENABLE_INSTALL=ON
cmake --build --preset linux-gcc-debug
cmake --install build/linux-gcc-debug --prefix /opt/argus
```

```cmake
list(APPEND CMAKE_PREFIX_PATH "/opt/argus")
find_package(argus CONFIG REQUIRED)

add_executable(my_tool src/main.cpp)
target_link_libraries(my_tool PRIVATE argus::argus)
```

Installed files: `argusConfig.cmake`, `argusConfigVersion.cmake`, `argusTargets.cmake` ([`cmake/Install.cmake`](cmake/Install.cmake)). When the algorithm module was built with Embree, the package also installs those binaries and the config calls `find_dependency(embree)`.

<a href="#readme-top">↑ Back to Top</a>

---

## Documentation

- [Embree](https://github.com/RenderKit/embree) -- CPU ray tracing used by the first backend
- [Embree v4.4.1](https://github.com/RenderKit/embree/releases/tag/v4.4.1) -- binaries CMake downloads

<a href="#readme-top">↑ Back to Top</a>

---

## Development

### Code formatting

```bash
python scripts/format.py
python scripts/format.py --check
```

### Other scripts

```bash
python scripts/lint.py --build-dir build/linux-gcc-debug
```

<a href="#readme-top">↑ Back to Top</a>

---

## License

Distributed under the MIT License. See [LICENSE][license-url] for details.

<a href="#readme-top">↑ Back to Top</a>

---

## Contact

**RexarX** - who727cares@gmail.com

**Project:** [github.com/RexarX/argus](https://github.com/RexarX/argus)

<a href="#readme-top">↑ Back to Top</a>

<!-- MARKDOWN LINKS & IMAGES -->

[license-shield]: https://img.shields.io/github/license/RexarX/argus.svg?style=for-the-badge
[license-url]: https://github.com/RexarX/argus/blob/main/LICENSE
[cpp-shield]: https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=for-the-badge&logo=c%2B%2B
[cpp-url]: https://en.cppreference.com/w/cpp/23
