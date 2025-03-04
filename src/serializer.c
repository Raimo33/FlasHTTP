/*================================================================================

File: serializer.c                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-03-05 21:04:20                                                

================================================================================*/

#include <string.h>
#include <sys/uio.h>

#include "common.h"
#include "serializer.h"

#ifdef __AVX512F__

#endif

#ifdef __AVX2__

#endif

#ifdef __SSE2__

#endif

constexpr char methods_str[][sizeof(uint64_t)] ALIGNED(64) = {
  [HTTP_GET] = "GET",
  [HTTP_HEAD] = "HEAD",
  [HTTP_POST] = "POST",
  [HTTP_PUT] = "PUT",
  [HTTP_DELETE] = "DELETE",
  [HTTP_OPTIONS] = "OPTIONS",
  [HTTP_TRACE] = "TRACE",
  [HTTP_PATCH] = "PATCH",
  [HTTP_CONNECT] = "CONNECT"
};
constexpr uint8_t methods_len[] = {
  [HTTP_GET] = STR_LEN("GET"),
  [HTTP_HEAD] = STR_LEN("HEAD"),
  [HTTP_POST] = STR_LEN("POST"),
  [HTTP_PUT] = STR_LEN("PUT"),
  [HTTP_DELETE] = STR_LEN("DELETE"),
  [HTTP_OPTIONS] = STR_LEN("OPTIONS"),
  [HTTP_TRACE] = STR_LEN("TRACE"),
  [HTTP_PATCH] = STR_LEN("PATCH"),
  [HTTP_CONNECT] = STR_LEN("CONNECT")
};

constexpr char versions_str[][sizeof(uint64_t)] ALIGNED(64) = {
  [HTTP_1_0] = "HTTP/1.0",
  [HTTP_1_1] = "HTTP/1.1",
  [HTTP_2_0] = "HTTP/2.0",
  [HTTP_3_0] = "HTTP/3.0"
};
constexpr uint8_t versions_len[] = {
  [HTTP_1_0] = STR_LEN("HTTP/1.0"),
  [HTTP_1_1] = STR_LEN("HTTP/1.1"),
  [HTTP_2_0] = STR_LEN("HTTP/2.0"),
  [HTTP_3_0] = STR_LEN("HTTP/3.0")
};

constexpr char clrf[sizeof(uint16_t)] = "\r\n";
constexpr char colon_space[sizeof(uint16_t)] = ": ";

CONSTRUCTOR void http_serializer_init(void)
{
#ifdef __AVX512F__

#endif

#ifdef __AVX2__

#endif

#ifdef __SSE2__

#endif
}

static inline uint8_t serialize_method(char *restrict buffer, const http_method_t method);
static inline uint8_t vectorize_method(struct iovec *restrict iov, const http_method_t method);
static inline uint16_t serialize_path(char *restrict buffer, const char *restrict path, const uint16_t path_len);
static inline uint8_t vectorize_path(struct iovec *restrict iov, const char *restrict path, const uint16_t path_len);
static inline uint8_t serialize_version(char *restrict buffer, const http_version_t version);
static inline uint8_t vectorize_version(struct iovec *restrict iov, const http_version_t version);
static uint16_t serialize_headers(char *restrict buffer, const http_header_t *restrict headers, const uint16_t headers_count);
static uint16_t vectorize_headers(struct iovec *restrict iov, const http_header_t *restrict headers, const uint16_t headers_count);
static inline uint32_t serialize_body(char *restrict buffer, const char *restrict body, const uint32_t body_len);
static inline uint8_t vectorize_body(struct iovec *restrict iov, const char *restrict body, const uint32_t body_len);

uint32_t http1_serialize(char *restrict buffer, const http_request_t *restrict request)
{
  const char *const buffer_start = buffer;

  buffer += serialize_method(buffer, request->method);
  buffer += serialize_path(buffer, request->path, request->path_len);
  buffer += serialize_version(buffer, request->version);
  buffer += serialize_headers(buffer, request->headers, request->headers_count);
  buffer += serialize_body(buffer, request->body, request->body_len);

  return buffer - buffer_start;
}

int32_t http1_serialize_write(const int fd, const http_request_t *restrict request) //TODO optimize, too many microwrites
{
  if (UNLIKELY((8 + (request->headers_count << 4) + 1) > IOV_MAX))
    return -1;
 
  const uint16_t headers_count = request->headers_count;

  struct iovec iov[IOV_MAX] ALIGNED(64);
  uint16_t iovcnt = 0;

  iovcnt += vectorize_method(iov + iovcnt, request->method);
  iovcnt += vectorize_path(iov + iovcnt, request->path, request->path_len);
  iovcnt += vectorize_version(iov + iovcnt, request->version);
  iovcnt += vectorize_headers(iov + iovcnt, request->headers, headers_count);
  iovcnt += vectorize_body(iov + iovcnt, request->body, request->body_len);

  return writev(fd, iov, iovcnt);
}

static inline uint8_t serialize_method(char *restrict buffer, const http_method_t method)
{
  const char *const buffer_start = buffer;

  memcpy8(buffer, methods_str[method]);
  buffer += methods_len[method];
  *buffer++ = ' ';

  return buffer - buffer_start;
}

static inline uint8_t vectorize_method(struct iovec *restrict iov, const http_method_t method)
{
  *iov++ = (struct iovec){(char *)methods_str[method], methods_len[method]};
  *iov = (struct iovec){" ", 1};

  return 2;
}

static inline uint16_t serialize_path(char *restrict buffer, const char *restrict path, const uint16_t path_len)
{
  const char *const buffer_start = buffer;

  memcpy(buffer, path, path_len);
  buffer += path_len;
  *buffer++ = ' ';

  return buffer - buffer_start;
}

static inline uint8_t vectorize_path(struct iovec *restrict iov, const char *restrict path, const uint16_t path_len)
{
  *iov++ = (struct iovec){(char *)path, path_len};
  *iov = (struct iovec){" ", 1};

  return 2;
}

static inline uint8_t serialize_version(char *restrict buffer, const http_version_t version)
{
  const char *const buffer_start = buffer;

  memcpy8(buffer, versions_str[version]);
  buffer += versions_len[version];
  memcpy8(buffer, clrf);
  buffer += sizeof(clrf);

  return buffer - buffer_start;
}

static inline uint8_t vectorize_version(struct iovec *restrict iov, const http_version_t version)
{
  *iov++ = (struct iovec){(char *)versions_str[version], versions_len[version]};
  *iov = (struct iovec){(char *)clrf, sizeof(clrf)};

  return 2;
}

static uint16_t serialize_headers(char *restrict buffer, const http_header_t *restrict headers, const uint16_t headers_count)
{
  const char *const buffer_start = buffer;

  for (uint16_t i = 0; LIKELY(i < headers_count); i++)
  {
    const http_header_t *header = &headers[i];

    memcpy(buffer, header->key, header->key_len);
    buffer += header->key_len;
    memcpy2(buffer, colon_space);
    buffer += sizeof(colon_space);

    memcpy(buffer, header->value, header->value_len);
    buffer += header->value_len;
    memcpy2(buffer, clrf);
    buffer += sizeof(clrf);
  }

  memcpy2(buffer, clrf);
  buffer += sizeof(clrf);

  return buffer - buffer_start;
}

//TODO SIMD
static uint16_t vectorize_headers(struct iovec *restrict iov, const http_header_t *restrict headers, uint16_t headers_count)
{
  const struct iovec *const iov_start = iov;

  while (LIKELY(headers_count--))
  {
    const http_header_t header = *headers++;

    *iov++ = (struct iovec){(char *)header.key, header.key_len};
    *iov++ = (struct iovec){(char *)colon_space, sizeof(colon_space)};
    *iov++ = (struct iovec){(char *)header.value, header.value_len};
    *iov++ = (struct iovec){(char *)clrf, sizeof(clrf)};
  }

  *iov++ = (struct iovec){(char *)clrf, sizeof(clrf)};

  return iov - iov_start;
}

static inline uint32_t serialize_body(char *restrict buffer, const char *restrict body, const uint32_t body_len)
{
  const char *const buffer_start = buffer;

  const uint32_t len = body_len * (body != NULL);

  memcpy(buffer, body, len);
  buffer += len;

  return buffer - buffer_start;
}

static inline uint8_t vectorize_body(struct iovec *restrict iov, const char *restrict body, const uint32_t body_len)
{
  *iov = (struct iovec){(char *)body, body_len};

  return 1;
}
