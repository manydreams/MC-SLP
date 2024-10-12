#ifndef BASE64_H
#define BASE64_H

int base64_encode(char *input, const int input_len, char **output, int *output_len);
int base64_decode(char *input, const int input_len, char **output, int *output_len);

#endif /* BASE64_H */