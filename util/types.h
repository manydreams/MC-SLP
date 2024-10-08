#ifndef UTIL_TYPES_H
#define UTIL_TYPES_H

#include<stdbool.h>
#include<sys/types.h>

typedef char byte_t;
typedef unsigned char u_byte_t;
typedef __uint128_t uuid_t;
typedef long position_t;

#define SEGMENT_BITS 0b01111111
#define CONTINUE_BITS 0b10000000


#define USE_FILE_LINE LOG_USE_FILE_LINE
#define is_success(var,error_str)\
    if(var < 0){\
        log_warn(USE_FILE_LINE, "Can't %s: Caller=>%p", error_str, __builtin_return_address(0));\
        return var;\
    }


#ifndef VARINT
#define VARINT
    #define is_varint_read_success(var) is_success(var,"read varint")
    #define is_varint_write_success(var) is_success(var,"write varint")
    int varint_read(char **buf, int *ret);
    int varint_write(char **buf, int data);
    int varint_size(int data);
#endif

#ifndef VARLONG
#define VARLONG
    #define is_varlong_read_success(var) is_success(var,"read varlong")
    #define is_varlong_write_success(var) is_success(var,"write varlong")
    int varlong_read(char **buf, long *ret);
    int varlong_write(char **buf, long data);
#endif

#ifndef STRING
#define STRING
    int string_read(char **buf, char *ret, size_t ret_size);
    int string_write(char **buf, const char *data, size_t buf_size);
#endif

#endif