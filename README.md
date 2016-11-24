Cross-platform work with filesystem paths with common fs operations: create/remove directory, list files.


Header-only library.
Mac OS, Linux, Windows.

Only requirement: C++11 compiler.

## Examples

```c++
auto foo_path = crefile::Path{"foo"};
foo_path.rmrf_if_exists().mkdir();

std::ofstream a_file((foo_path / "a.txt").c_str());
a_file << "Hello" << std::endl;

std::ofstream b_file((foo_path / "b.txt").c_str());
b_file << "World!" << std::endl;

for (auto file : crefile::iter_dir(foo_path)) {
    std::cout << file.name() << " ";
}
// a.txt b.txt
```

Read [full guide](docs/guide.md).

## Roadmap
API is unstable before 1.0.0 release.

- 0.1 First release. 3 platforms, minimal docs and tests coverage (*ETA 15 Dec 2016*)
- ... Features, bugs and tests
- ... File watcher
- ... Stabilization
