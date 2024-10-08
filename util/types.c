#include<stdlib.h>
#include<string.h>

#include"types.h"
#include"../log/log.h"

int string_read(char **buf, char *ret, size_t ret_size){
    int str_size;
    is_varint_read_success(varint_read(buf, &str_size));

    if(ret_size < str_size){
        log_warn(LOG_USE_FILE_LINE, "Ret_buf size too small: Caller=>%p", __builtin_return_address(0));
        return -1;
    }

    if(!(memcpy(ret, *buf, str_size))){
        log_warn(LOG_USE_FILE_LINE, "Can't memcpy: Caller=>%p", __builtin_return_address(0));
        return -1;
    }

    *buf += str_size;
    return str_size;
}

int string_write(char **buf, const char *data, size_t buf_size){
    
    int data_size;
    if(!(data_size = strlen(data))){
        log_warn(LOG_USE_FILE_LINE, "Data size equal zero: Caller=>%p", __builtin_return_address(0));
        return -1;
    }

    if(buf_size < data_size){
        log_warn(LOG_USE_FILE_LINE, "Ret_buf size too small: Caller=>%p", __builtin_return_address(0));
        return -1;
    }

    is_varint_write_success(varint_write(buf, data_size));

    if(!(memcpy(*buf, data, data_size))){
        log_warn(LOG_USE_FILE_LINE, "Can't memcpy: Caller=>%p", __builtin_return_address(0));
        return -1;
    }

    *buf += data_size;

    return data_size;
}

int varint_read(char **buf, int *ret){
    int position = 0 , pread = 0;
    *ret = 0;
    char current_byte;
    while(1){
        current_byte = (*buf)[pread++];
        *ret |= (current_byte & SEGMENT_BITS) << position;
        if ((current_byte & CONTINUE_BITS) == 0){
            break;
        }
        position += 7;
        if(position >= 32){
            log_warn(LOG_USE_FILE_LINE, "VarInt is too big => Caller: %p", __builtin_return_address(0));
            return -1;
        }
    }
    *buf += pread;
    return pread;
}

int varint_write(char **buf, int data){
    int pread = 0;
    while (1){
        if((data & ~SEGMENT_BITS) == 0){
            (*buf)[pread++] = data;
            (*buf) += pread;
            return pread;
        }
        (*buf)[pread++] = (data & SEGMENT_BITS | CONTINUE_BITS);
        data >>= 7;
    }
    return -1;
}

int varint_size(int data)
{
    int pread = 0;
    while (1){
        if((data & ~SEGMENT_BITS) == 0){
            pread++;
            return pread;
        }
        pread++;
        data >>= 7;
    }
    return -1;
}
