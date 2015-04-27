#define _GNU_SOURCE
#include "multipart-parser.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
/*
 *      |-----bdpartlen---|
 *      \r\n--boundaryabcde
 *      ^
 *      bdpartptr
 */

enum
{
    MULTIPART_BEGIN,
    PART_BEGIN,
    PARTDATA_BEGIN,
    PARTDATA_ENDING,
    BODY_END,
};

typedef struct multipart_parser 
{
    const multipart_parser_settings* setting;    
    size_t bdpartlen;
    void* data;
    char state;
    char buf[0];
}multipart_parser;

static const char* 
util_memfindchrs(const char* ptr, size_t len, const char* chrs)
{
    int i = 0;
    int j = 0;
    size_t chrslen = strlen(chrs);
    for(i = 0; i < len; i++){
        for(j = 0; j < chrslen; j++){
            if(ptr[i] == chrs[j]){
                return ptr + i;
            }
        }
    }
    return NULL;
}

multipart_parser* 
multipart_parser_create(const char *boundary, size_t boundary_len, const multipart_parser_settings* settings)
{
    size_t bdpartlen = 4 + boundary_len;
    multipart_parser* parser = (multipart_parser*)malloc(sizeof(multipart_parser) + bdpartlen);
    if(NULL == parser){
        return NULL;
    }
    parser->setting = settings;
    parser->state = MULTIPART_BEGIN;
    parser->bdpartlen = bdpartlen;
    memcpy(parser->buf, "\r\n--", 4);
    memcpy(parser->buf+4, boundary, boundary_len);
    return parser;
}

void 
multipart_parser_destroy(multipart_parser* p)
{
    free(p);
}

size_t 
multipart_parser_execute(multipart_parser* p, const char *buf, size_t len)
{
    size_t consumed = 0;
    const char* curr = buf;
    size_t remain = len;
    const char* pin = NULL;
    switch(p->state){
        case MULTIPART_BEGIN:{
            pin = memmem(curr, len, p->buf, p->bdpartlen);
            if(NULL == pin){
                if(len >= p->bdpartlen){
                    return len - p->bdpartlen;
                }else{
                    return 0;
                }
            }
            curr = pin + p->bdpartlen;
            remain = len - (curr-buf);
            p->state = PART_BEGIN;
        }
        case PART_BEGIN:{
            pin = memmem(curr, remain, "\r\n\r\n", 4);
            if(NULL == pin){
                return curr - buf;
            }
            curr = pin + 4;
            if(p->setting->on_part_data_begin){
                p->setting->on_part_data_begin(p);
            }
            remain = len - (curr-buf);
            p->state = PARTDATA_BEGIN;
        }
        case PARTDATA_BEGIN:{
            pin = memmem(curr, remain, p->buf, p->bdpartlen);
            if(NULL == pin){
                const char* reserved = buf + len - p->bdpartlen;
                if(reserved > curr){
                    if(p->setting->on_part_data){
                        p->setting->on_part_data(p, curr, reserved - curr);
                    }
                    return reserved - buf;
                }else{
                    return curr - buf;
                }
            }
            if(p->setting->on_part_data){
                p->setting->on_part_data(p, curr, pin - curr);
            }
            curr = pin + p->bdpartlen;
            p->state = PARTDATA_ENDING;
        }
        case PARTDATA_ENDING:{
            if(buf + len >= curr + 2){
                if('-' == curr[0] && '-' == curr[1]){
                    if(p->setting->on_body_end){
                        p->setting->on_body_end(p);
                    }
                    p->state = BODY_END;
                    return curr - buf + 2;
                }else{
                    if(p->setting->on_part_data_end){
                        p->setting->on_part_data_end(p);
                    }
                    p->state = PART_BEGIN;
                    return curr - buf;
                }
            }else{
                return curr - buf;
            }
        }
    }
}

void 
multipart_parser_set_data(multipart_parser* p, void* data)
{
    p->data = data;
}

void * 
multipart_parser_get_data(multipart_parser* p)
{
    return p->data;
}

int 
multipart_extract_boundary(const char* content_type, size_t len, const char** boundary, size_t* boundary_len)
{
    size_t flaglen = strlen("boundary=");
    const char* start = memmem(content_type, len, "boundary=", flaglen);
    if(NULL == start){
        return -1;
    }
    start += flaglen;
    const char* endptr = util_memfindchrs(start, len - (start - content_type), " \r\n;");
    if(NULL == endptr){
        endptr = content_type + len;
    }
    endptr--;
    if('"' == endptr[0]){
        endptr--;
    }
    if('"' == start[0]){
        start++;
    }
    *boundary = start;
    *boundary_len = endptr - start + 1;
    return 0;
}
