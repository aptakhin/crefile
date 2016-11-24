# Guide

Crefile is heavily influenced by Python `os` and `os.path` modules filesystem work. So it's also more familiar with POSIX API than with Win32 API.

## Platform-independent

Crefile has platform-independent paths types, which available on any platform.
`WinPath` and `UnixPath` with own native separators.


```cpp
crefile::PosixPath("a/b/c") == crefile::PosixPath("a", "b", "c");
crefile::WinPath("a\\b\\c") == crefile::WinPath("a", "b", "c");

```


Base operations with filenames:

```cpp
crefile::extension("a/b/c.txt") == "txt"; // Get extension from last dot
crefile::dirname("a/b/c.txt") == "a/b"; // Get dirname of file
```


Operations with relative/absolute paths:

```cpp
crefile::PosixPath("a/b/c.txt").is_abspath() == false;

```

## Native path
`Path` is class over current native paths:

```cpp
crefile::Path("a/b/c.txt").abspath(); // Gives absolute path from current working directory
crefile::join("a/b", "c") == "a/b\\c"; // On Windows
```

### Directories
Let's create a directory.

```cpp
const auto dir = crefile::Path{"not_existing_folder", "a"};
dir.mkdir();
```

We'll get an exception `crefile::NoSuchFileException` here because there is no `not_existing_folder` in our current dir.

You can call analogue `mkdir -p` to create all intermediate directories:

```cpp
dir.mkdir_parents();
```

But we don't call it. We'll create our temporary folder.

```cpp
crefile::Path{"my_tmp"}.mkdir();
```

Next time running the program we try to create directory once again. But it fails the same as `mkdir(1)` way with `crefile::FileExistsException`.

Try to use method checking folder existence.

```cpp
crefile::Path{"my_tmp"}.mkdir_if_not_exists();
```

Now we dump some garbage here and want cleanup it every run this way.

```cpp
crefile::Path{"my_tmp"}.rmrf_if_exists().mkdir();
```


### Iterate directory
```cpp
for (auto file : crefile::iter_dir("/tmp")) {
    std::cout << (file.is_directory()? "D" : "f") << ": " << file.name() << " ";
}

