Cross-platform work with filesystem paths. 
With common fs operations: create/remove directory, list files.

Header-only library.
Mac OS, Linux, Windows.
C++11 compiler.

```c++
#include <fstream>
#include <crefile.hpp>

int main() {
    auto foo_path = crefile::Path{"foo"};
    foo_path.mkdir();

    std::ofstream a_file((foo_path / "a.txt").c_str());
    a_file << "Hello" << std::endl;

    std::ofstream b_file((foo_path / "b.txt").c_str());
    b_file << "World!" << std::endl;

    for (auto file : crefile::iter_dir(foo_path)) {
        std::cout << file.name() << " ";
    }
    // a.txt b.txt
}
```

## Roadmap
API is unstable before 1.0.0 release.

- 0.1 First release. Minimal tests coverage
- 0.2, 0.3 Features, bugs and tests
- 0.4 File watcher
- 0.5, 0.6
