#include <crefile.hpp>
#include <gtest/gtest.h>
#include <iostream>

crefile::Path TestsDir;

TEST(common, path_join) {
#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
    ASSERT_EQ("a\\b\\c", crefile::join("a", "b", "c"));
    ASSERT_EQ(crefile::Path{"a\\b\\c"}, crefile::Path("a", "b", "c"));
#else
    ASSERT_EQ("a/b/c", crefile::join("a", "b", "c"));
    ASSERT_EQ(crefile::Path{"a/b/c"}, crefile::join("a", "b", "c"));
#endif
}

TEST(common, path_join1) {
#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
    ASSERT_EQ("a/b\\c", crefile::join("a/b", "c"));
#else
    ASSERT_EQ("a/b/c", crefile::join("a/b", "c"));
#endif
}

TEST(common, path_join2) {
    ASSERT_EQ("C:/Documents", crefile::join("C:/", "Documents"));
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

TEST(dir, not_exists) {
    const auto dir = crefile::Path{TestsDir, "unexist", "a"};
    ASSERT_THROW(dir.mkdir(), crefile::NoSuchFileException);
}

TEST(dir, no_permissions) {
    ASSERT_THROW(crefile::Path{"/nope"}.mkdir(), crefile::NoPermissionException);
}

TEST(iter_dir, dir0) {
    const auto dir = crefile::Path{TestsDir, "iter_dir"};
    crefile::Path{dir, "a"}.mkdir_parents();
    std::vector<std::string> filenames;
    for (auto file : crefile::iter_dir(dir)) {
        filenames.push_back(file.name());
    }
    ASSERT_EQ(std::vector<std::string>{"a"}, filenames);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    TestsDir = crefile::Path{crefile::get_tmp_path(), "crefile_tests"};
    TestsDir/*.rmrf_if_exists()*/.mkdir();
    std::cout << "Tests dir: " << TestsDir.path() << std::endl;
    int result = RUN_ALL_TESTS();
    return result;
}