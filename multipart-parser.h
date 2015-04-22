#ifndef MULTIPART_PARSER_H_
#define MULTIPART_PARSER_H_

typedef struct multipart_parser multipart_parser;
typedef struct multipart_parser_settings multipart_parser_settings;

typedef int (*multipart_data_cb) (multipart_parser*, const char *at, size_t length);
typedef int (*multipart_notify_cb) (multipart_parser*);

struct multipart_parser_settings {
  multipart_data_cb on_part_data;

  multipart_notify_cb on_part_data_begin;
  multipart_notify_cb on_part_data_end;
  multipart_notify_cb on_body_end;
};

multipart_parser* multipart_parser_create
    (const char *boundary, size_t boundary_len, const multipart_parser_settings* settings);

void multipart_parser_destroy(multipart_parser* p);

size_t multipart_parser_execute(multipart_parser* p, const char *buf, size_t len);

void multipart_parser_set_data(multipart_parser* p, void* data);
void * multipart_parser_get_data(multipart_parser* p);

int multipart_extract_boundary(const char* content_type, size_t len, const char** boundary, size_t* boundary_len);

#endif
