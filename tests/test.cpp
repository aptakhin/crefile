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

    auto runtests = crefile::Path{crefile::join(crefile::get_tmp_path(), out_dir.str())}.mkdir_if_not_exists();
    ASSERT_TRUE(runtests.exists());

    auto runtests_a_b = crefile::Path{crefile::join(runtests, "a", "b")}.mkdir_parents();
    ASSERT_TRUE(runtests_a_b.exists());

    auto file = crefile::generate_tmp_filename(runtests_a_b, "crefile_test_");
    int p = 0;
}

TEST(list, dir0) {
    for (auto file : crefile::iter_dir("C:\\Users\\Alex\\Assembling\\crefile\\tests\\*")) {
        std::cout << file.name() << std::endl;
    }

    int p = 0;
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    return result;
}