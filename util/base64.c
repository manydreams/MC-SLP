#include <stdlib.h>
#include <string.h>

#include "base64.h"

char __base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int __base64_decode_table[] ={0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,62,0,0,0,
    		 63,52,53,54,55,56,57,58,
    		 59,60,61,0,0,0,0,0,0,0,0,
    		 1,2,3,4,5,6,7,8,9,10,11,12,
    		 13,14,15,16,17,18,19,20,21,
    		 22,23,24,25,0,0,0,0,0,0,26,
    		 27,28,29,30,31,32,33,34,35,
    		 36,37,38,39,40,41,42,43,44,
    		 45,46,47,48,49,50,51
    	    };

int base64_encode(char *input, const int input_len, char **output, int *output_len){

    int padding = 0;
    unsigned char *pi = (unsigned char*)input;
    char *po;


    if(input_len % 3!= 0){
        padding = 3 - (input_len % 3);
    }

    *output_len = (input_len + padding) * 4 / 3;

    *output = (char*)malloc(*output_len + 1);
    if(*output == NULL) {
        return -1;
    }
    po = *output;

    for(int i = 0; i < input_len; i += 3){
    
        if(i + 3 >= input_len && padding == 1){
            *po++ = __base64_chars[pi[0] >> 2];
            *po++ = __base64_chars[(pi[0] & 0x3) << 4 | pi[1] >> 4];
            *po++ = __base64_chars[(pi[1] & 0xf) << 2 | pi[2] >> 6];
            *po++ = '=';
            break;
        }
        if(i + 3 >= input_len && padding == 2){
            *po++ = __base64_chars[pi[0] >> 2];
            *po++ = __base64_chars[(pi[0] & 0x3) << 4 | pi[1] >> 4];
            *po++ = '=';
            *po++ = '=';
            break;
        }

        *po++ = __base64_chars[pi[0] >> 2];
        *po++ = __base64_chars[(pi[0] & 0x3) << 4 | pi[1] >> 4];
        *po++ = __base64_chars[(pi[1] & 0xf) << 2 | pi[2] >> 6];
        *po++ = __base64_chars[pi[2] & 0x3f];

        pi += 3;
    }
    *po = 0;    
    return 0;
}

int base64_decode(char *input, const int input_len, char **output, int *output_len){
    
    int padding;
    char *po;
    
    *output_len = input_len * 3 / 4;
    *output = (char*)malloc(*output_len + 1);
    if(*output == NULL) {
        return -1;
    }
    po = *output;

    if(strstr(input, "=") != NULL){
        padding = 1;
    }
    if (strstr(input, "==") != NULL){
        padding = 2;
    }

    for(int i = 0; i < input_len; i += 4){

        if(i + 4 >= input_len && padding == 1){
            *po++ = __base64_decode_table[input[i]] << 2 | __base64_decode_table[input[i+1]] >> 4;
            *po++ = __base64_decode_table[input[i+1]] << 4 | __base64_decode_table[input[i+2]] >> 2;
            break;
        }
        if(i + 4 >= input_len && padding == 2){
            *po++ = __base64_decode_table[input[i]] << 2 | __base64_decode_table[input[i+1]] >> 4;
            break;
        }
        *po++ = __base64_decode_table[input[i]] << 2 | __base64_decode_table[input[i+1]] >> 4;
        *po++ = __base64_decode_table[input[i+1]] << 4 | __base64_decode_table[input[i+2]] >> 2;
        *po++ = __base64_decode_table[input[i+2]] << 6 | __base64_decode_table[input[i+3]];
    }
    *po = 0;
    return 0;
}