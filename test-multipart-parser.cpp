#include "gtest/gtest.h"

static const char* content_type1 = "multipart/form; boundary=\"abcdefghijk\"";
static const char* content_type1 = "multipart/form; boundary=abcdefghijk";
static const char* content_type1 = "multipart/form; boundary=abcdefghijk\n  casdfeeeasdf";
static const char* content_type1 = "multipart/form; boundary=\"abcdefghij:k\"\n  casdfeeeasdf";

class boundaryParam
{
    public:
        const char* content_type;
        const char* expected_boundary;
        size_t expected_len;
    public:
        ruleparam(const char* _ct, const char* _eb, size_t _el):content_type(_ct), expected_boundary(_eb), expected_len(_el){};
};

class extractBoundaryParamTest :public testing::TestWithParam<boundaryParam> {
};

TEST_P(extractBoundaryParamTest, extract_boundary)
{
    boundaryParam const & p = GetParam();
    const char* bd = NULL;
    size_t bdlen = 0;
    int ret = multipart_extract_boundary(p.content_type, strlen(p.content_type), &bd, &bdlen);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(p.expected_len, bdlen);
    int cmp = memcmp(p.expected_boundary, bd, bdlen);
    ASSERT_EQ(cmp, 0);
}

INSTANTIATE_TEST_CASE_P(RandomExtract, extractBoundaryParamTest, testing::Values(
            boundaryParam("multipart/form; boundary=\"abcdefghijk\"", "abcdefghijk", strlen("abcdefghijk")), 
            boundaryParam("multipart/form; boundary=abcdefghijk", "abcdefghijk", strlen("abcdefghijk")), 
            boundaryParam("multipart/form; boundary=abcdefghijk\n  casdfeeeasdf", "abcdefghijk", strlen("abcdefghijk")),
            boundaryParam("multipart/form; boundary=\"abcdefghij:k\"\n  casdfeeeasdf", "abcdefghij:k", strlen("abcdefghij:k")),
            ));


int
main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
