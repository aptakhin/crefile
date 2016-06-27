#include <crefile.hpp>
#include <gtest/gtest.h>

TEST(common, path_join) {
#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
    ASSERT_EQ("a\\b\\c", crefile::join("a", "b", "c"));
#else
    ASSERT_EQ("a/b/c", crefile::join("a", "b", "c"));
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

TEST(file_op, op0) {
    std::ostringstream out_dir;
    out_dir << "runtests";

    auto runtests_dir = crefile::Path{crefile::join(crefile::get_tmp_path(), out_dir.str())};
    runtests_dir.mkdir_if_not_exists();
    ASSERT_TRUE(runtests_dir.exists());

    auto runtests_a_b = crefile::Path{crefile::join(runtests_dir, "a", "b")}.mkdir_parents();
    ASSERT_TRUE(runtests_a_b.exists());

    auto file = crefile::generate_tmp_filename(runtests_a_b, "crefile_test_");
}

TEST(list, dir0) {
#if CREFILE_PLATFORM == CREFILE_PLATFORM_WIN32
    for (auto file : crefile::iter_dir("C:\\Users\\Alex\\Assembling\\crefile\\tests\\*")) {
        std::cout << file.name() << std::endl;
    }
#else
    //for (auto file : crefile::iter_dir("/usr")) {
    //    std::cout << file.name() << std::endl;
    //}
#endif
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    return result;
}