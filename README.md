# smart_seq - A Smart Sequence 'Pseudo' Container 

Hello! This code is a special data structure written in modern C++. It's designed to be **faster than a standard `std::vector`** for many common use cases.

### Why I Created This

My main goal was to solve a common problem in **high-performance C++ projects.** You often face a trade-off: use a simple but slower container like `std::vector` or create a complex, hand-optimized one. I wanted something in the middle—a container that was both easy to use and genuinely fast.

This code aims to fill that gap.

---

### What It Does

- **Super Fast for Small Data:** For a few elements, it avoids using the heap entirely, just like a small string optimization. This makes common operations like `push_back` much faster by eliminating memory allocation overhead.
- **Smart for Large Data:** For more complex data types (like structs), it automatically reorganizes your data into a **Structure of Arrays (SoA)** format. This improves **cache locality**, allowing your CPU to process data much more efficiently and speeding up your program.
- **Automatic:** You don't need to write complex boilerplate code. Just add your data, and it handles the best memory layout for you.

---

## Installation

### Using CMake

1. **Clone the repository**:

```bash
git clone https://github.com/yourusername/smart_seq.git
cd smart_seq
```

Build and install:

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/your/install/path ..
cmake --build . --target install
```

This will install the headers to /your/install/path/include.
Use with find_package in another CMake project:

```
find_package(smart_seq REQUIRED)
target_link_libraries(your_target PRIVATE smart_seq)
```

Now you can include the library headers in your project:

```cpp
#include <machinetherapist/smart_seq.h>
```

---

### Why ‘Pseudo’?

As pointed out by [u/yuri-kilochek on Reddit](https://www.reddit.com/user/yuri-kilochek), this container is technically a "pseudo-container" for class types. 
Since class objects are stored in a decomposed (field-wise) form, direct references (`T&`) cannot be exposed. 
For primitive types, normal container-like behavior is supported, but for class types, every access involves copying the object.

---

### TODO

- [ ] Add a proper license (e.g., Apache 2.0 or MIT)
- [x] Improve CMake (suggested by [u/FlyingRhenquest](https://www.reddit.com/user/FlyingRhenquest/)):
  - [x] Add `install()` and `find_package()` support
  - [x] Make it easier to include this library in other projects
- [ ] Add unit tests using Google Test or Catch2 (suggested by [u/FlyingRhenquest](https://www.reddit.com/user/FlyingRhenquest/)):
- [ ] Optional: Set up a CI/CD pipeline for automatic builds and tests
- [ ] Improve documentation with examples and usage notes
- [ ] Experimental / next steps (suggested by [u/yuri-kilochek](https://www.reddit.com/user/yuri-kilochek/)):
  - [x] Research C++ reflection and proxy types for field-level access
  - [x] Try some experiments on a side branch to explore alternative designs
  - [x] Explore fully data-oriented design (SoA structs) as suggested by community
  - [ ] Explore optimal field ordering for cache-friendly layout and micro-optimizations
  - [x] Add proper exception safety guarantees (suggested by [u/masscry](https://www.reddit.com/user/masscry/)):
    - [x] Ensure strong/ basic exception safety for `push_back` and storage transitions
    - [x] Handle copy/move/assignment safely without leaks

---

### A Note for the Future

While this approach can be very useful, it might not always be the absolute best solution. It's important to be transparent that **I haven't yet conducted detailed benchmarks** to confirm these performance gains. The current implementation is based on theoretical principles. I believe these optimizations will prove valuable in practice, but I'd be grateful if others could contribute with more rigorous performance testing.

I hope you find it useful!
