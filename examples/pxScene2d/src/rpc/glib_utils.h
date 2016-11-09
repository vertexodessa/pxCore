#ifndef GLIB_UTILS_H
#define GLIB_UTILS_H

#include <glib.h>


constexpr int eintr_maximum_attempts =100;
#define HANDLE_EINTR_EAGAIN(x) ({            \
  unsigned char _attempts = 0;               \
                                             \
  decltype(x) _result;                       \
                                             \
  do                                         \
  {                                          \
    _result = (x);                           \
  }                                          \
  while (_result == -1                       \
         && (errno == EINTR ||               \
             errno == EAGAIN ||              \
             errno == EWOULDBLOCK)           \
         && _attempts++ < eintr_maximum_attempts); \
                                             \
  _result;                                   \
})

constexpr int PIPE_LISTEN = 0, PIPE_WRITE = 1;
typedef void (*PipeSourceCallback)(void* ctx);

GSource* pipe_source_new(int pipefd[2], PipeSourceCallback cb, void* ctx);

#endif // GLIB_UTILS_H
