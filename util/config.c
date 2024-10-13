#include<stdlib.h>
#include<string.h>

#include"config.h"
#include"yyjson.h"

// int config_load(config_t *config, const char *filename){

// }
int json2config(char *json, config_t *config){
    yyjson_doc *doc;
    yyjson_val *root, *tmp;
    yyjson_val *version, *players;
    const char *name, *motd, *favicon;
    
    /*init json doc*/
    doc = yyjson_read(json, strlen(json), 0);
    if(doc == NULL){
        return -1;
    }

    /*get root*/
    root = yyjson_doc_get_root(doc);
    if(root == NULL || yyjson_is_obj(root) == false){
        yyjson_doc_free(doc);
        return -1;
    }

    /*get version and players*/
    version = yyjson_obj_get(root, "version");
    if(version == NULL || yyjson_is_obj(version) == false){
        yyjson_doc_free(doc);
        return -1;
    }
    players = yyjson_obj_get(root, "players");
    if(players == NULL || yyjson_is_obj(players) == false){
        yyjson_doc_free(doc);
        return -1;
    }

    /*get version info*/
    tmp = yyjson_obj_get(version, "name");
    if(tmp == NULL || yyjson_is_str(tmp) == false){
        yyjson_doc_free(doc);
        return -1;
    }
    name = yyjson_get_str(tmp);
    config->name = malloc(strlen(name) + 1);
    if(config->name == NULL){
        yyjson_doc_free(doc);
        return -1;
    }
    strcpy(config->name, name);

    tmp = yyjson_obj_get(version, "protocol");
    if(tmp == NULL || yyjson_is_int(tmp) == false){
        yyjson_doc_free(doc);
        free(config->name);
        return -1;
    }
    config->protocol = yyjson_get_int(tmp);

    /*get players info*/
    tmp = yyjson_obj_get(players, "max");
    if(tmp == NULL || yyjson_is_int(tmp) == false){
        yyjson_doc_free(doc);
        free(config->name);
        return -1;
    }
    config->max_players = yyjson_get_int(tmp);

    tmp = yyjson_obj_get(players, "online");
    if(tmp == NULL || yyjson_is_int(tmp) == false){
        yyjson_doc_free(doc);
        free(config->name);
        return -1;
    }
    config->online_players = yyjson_get_int(tmp);

    /*get motd*/
    tmp = yyjson_obj_get(root, "description");
    if(tmp == NULL){
        config->motd = NULL;
    }
    if(yyjson_is_str(tmp)){
        motd = yyjson_get_str(tmp);
        config->motd = malloc(strlen(motd) + 1);
        if(config->motd == NULL){
            yyjson_doc_free(doc);
            free(config->name);
            return -1;
        }
        strcpy(config->motd, motd);
    }else if(yyjson_is_obj(tmp)){
        yyjson_val *tmp2 = yyjson_obj_get(tmp, "text");
        if(tmp2 == NULL || yyjson_is_str(tmp2) == false){
            yyjson_doc_free(doc);
            return -1;
        }
        motd = yyjson_get_str(tmp2);
        config->motd = malloc(strlen(motd) + 1);
        if(config->motd == NULL){
            yyjson_doc_free(doc);
            free(config->name);
            return -1;
        }
        strcpy(config->motd, motd);
    }else{
        yyjson_doc_free(doc);
        free(config->name);
        return -1;
    }

    /*get favicon*/
    tmp = yyjson_obj_get(root, "favicon");
    if(tmp == NULL || yyjson_is_str(tmp) == false){
        yyjson_doc_free(doc);
        free(config->name);
        free(config->motd);
        return -1;
    }
    favicon = yyjson_get_str(tmp) + 22;
    config->favicon = malloc(strlen(favicon) + 1);
    if(config->favicon == NULL){
        yyjson_doc_free(doc);
        free(config->name);
        free(config->motd);
        return -1;
    }
    strcpy(config->favicon, favicon);

#if defined(USE_EFSC)
    tmp = yyjson_obj_get(root, "enforcesSecureChat");
    if(tmp == NULL || yyjson_is_bool(tmp) == false){
        yyjson_doc_free(doc);
        free(config->name);
        free(config->motd);
        free(config->favicon);
        return -1;
    }
    config->enforce_secure_chat = yyjson_get_bool(tmp);
#endif // USE_EFSC

    yyjson_doc_free(doc);
    return 0;

}
int config2json(config_t config, char **json){
    yyjson_mut_doc *doc;
    yyjson_mut_val *root, *key;
    yyjson_mut_val *version, *players, *arr;
    char *favicon;
    int favicon_len;

    if(config.favicon != NULL){
        favicon_len = strlen(config.favicon);
        favicon = malloc(favicon_len + 23);
        if(favicon == NULL){
            return -1;
        }
        sprintf(favicon, "data:image/png;base64,%s", config.favicon);
    }
    

    doc = yyjson_mut_doc_new(NULL);
    root = yyjson_mut_obj(doc);
    version = yyjson_mut_obj(doc);
    players = yyjson_mut_obj(doc);
    arr = yyjson_mut_arr(doc);

    yyjson_mut_doc_set_root(doc, root);
    key = yyjson_mut_str(doc, "version");
    yyjson_mut_obj_add(root, key, version);
    key = yyjson_mut_str(doc, "players");
    yyjson_mut_obj_add(root, key, players);   

    yyjson_mut_obj_add_str(doc, version, "name", config.name);
    yyjson_mut_obj_add_int(doc, version, "protocol", config.protocol);

    yyjson_mut_obj_add_int(doc, players, "max", config.max_players);
    yyjson_mut_obj_add_int(doc, players, "online", config.online_players);
    key = yyjson_mut_str(doc, "sample");
    yyjson_mut_obj_add(players, key, arr);

    if(config.motd != NULL){
        yyjson_mut_obj_add_str(doc, root, "description", config.motd);
    }
    if(config.favicon != NULL){
        yyjson_mut_obj_add_str(doc, root, "favicon", favicon);
    }

#if defined(USE_EFSC)
    yyjson_mut_obj_add_bool(doc, root, "enforcesSecureChat", config.enforce_secure_chat);
#endif // USE_EFSC

    *json = yyjson_mut_write(doc, YYJSON_WRITE_NOFLAG, NULL);
    if(*json == NULL){
        yyjson_mut_doc_free(doc);
        return -1;
    }

    yyjson_mut_doc_free(doc);
    return 0;
}