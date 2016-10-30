// CREFILE
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#define CREFILE_PLATFORM_DARWIN 8
#define CREFILE_PLATFORM_UNIX 16
#define CREFILE_PLATFORM_WIN32 32

#ifdef _WIN32
#   define CREFILE_PLATFORM CREFILE_PLATFORM_WIN32
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
    RuntimeError() : std::runtime_error("") {}
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
class NoPermissionException : public Exception {
    CREFILE_EXCEPTION_BASE(NoPermissionException);
};
class UnknownErrorException : public Exception  {
    CREFILE_EXCEPTION_BASE(UnknownErrorException);
};

#undef CREFILE_EXCEPTION_BASE

static bool is_slash(const char c) {
    return c == '/' || c == '\\';
}

#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
const char DefaultSeparator = '\\';
#else
const char DefaultSeparator = '/';
#endif

std::vector<String> split_impl(const char* base, size_t size) {
    std::vector<String> res;
    size_t start_pos = 0;
    for (size_t i = 0; i < size; ++i) {
        if (is_slash(base[i])) {
            res.push_back(String{base + start_pos, base + i + 1});
            start_pos = i + 1;
        }
    }

    if (size - start_pos > 0) {
        res.push_back(String{base + start_pos, base + size});
    }

    return res;
}

std::vector<String> split(const String& path) {
    return split_impl(path.c_str(), path.size());
}

std::vector<String> split(const char* base, size_t size) {
    return split_impl(base, strlen(base));
}

template<typename Append>
void path_join_append_one(String& to, Append append) {
    if (!to.empty() && !is_slash(to[to.size() - 1])) {
        to += DefaultSeparator;
    }
    to += append;
}

template<typename First>
void path_join_impl(String& buf, First first) {
    path_join_append_one(buf, first);
}

template<typename First, typename... Types>
void path_join_impl(String& buf, First first, Types... tail) {
    path_join_append_one(buf, first);
    path_join_impl(buf, tail...);
}

template<typename ... Types>
String join(Types... args) {
    String buf;
    path_join_impl(buf, args...);
    return buf;
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

#define WINERROR(ret_code, exc, win_error) { winerror<exc>(GetLastError(), (win_error), __FILE__, __LINE__); }

#define WINCHECK(ret_code, exc, win_error) { if (!(ret_code)) { winerror<exc>(GetLastError(), (win_error), __FILE__, __LINE__); } }


class PathImplWin32 {
public:
    PathImplWin32() {}

    PathImplWin32(const String& path)
    :   path_{path} {
    }

    PathImplWin32(const char* path)
    :   path_{path} {
    }

    LPCSTR path_to_host() const {
        return PathImplWin32::path_to_host(*this);
    }

    static LPCSTR path_to_host(const PathImplWin32& path) {
        return path.path().c_str();
    }

    const String& path() const { return path_; }

    const PathImplWin32& mkdir() const {
        return PathImplWin32::mkdir(*this);
    }

    static const PathImplWin32& mkdir(const PathImplWin32& path) {
        WINCHECK(CreateDirectory(path_to_host(path), NULL),
            std::runtime_error, "CreateDirectory failed");
        return path;
    }

    static const PathImplWin32& mkdir_if_not_exists(const PathImplWin32& path) {
        const auto res = CreateDirectory(path_to_host(path), NULL);
        if (!res) {
            auto const error = GetLastError();
            if (error != 0 && error != ERROR_ALREADY_EXISTS) {
                WINERROR(error, std::runtime_error, "CreateDirectory failed");
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

private:
    String path_;
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

#endif // #if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32

#if CREFILE_PLATFORM == CREFILE_PLATFORM_UNIX || CREFILE_PLATFORM == CREFILE_PLATFORM_DARWIN

//template <typename Exc>
//void winerror(int error_code, String&& win_error, const char* file, int line) {
//    if (error_code != 0) {
//        std::ostringstream err;
//        err << win_error << " (win error: " << error_code << ", str: " << translate_error_code2string(error_code) << ") at file " << file << ":" << line;
//        throw Exc(err.str());
//    }
//}
//
//#define CREFILE_UNIXERROR(ret_code, exc, win_error) { winerror<exc>(GetLastError(), (win_error), __FILE__, __LINE__); }
//
//#define CREFILE_UNIXCHECK(ret_code, exc, win_error) { if (!(ret_code)) { winerror<exc>(GetLastError(), (win_error), __FILE__, __LINE__); } }
//


class FileInfoImplUnix {
public:
    FileInfoImplUnix() {

    }

    FileInfoImplUnix(dirent* entry)
    :   entry_(entry) {
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

    bool is_end() const {
        return entry_ == nullptr;
    }


private:
    struct dirent* entry_ = nullptr;
};

void check_error(ErrorCode code) {
    if (code != 0) {
        const auto error = errno;
        switch (error) {
            case EPERM:
                throw NoPermissionException{error};
            case ENOENT:
                throw NoSuchFileException{error};
            default:
                throw UnknownErrorException{error};
        }
    }
}

class FileIterImplUnix {
public:
    FileIterImplUnix() {
    }

    ~FileIterImplUnix() {
        if (dir_) {
            closedir(dir_);
        }
    }

    FileIterImplUnix(const String& path) {
        dir_ = ::opendir(path.c_str());
        do {
            auto dir_entry = ::readdir(dir_);
            dir_entry_ = FileInfoImplUnix{dir_entry};
        }
        while (dir_entry_.name() == "." || dir_entry_.name() == "..");
    }

    bool is_end() const {
        if (!dir_) {
            throw RuntimeError("Called is_end for non-initialized iterator");
        }
        return dir_entry_.is_end();
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
        auto dir_entry = ::readdir(dir_);
        dir_entry_ = FileInfoImplUnix{dir_entry};
        return *this;
    }

    FileInfoImplUnix operator *() const {
        return dir_entry_;
    }

private:
    DIR* dir_ = nullptr;
    FileInfoImplUnix dir_entry_;
};

typedef FileInfoImplUnix FileInfo;
typedef FileIterImplUnix FileIter;

class PathImplUnix{
public:
    PathImplUnix() {}

    PathImplUnix(String path)
    :   path_(path) {
    }

    PathImplUnix(const char* path)
    :   path_(path) {
    }

    template<typename ... Types>
    PathImplUnix(Types... args)
    :   path_(join(args...)) {
    }

    const char* path_to_host() const {
        return PathImplUnix::path_to_host(*this);
    }

    static const char* path_to_host(const PathImplUnix& path) {
        return path.path().c_str();
    }

    const String& path() const { return path_; }

    const PathImplUnix& mkdir() const {
        return PathImplUnix::mkdir(*this);
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
        return PathImplUnix::mkdir_if_not_exists(*this);
    }

    static const PathImplUnix& mkdir_parents(const PathImplUnix& path) {
        PathImplUnix cur_path;
        for (const auto& dir : path.split()) {
            cur_path = join(cur_path, dir);
            cur_path.mkdir_if_not_exists();
        }
        return path;
    }

    const PathImplUnix& mkdir_parents() const {
        return PathImplUnix::mkdir_parents(*this);
    }

    static const PathImplUnix& rm(const PathImplUnix& path) {
        const auto res = ::remove(path.path_to_host());
        check_error(res);
        return path;
    }

    const PathImplUnix& rm() const {
        return PathImplUnix::rm(*this);
    }

    static const PathImplUnix& rmrf(const PathImplUnix& path) {
        throw NotImplementedException();
//        FileIterImplUnix iter{path.path()};
//        while (!iter.is_end()) {
//            if (iter.is_directory()) {
//                iter.path.rmrf();
//            } else {
//                iter.path.rm();
//            }
//        }
//        return path;
    }

    const PathImplUnix& rmrf() const {
        return PathImplUnix::rmrf(*this);
    }

    static const PathImplUnix& rmrf_if_exists(const PathImplUnix& path) {
        if (path.exists()) {
            return path.rmrf();
        }
        return path;
    }

    const PathImplUnix& rmrf_if_exists() const {
        return PathImplUnix::rmrf_if_exists(*this);
    }

    std::vector<String> split() const {
        return crefile::split(path_);
    }

    bool exists() const {
        return PathImplUnix::exists(*this);
    }

    static bool exists(const PathImplUnix& path) {
        struct stat st;
        const auto res = ::stat(path.path_to_host(), &st);
        return res == 0;
    }

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
            return String{};
        } else {
            return filename.substr(last_dot + 1);
        }
    }

private:
    String path_;
};

typedef PathImplUnix Path;

PathImplUnix get_tmp_path() {
    static PathImplUnix tmp_path = getenv("TMPDIR");
    return tmp_path;
}

//PathImplUnix generate_tmp_filename(const PathImplUnix& path, const String& file_prefix) {
//    String filename = file_prefix + "XXXXXX";
//    //DO mkstemp
//    //return path / mktemp(const_cast<char*>(&filename.front()));
//}

#undef CREFILE_UNIXERROR
#undef CREFILE_UNIXCHECK

#endif // #if CREFILE_PLATFORM == CREFILE_PLATFORM_UNIX


bool operator == (const Path& path_a, const char* path_b) {
    return path_a.path() == path_b;
}

bool operator == (const Path& path_a, const String& path_b) {
    return path_a.path() == path_b;
}

void path_join_append_one(String& to, const Path& append) {
    if (!to.empty() && !is_slash(to[to.size() - 1])) {
        to += DefaultSeparator;
    }
    to += append.path();
}

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

    const String& path() const { return path_.path();  }

private:
    Path path_;
};

static const IterPath iter_dir(const Path& path) {
    return IterPath{path};
}

IterPath::const_iterator begin(const IterPath& path) {
    return IterPath::const_iterator{path.path()};
}

IterPath::const_iterator end(const IterPath& path) {
    return IterPath::const_iterator{};
}

} // namespace crefile {