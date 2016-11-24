// CREFILE
#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <exception>

#define CREFILE_PLATFORM_DARWIN 8
#define CREFILE_PLATFORM_UNIX 16
#define CREFILE_PLATFORM_WIN32 32

#ifdef _WIN32
#   define CREFILE_PLATFORM CREFILE_PLATFORM_WIN32
#endif

#ifdef __unix__
#   define CREFILE_PLATFORM CREFILE_PLATFORM_UNIX
#endif

#ifdef __APPLE__
#   define CREFILE_PLATFORM CREFILE_PLATFORM_DARWIN
#endif

#ifndef CREFILE_PLATFORM
#   error "Can't detect current platform for crefile!"
#endif

#if CREFILE_PLATFORM == CREFILE_PLATFORM_DARWIN
#   include <CoreServices/CoreServices.h>
#endif

#if CREFILE_PLATFORM == CREFILE_PLATFORM_UNIX || CREFILE_PLATFORM == CREFILE_PLATFORM_DARWIN
#   include <sys/types.h>
#   include <dirent.h>
#   include <sys/stat.h>
#   include <unistd.h>
#endif

#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
#   include <windows.h>
#   include <tchar.h>
#endif

namespace crefile {

typedef std::string String;

#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
    using ErrorCode = DWORD;
#else
    using ErrorCode = int;
#endif

class BaseException {
public:
    virtual ~BaseException() {}
};

class Exception : public BaseException, public std::exception {
public:
    Exception(ErrorCode code) : code_(code) {}

private:
    ErrorCode code_;
};

class RuntimeError : public BaseException, public std::runtime_error {
public:
    RuntimeError() : std::runtime_error(String{}) {}
    RuntimeError(const char* message) : std::runtime_error(message) {}
};


#define CREFILE_EXCEPTION_BASE(name) \
    public:\
        name(ErrorCode code) : Exception(code) {}

class NotImplementedException : public RuntimeError {};

class FileExistsException : public Exception {
    CREFILE_EXCEPTION_BASE(FileExistsException);
};
class NoSuchFileException : public Exception {
    CREFILE_EXCEPTION_BASE(NoSuchFileException);
};
class NotDirectoryException : public Exception {
    CREFILE_EXCEPTION_BASE(NotDirectoryException);
};
class NoPermissionException : public Exception {
    CREFILE_EXCEPTION_BASE(NoPermissionException);
};
class UnknownErrorException : public Exception  {
    CREFILE_EXCEPTION_BASE(UnknownErrorException);
};

#undef CREFILE_EXCEPTION_BASE

namespace priv {

static bool is_slash(const char c) {
    return c == '/' || c == '\\';
}

class WinPolicy {
public:
    static const char Separator = '\\';
};

class PosixPolicy {
public:
    static const char Separator = '/';
};

std::vector<String> split_impl(const char* base, size_t size) {
    std::vector<String> res;
    size_t start_pos = 0;
    for (size_t i = 0; i < size; ++i) {
        if (priv::is_slash(base[i])) {
            res.push_back(String{base + start_pos, base + i + 1});
            start_pos = i + 1;
        }
    }

    if (size - start_pos > 0) {
        res.push_back(String{base + start_pos, base + size});
    }

    return res;
}

} // namespace priv {

String dirname(const String& filename) {
    auto last_slash = filename.find_last_of('/');
    if (last_slash == String::npos) {
        last_slash = filename.find_last_of("\\\\");
        if (last_slash == String::npos) {
            return filename;
        }
    }
    return filename.substr(0, last_slash);
}

String extension(const String& filename) {
    const auto last_dot = filename.find_last_of('.');
    if (last_dot == String::npos) {
        return String{};
    } else {
        return filename.substr(last_dot + 1);
    }
}

std::vector<String> split(const String& path) {
    return priv::split_impl(path.c_str(), path.size());
}

std::vector<String> split(const char* base, size_t size) {
    return priv::split_impl(base, size);
}

class PosixPath {
private:
    using Policy = priv::PosixPolicy;
    static void path_join_append_one(String& to, const char* append) {
        if (!to.empty() && !priv::is_slash(to[to.size() - 1])) {
            to += Policy::Separator;
        }
        to += append;
    }

    static void path_join_append_one(String& to, const PosixPath& append) {
        if (!to.empty() && !priv::is_slash(to[to.size() - 1])) {
            to += Policy::Separator;
        }
        to += append.str();
    }

    template<typename First>
    static void path_join_impl(String& buf, First first) {
        path_join_append_one(buf, first);
    }

    template<typename First, typename... Types>
    static void path_join_impl(String& buf, First first, Types... tail) {
        path_join_append_one(buf, first);
        path_join_impl(buf, tail...);
    }
public:
    PosixPath() {}
    PosixPath(const char* path) : path_(path) {}
    PosixPath(String path) : path_(std::move(path)) {}

    template<typename ... Types>
    PosixPath(Types... args) {
        path_join_impl(path_, args...);
    }

    const String& str() const { return path_; }
    const char* c_str() const { return path_.c_str(); }

    template<typename ... Types>
    static PosixPath join(Types... args) {
        String buf;
        path_join_impl(buf, args...);
        return PosixPath{buf};
    }

    String dirname() const {
        return crefile::dirname(path_);
    }

    String extension() const {
        return crefile::extension(path_);
    }

    std::vector<String> split() const {
        return crefile::split(path_);
    }

    bool is_abspath() const {
        return PosixPath::is_abspath(path_);
    }

    static bool is_abspath(const String& path) {
        if (path.empty()) {
            return false;
        }
        return path[0] == Policy::Separator;
    }

private:
    String path_;
};

bool operator == (const PosixPath& a, const PosixPath& b) {
    return a.str() == b.str();
}

bool operator != (const PosixPath& a, const PosixPath& b) {
    return a.str() != b.str();
}

bool operator < (const PosixPath& a, const PosixPath& b) {
    return a.str() < b.str();
}

class WinPath {
private:
    using Policy = priv::WinPolicy;

    static void path_join_append_one(String& to, const char* append) {
        if (!to.empty() && !priv::is_slash(to.back())) {
            to += Policy::Separator;
        }
        to += append;
    }

    static void path_join_append_one(String& to, const WinPath& append) {
        if (!to.empty() && !priv::is_slash(to.back())) {
            to += Policy::Separator;
        }
        to += append.str();
    }

    template<typename First>
    static void path_join_impl(String& buf, First first) {
        path_join_append_one(buf, first);
    }

    template<typename First, typename... Types>
    static void path_join_impl(String& buf, First first, Types... tail) {
        path_join_append_one(buf, first);
        path_join_impl(buf, tail...);
    }
public:
    WinPath() {}
    WinPath(const char* path) : path_(path) {}
    WinPath(String path) : path_(std::move(path)) {}

    template<typename ... Types>
    WinPath(Types... args) {
        path_join_impl(path_, args...);
    }

    const String& str() const { return path_; }
    const char* c_str() const { return path_.c_str(); }

    template<typename ... Types>
    static WinPath join(Types... args) {
        String buf;
        path_join_impl(buf, args...);
        return WinPath{buf};
    }

    String dirname() const {
        return crefile::dirname(path_);
    }

    String extension() const {
        return crefile::extension(path_);
    }

    std::vector<String> split() const {
        return crefile::split(path_);
    }

    bool is_abspath() const {
        return WinPath::is_abspath(path_);
    }

    static bool is_abspath(const String& path) {
        if (path.size() < 3) {
            return false;
        }
        return path[1] == ':' && priv::is_slash(path[2]);
    }

private:
    String path_;
};

bool operator == (const WinPath& a, const WinPath& b) {
    return a.str() == b.str();
}

bool operator != (const WinPath& a, const WinPath& b) {
    return a.str() != b.str();
}

bool operator < (const WinPath& a, const WinPath& b) {
    return a.str() < b.str();
}

#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32

class FileInfoImplWin32 {
public:

    WIN32_FIND_DATA* native_ptr() { return &find_data_; }
    const WIN32_FIND_DATA* native_ptr() const { return &find_data_; }

    String name() const {
        // TODO: Support long names
        // TODO: Make lazy
        return String{find_data_.cFileName};
    }

private:
    WIN32_FIND_DATA find_data_;
};

namespace priv {
String translate_error_code2string(DWORD win_error_code) {
    if (win_error_code == 0)
        return String{};
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, win_error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    String message{messageBuffer, size};
    LocalFree(messageBuffer);
    return message;
}


template <typename Exc>
void winerror(int error_code, String&& win_error, const char* file, int line) {
    if (error_code != 0) {
        std::ostringstream err;
        err << win_error << " (win error: " << error_code << ", str: " << translate_error_code2string(error_code) << ") at file " << file << ":" << line;
        throw Exc(err.str());
    }
}

}; // namespace priv {

#define WINERROR(ret_code, exc, win_error) { priv::winerror<exc>(GetLastError(), (win_error), __FILE__, __LINE__); }

#define WINCHECK(ret_code, exc, win_error) { if (!(ret_code)) { priv::winerror<exc>(GetLastError(), (win_error), __FILE__, __LINE__); } }


class PathImplWin32 : public WinPath {
public:
    using Self = PathImplWin32;

    PathImplWin32() {}

    PathImplWin32(const String& path)
    :   path_{path} {
    }

    PathImplWin32(const char* path)
    :   path_{path} {
    }

    PathImplWin32 abspath() const {
        return Self{Self::cwd(), path_};
    }

    const PathImplWin32& mkdir() const {
        return PathImplWin32::mkdir(*this);
    }

    static const PathImplWin32& mkdir(const PathImplWin32& path) {
        WINCHECK(CreateDirectory(path_to_host(path), NULL),
            RuntimeError, "CreateDirectory failed");
        return path;
    }

    static const PathImplWin32& mkdir_if_not_exists(const PathImplWin32& path) {
        const auto res = CreateDirectory(path_to_host(path), NULL);
        if (!res) {
            auto const error = GetLastError();
            if (error != 0 && error != ERROR_ALREADY_EXISTS) {
                WINERROR(error, RuntimeError, "CreateDirectory failed");
            }
        }
        return path;
    }

    const PathImplWin32& mkdir_if_not_exists() const {
        return PathImplWin32::mkdir_if_not_exists(*this);
    }

    static const PathImplWin32& mkdir_parents(const PathImplWin32& path) {
        PathImplWin32 cur_path;
        for (auto dir : path.split()) {
            cur_path = join(cur_path, dir);
            if (!cur_path.exists()) {
                cur_path.mkdir();
            }
        }
        return path;
    }

    const PathImplWin32& mkdir_parents() const {
        return PathImplWin32::mkdir_parents(*this);
    }

    std::vector<String> split() const {
        return crefile::split(path_);
    }
    
    bool exists() const {
        return PathImplWin32::exists(*this);
    }

    static bool exists(const PathImplWin32& path) {
        WIN32_FIND_DATA file_data;
        HANDLE handle = FindFirstFile(path.path_.c_str(), &file_data);
        bool found = (handle != INVALID_HANDLE_VALUE);
        if (found) {
            FindClose(handle);
        }
        return found;
    }
    
  /*  template<typename ... Types>
    String join(Types... args) {
        String buf;
        path_join_impl(buf, args...);
        return buf;
    }*/
    
    String path_dirname(const String& filename) {
        auto last_slash = filename.find_last_of('/');
        if (last_slash == String::npos) {
            last_slash = filename.find_last_of("\\\\");
            if (last_slash == String::npos) {
                return filename;
            }
        }
        return filename.substr(0, last_slash);
    }

    String path_extension(const String& filename) {
        const auto last_dot = filename.find_last_of('.');
        if (last_dot == String::npos) {
            return "";
        } else {
            return filename.substr(last_dot + 1);
        }
    }
};


typedef PathImplWin32 Path;

class FileIterImplWin32 {
public:
    FileIterImplWin32()
    :   handle_{0},
        end_{true} {

    }

    ~FileIterImplWin32() {
        if (handle_) {
            FindClose(handle_);
        }
    }

    FileIterImplWin32(const String& path) {
        // TODO: Extend MAX_PATH
        WINCHECK(handle_ = FindFirstFile(path.c_str(), find_data_.native_ptr()),
            std::runtime_error, "FindFirstFile failed");
    }

    bool operator == (const FileIterImplWin32& other) const {
        return end_ && other.end_;
    }

    bool operator != (const FileIterImplWin32& other) const {
        return !(*this == other);
    }

    FileIterImplWin32& operator ++() {
        auto res = FindNextFile(handle_, find_data_.native_ptr());

        if (!res) {
            const auto error = GetLastError();
            if (error == ERROR_NO_MORE_FILES) {
                end_ = true;
            } else {
                WINERROR(error, std::runtime_error, "FindNextFile failed");
            }
        }

        return *this;
    }

    FileInfoImplWin32 operator *() const {
        return find_data_;
    }

private:
    HANDLE handle_;
    bool end_ = false;
    FileInfoImplWin32 find_data_;
};

typedef FileInfoImplWin32 FileInfo;
typedef FileIterImplWin32 FileIter;

PathImplWin32 get_tmp_path() {
    char tmp_path[MAX_PATH + 1];
    WINCHECK(GetTempPath(sizeof(tmp_path) - 1, tmp_path),
        std::runtime_error, "GetTempPath failed");
    return PathImplWin32{tmp_path};
}

PathImplWin32 generate_tmp_filename(const PathImplWin32& path, const String& file_prefix) {
    char tmp[MAX_PATH + 1];
    WINCHECK(GetTempFileName(path.path_to_host(), file_prefix.c_str(), 0, tmp),
        std::runtime_error, "GetTempFileName failed");
    return PathImplWin32{tmp};
}

#undef WINCHECK
#undef WINERROR

#endif // #if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32

#if CREFILE_PLATFORM == CREFILE_PLATFORM_UNIX || CREFILE_PLATFORM == CREFILE_PLATFORM_DARWIN

void check_error(ErrorCode code) {
    if (code != 0) {
        const auto error = errno;
        switch (error) {
            case EPERM:
            case EACCES:
                throw NoPermissionException{error};
            case ENOENT:
                throw NoSuchFileException{error};
            case EEXIST:
                throw FileExistsException{error};
            case ENOTDIR:
                throw NotDirectoryException{error};
            default:
                throw UnknownErrorException{error};
        }
    }
}

class FileInfoImplUnix {
private:
    void valid() const {
        if (is_end()) {
            throw RuntimeError("Can't get info from invalid file!");
        }
    }

    struct stat* get_stat() const {
        valid();
        if (!stat_) {
            stat_ = std::make_shared<struct stat>();
            const auto path = PosixPath(from_dir_, entry_->d_name);
            const auto res = lstat(path.c_str(), stat_.get());
            check_error(res);
        }
        return stat_.get();
    }

public:
    FileInfoImplUnix() = default;

    FileInfoImplUnix(dirent* entry, PosixPath from_dir)
    :   entry_(entry),
        from_dir_(from_dir) {
    }

    dirent* native_ptr_impl() { return entry_; }
    const dirent* native_ptr_impl() const { return entry_; }

    String name() const {
        if (is_end()) {
            return String{};
        } else {
            return String{entry_->d_name};
        }
    }

    bool is_directory() const {
        struct stat* st = get_stat();
        return S_ISDIR(stat_->st_mode);
    }

    bool is_end() const {
        return entry_ == nullptr;
    }

private:
    struct dirent* entry_ = nullptr;
    mutable std::shared_ptr<struct stat> stat_;

    PosixPath from_dir_;
};

class FileIterImplUnix {
private:
    void next() {
        do {
            auto dir_entry = ::readdir(dir_);
            dir_entry_ = FileInfoImplUnix{dir_entry, dir_path_};
        }
        while (dir_entry_.name() == "." || dir_entry_.name() == "..");
    }

public:
    FileIterImplUnix() {
    }

    ~FileIterImplUnix() {
        if (dir_) {
            closedir(dir_);
        }
    }

    FileIterImplUnix(const char* path)
        : dir_path_(path) {
        dir_ = ::opendir(path);
        next();
    }

    FileIterImplUnix(const String& path)
        : FileIterImplUnix(path.c_str()) {

    }

    FileIterImplUnix(const PosixPath& path)
        : FileIterImplUnix(path.c_str()) {

    }

    bool is_end() const {
        if (!dir_) {
            throw RuntimeError("Called is_end() for non-initialized iterator");
        }
        return dir_entry_.is_end();
    }

    bool is_directory() const {
        return dir_entry_.is_directory();
    }

    const PosixPath& dir_path() const {
        return dir_path_;
    }

    PosixPath path() const {
        if (!dir_) {
            throw RuntimeError("Called path() for non-initialized iterator");
        }
        return PosixPath{dir_path_, dir_entry_.name()};
    }

    bool operator == (const FileIterImplUnix& other) const {
        return dir_entry_.is_end() && other.dir_entry_.is_end();
    }

    bool operator != (const FileIterImplUnix& other) const {
        return !(*this == other);
    }

    FileIterImplUnix& operator ++() {
        if (!dir_) {
            throw RuntimeError("Called next file for non-initialized iterator");
        }
        next();
        return *this;
    }

    const FileInfoImplUnix& operator *() const {
        return dir_entry_;
    }

private:
    PosixPath dir_path_;
    DIR* dir_ = nullptr;
    FileInfoImplUnix dir_entry_;
};

typedef FileInfoImplUnix FileInfo;
typedef FileIterImplUnix FileIter;

class PathImplUnix : public PosixPath {
public:
    using Self = PathImplUnix;

    PathImplUnix() = default;

    PathImplUnix(String path)
    :   PosixPath{path} {
    }

    PathImplUnix(PosixPath path)
    :   PosixPath{std::move(path)} {
    }

    PathImplUnix(const char* path)
    :   PosixPath{path} {
    }

    template<typename ... Types>
    PathImplUnix(Types... args)
    :   PosixPath{args...} {
    }

    const char* path_to_host() const {
        return Self::path_to_host(*this);
    }

    static const char* path_to_host(const PathImplUnix& path) {
        return path.c_str();
    }

    static PathImplUnix tmp_dir() {
        static const char* tmp_path = getenv("TMPDIR");
        static Self tmp;
        if (tmp_path) {
            tmp = tmp_path;
        } else {
            tmp = "/tmp";
            // HACK FOR UNIX? FIXME
            //throw RuntimeError("No TMPDIR in env");
        }

        return tmp;
    }

    static const PathImplUnix cwd() {
        char buf[2000]; // FIXME: Handle bigger size and error
        const auto res = ::getcwd(buf, sizeof(buf));
        //check_error(res);
        return Self{res};
    }

    PathImplUnix abspath() const {
        return Self::abspath(*this);
    }

    static PathImplUnix abspath(const PathImplUnix& path) {
        return Self{cwd(), path};
    }

    const PathImplUnix& mkdir() const {
        return Self::mkdir(*this);
    }

    static const PathImplUnix& mkdir(const PathImplUnix& path) {
        const auto res = ::mkdir(path_to_host(path), 0777);
        check_error(res);
        return path;
    }

    static const PathImplUnix& mkdir_if_not_exists(const PathImplUnix& path) {
        if (!path.exists()) {
            path.mkdir();
        }
        return path;
    }

    const PathImplUnix& mkdir_if_not_exists() const {
        return Self::mkdir_if_not_exists(*this);
    }

    static const PathImplUnix& mkdir_parents(const PathImplUnix& path) {
        Self cur_path;
        for (const auto& dir : path.split()) {
            cur_path = join(cur_path, dir);
            cur_path.mkdir_if_not_exists();
        }
        return path;
    }

    const PathImplUnix& mkdir_parents() const {
        return Self::mkdir_parents(*this);
    }

    static const PathImplUnix& rm(const PathImplUnix& path) {
        const auto res = ::remove(path.path_to_host());
        check_error(res);
        return path;
    }

    const PathImplUnix& rm() const {
        return Self::rm(*this);
    }

    static const PathImplUnix& rmrf(const PathImplUnix& path) {
        FileIterImplUnix iter{path.str()};
        while (!iter.is_end()) {
            if (iter.is_directory()) {
                Self{iter.path()}.rmrf();
            } else {
                Self{iter.path()}.rm();
            }
            ++iter;
        }
        return path;
    }

    const PathImplUnix& rmrf() const {
        return Self::rmrf(*this);
    }

    static const PathImplUnix& rmrf_if_exists(const PathImplUnix& path) {
        if (path.exists()) {
            return path.rmrf();
        }
        return path;
    }

    const PathImplUnix& rmrf_if_exists() const {
        return Self::rmrf_if_exists(*this);
    }

    bool exists() const {
        return Self::exists(*this);
    }

    static bool exists(const PathImplUnix& path) {
        struct stat st;
        const auto res = ::stat(path.path_to_host(), &st);
        return res == 0;
    }
};

typedef PathImplUnix Path;

//PathImplUnix generate_tmp_filename(const PathImplUnix& path, const String& file_prefix) {
//    String filename = file_prefix + "XXXXXX";
//    //DO mkstemp
//    //return path / mktemp(const_cast<char*>(&filename.front()));
//}

#undef CREFILE_UNIXERROR
#undef CREFILE_UNIXCHECK

#endif // #if CREFILE_PLATFORM == CREFILE_PLATFORM_UNIX


bool operator == (const Path& path_a, const char* path_b) {
    return path_a.str() == path_b;
}

bool operator == (const Path& path_a, const String& path_b) {
    return path_a.str() == path_b;
}

//void path_join_append_one(String& to, const Path& append) {
//    if (!to.empty() && !is_slash(to[to.size() - 1])) {
//        to += DefaultSeparator;
//    }
//    to += append.path();
//}

Path operator / (const Path& to, const char* add) {
    return Path{to, add};
}

Path operator / (const Path& to, const String& add) {
    return Path{to, add};
}

class IterPath {
public:
    typedef FileIter const_iterator;

    IterPath(Path path)
    :   path_{path} {
    }

    const String& str() const { return path_.str();  }

private:
    Path path_;
};

static const IterPath iter_dir(const Path& path) {
    return IterPath{path};
}

IterPath::const_iterator begin(const IterPath& path) {
    return IterPath::const_iterator{path.str()};
}

IterPath::const_iterator end(const IterPath& path) {
    return IterPath::const_iterator{};
}

bool is_abspath(const String& path) {
    return Path::is_abspath(path);
}

Path cwd() {
    return Path::cwd();
}

Path cd(const Path& path) {
    throw NotImplementedException();
    // return Path::cd(path);
}

Path tmp_dir() {
    return Path::tmp_dir();
}

Path user_dir() {
    throw NotImplementedException();
    // return Path::user_dir();
}

template<typename ... Types>
String join(Types... args) {
    return Path::join(args...).str();
}

} // namespace crefile {