#include <crefile.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>

crefile::Path TestsDir;

TEST(common, path_join) {
    ASSERT_EQ(crefile::PosixPath("a/b/c"), crefile::PosixPath("a", "b", "c"));
    ASSERT_EQ(crefile::PosixPath("a/c"), crefile::PosixPath("a", "", "c"));
    ASSERT_EQ(crefile::PosixPath("a/c"), crefile::PosixPath("a", "", "", "c"));

    ASSERT_EQ(crefile::WinPath("a\\b\\c"), crefile::WinPath("a", "b", "c"));
    ASSERT_EQ(crefile::WinPath("a\\c"), crefile::WinPath("a", "", "c"));
    ASSERT_EQ(crefile::WinPath("a\\c"), crefile::WinPath("a", "", "", "c"));
}

TEST(common, extension) {
    ASSERT_EQ("txt", crefile::extension("a/b/c.txt"));
    ASSERT_EQ("txt", crefile::Path("a/b/c.txt").extension());
    ASSERT_EQ("txt", crefile::PosixPath("a/b/c.txt").extension());
    ASSERT_EQ("txt", crefile::WinPath("C:/a/b/c.txt").extension());
}

TEST(common, dirname) {
    ASSERT_EQ("a/b", crefile::dirname("a/b/c.txt"));
    ASSERT_EQ("a/b", crefile::Path("a/b/c.txt").dirname());
    ASSERT_EQ("a/b", crefile::PosixPath("a/b/c.txt").dirname());
    ASSERT_EQ("a/b", crefile::WinPath("a/b/c.txt").dirname());
    ASSERT_EQ("C:/a/b", crefile::WinPath("C:/a/b/c.txt").dirname());
}

TEST(dir, cwd) {
    std::cout << "CWD: " << crefile::cwd().str() << std::endl;
    // FIXME: How to test this?
    //ASSERT_EQ(Path{"a/b"}, crefile::cwd());
}

TEST(dir, is_abs_path) {
    ASSERT_FALSE(crefile::PosixPath("a/b/c.txt").is_abspath());
    ASSERT_TRUE(crefile::PosixPath("/a/b/c.txt").is_abspath());
    ASSERT_FALSE(crefile::WinPath("a/b/c.txt").is_abspath());
    ASSERT_TRUE(crefile::WinPath("C:/a/b/c.txt").is_abspath());
}

#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
TEST(win32, path_join) {
    ASSERT_EQ("a\\b\\c", crefile::join("a", "b", "c"));
    ASSERT_EQ(crefile::Path("a\\b\\c"), crefile::Path("a", "b", "c"));
    ASSERT_EQ("a/b\\c", crefile::join("a/b", "c"));
}

TEST(win32, abspath) {
    ASSERT_EQ(crefile::Path(crefile::cwd(), "a/b/c"), crefile::Path("a/b/c.txt").abspath());
}

TEST(dir, no_permissions) {
    //ASSERT_THROW(crefile::Path{"/usr/bin/not_existing_folder_in_usr_bin"}.mkdir(), crefile::NoPermissionException);
}

TEST(dir, not_directory) {
    //ASSERT_THROW(crefile::Path{"/dev/null/not_directory"}.mkdir(), crefile::NotDirectoryException);
}
#endif

#if CREFILE_PLATFORM != CREFILE_PLATFORM_WIN32
TEST(posix, path_join) {
    ASSERT_EQ(crefile::Path("a/b/c"), crefile::Path("a", "b", "c"));
    ASSERT_EQ("a/b/c", crefile::Path("a/b", "c"));
}

TEST(posix, abspath) {
    ASSERT_EQ(crefile::Path(crefile::cwd(), "a/b/c.txt"), crefile::Path("a/b/c.txt").abspath());
}

TEST(dir, no_permissions) {
    ASSERT_THROW(crefile::Path{"/usr/bin/not_existing_folder_in_usr_bin"}.mkdir(), crefile::NoPermissionException);
}

TEST(dir, not_directory) {
    ASSERT_THROW(crefile::Path{"/dev/null/not_directory"}.mkdir(), crefile::NotDirectoryException);
}
#endif

TEST(common, path_join_same) {
    ASSERT_EQ(crefile::Path("C:/Documents"), crefile::Path("C:/", "Documents"));
}

TEST(dir, children) {
    const auto runtests_dir = crefile::Path{TestsDir, "runtests"};
    runtests_dir.mkdir_if_not_exists();
    ASSERT_TRUE(runtests_dir.exists());

    auto runtests_a_b = crefile::Path{runtests_dir, "a", "b"}.mkdir_parents();
    ASSERT_TRUE(runtests_a_b.exists());

//    auto file = crefile::generate_tmp_filename(runtests_a_b, "crefile_test_");
//    std::cout << "Fname: " << file.path() << std::endl;
}

TEST(dir, not_existing_folder) {
    const auto dir = crefile::Path{TestsDir, "not_existing_folder", "a"};
    ASSERT_THROW(dir.mkdir(), crefile::NoSuchFileException);
}


TEST(iter_dir, dir0) {
    const auto dir = crefile::Path{TestsDir, "iter_dir"};
    crefile::Path{dir, "a"}.mkdir_parents();
    crefile::Path{dir, "b"}.mkdir_parents();
    std::set<std::string> filenames;
    for (auto file : crefile::iter_dir(dir)) {
        filenames.insert(file.name());
    }
    const auto files = std::set<std::string>{"a", "b"};
    ASSERT_EQ(files, filenames);
}

//TEST(iter_dir, tmp) {
//    const auto dir = crefile::Path{"/tmp"};
//    for (auto file : crefile::iter_dir(dir)) {
//        std::cout << file.name() << std::endl;
//    }
//}



//TEST(examples, simple) {
//    auto foo_path = crefile::Path{"foo"};
//    foo_path.rmrf_if_exists().mkdir();
//
//    std::ofstream a_file(foo_path / "a.txt");
//    a_file << "Hello" << std::endl;
//
//    std::ofstream b_file(foo_path / "b.txt");
//    b_file << "World!" << std::endl;
//
//    for (auto file : crefile::iter_dir(foo_path)) {
//        std::cout << file.name() << " ";
//    }
//    // a.txt b.txt
//
//}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    TestsDir = crefile::Path{crefile::tmp_dir(), "crefile_tests"};
    TestsDir.rmrf_if_exists().mkdir();
    std::cout << "Tests dir: " << TestsDir.str() << std::endl;
    int result = RUN_ALL_TESTS();
    return result;
}