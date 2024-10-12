#ifndef CONFIG_H
#define CONFIG_H

#include<stdbool.h>

#define V1_21       767
#define V1_20_6     766
#define V1_20_5     766
#define V1_20_4     765
#define V1_20_3     765
#define V1_20_2     764
#define V1_20_1     763
#define V1_20       763
#define V1_19_4     762
#define V1_19_3     761
#define V1_19_2     760
#define V1_19_1     760
#define V1_19       759
#define V1_18_2     758
#define V1_18_1     757
#define V1_18       757
#define V1_17_1     756
#define V1_17       755
#define V1_16_5     754
#define V1_16_4     754
#define V1_16_3     753
#define V1_16_2     751
#define V1_16_1     736
#define V1_16       735
#define V1_15_2     578
#define V1_15_1     575
#define V1_15       573
#define V1_14_4     498
#define V1_14_3     490
#define V1_14_2     485
#define V1_14_1     480
#define V1_14       477
#define V1_13_2     404
#define V1_13_1     401
#define V1_13       393
#define V1_12_2     340
#define V1_12_1     338
#define V1_12       335
#define V1_11_2     316
#define V1_11_1     316
#define V1_11       315
#define V1_10_2     210
#define V1_10_1     210
#define V1_10       210
#define V1_9_4      110
#define V1_9_3      110
#define V1_9_2      109
#define V1_9_1      108
#define V1_9        107
#define V1_8_ALL    47

typedef struct config{
    char *name;
    int protocol;
    int max_players;
    int online_players;
    char *motd;
    char *favicon;    // base64 encoded
#if defined(USE_EFSC)
    bool enforce_secure_chat;
#endif // USE_EFSC
}config_t;
// int config_load(config_t *config, const char *filename);
int json2config(char *json, config_t *config);
int config2json(config_t config, char **json);

#endif // CONFIG_H