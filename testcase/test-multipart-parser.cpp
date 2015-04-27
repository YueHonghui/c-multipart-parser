#include "gtest/gtest.h"
extern "C"{
#include <stddef.h>
#include <stdint.h>
#include "multipart-parser.h"
#include "crc32.h"
}
class boundaryParam
{
    public:
        const char* content_type;
        const char* expected_boundary;
        size_t expected_len;
    public:
        boundaryParam(const char* _ct, const char* _eb, size_t _el):content_type(_ct), expected_boundary(_eb), expected_len(_el){};
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
            boundaryParam("multipart/form; boundary=\"abcdefghij:k\"\n  casdfeeeasdf", "abcdefghij:k", strlen("abcdefghij:k"))
            ));

class multipartParam
{
    public:
        const char** bufs;
        int bufcnt;
        const char* boundary;
        uint32_t expected_cksum;
    public:
        multipartParam(const char** _bufs, int _bufcnt, const char* _boundary, uint32_t _expected_cksum):bufs(_bufs), bufcnt(_bufcnt), boundary(_boundary), expected_cksum(_expected_cksum) {};
};

class extractMultipartParamTest: public testing::TestWithParam<multipartParam> {
    protected:
        virtual void SetUp() {
            cksum = 0u;
            bufused = 0;
            total_consumed = 0;
            buflen = 1024*1024;
            parser = NULL;
            buf = (char*)malloc(buflen);
            ASSERT_NE(buf, (char*)NULL);
        }
        virtual void TearDown() {
            if(NULL != parser){
                multipart_parser_destroy(parser);
            }
            parser = NULL;
            free(buf);
            buf = NULL;
            cksum = 0u;
        }
    public:
        char* buf;
        size_t bufused;
        size_t buflen;
        size_t total_consumed;
        multipart_parser* parser;
        uint32_t cksum;
};

static int
multipart_on_part_data(multipart_parser* p, const char *at, size_t length)
{
    extractMultipartParamTest* ctx = (extractMultipartParamTest*)multipart_parser_get_data(p);
    ctx->cksum = crc32_mem(ctx->cksum, at, length);
    return 0;
}

struct multipart_parser_settings setting = { multipart_on_part_data, 0, 0, 0};

TEST_P(extractMultipartParamTest, extract_multipart)
{
    size_t consumed = 0;
    multipartParam const & p = GetParam();
    parser = multipart_parser_create(p.boundary, strlen(p.boundary), &setting);
    ASSERT_NE(parser, (multipart_parser*)NULL);
    multipart_parser_set_data(parser, this);
    for(int i=0; i < p.bufcnt; i++){
        ASSERT_TRUE(bufused + strlen(p.bufs[i]) < buflen);
        memcpy(buf+bufused, p.bufs[i], strlen(p.bufs[i]));
        bufused += strlen(p.bufs[i]);
        consumed = multipart_parser_execute(parser, buf + total_consumed, bufused - total_consumed);
        total_consumed += consumed;
    }
    ASSERT_EQ(cksum, p.expected_cksum);
}

const char* array1[] = {"\r\n\r\n--iiIL", 
                "DVvvxS\r\nContent-type: dfasdfsadf\r\n\r",
                "\nddddsasdfasd\r\n--iiILDVvvxS",
                "\r\n\r\nddssioioioi",
                "oia\r\n--iiILDVvvxS--\r\n"};
const char* array2[] = {"\r\n----dd-",
    "-dddewc--\r\n\r",
    "\nddewdde-0.asdf--dd--",
    "dddewc--\r\n----dd--dddewc--\r\nContent",
    "-Type: asdfeee\r\nddewedf: ddew\r\n\r\nddweoio",
    "sdf..meiowe\r\n----dd--d",
    "ddewc----\r\n"};

INSTANTIATE_TEST_CASE_P(checksumTest, extractMultipartParamTest, testing::Values(multipartParam(array1,5,"iiILDVvvxS",2875688835u),
            multipartParam(array2, 7, "--dd--dddewc--", 1933708397)
            ));

int
main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
