
#ifndef database_h
#define database_h
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "cJSON.h"
#include "createKEY.h"
#include "md5.h"
void view_database(cJSON * database);
cJSON * open_database(){
    FILE *f;//输入文件
    long len;//文件长度
    char *content;//文件内容
    cJSON *json;//封装后的json对象
    f=fopen("./database.json","w+");
    fseek(f,0,SEEK_END);
    len=ftell(f);
    fseek(f,0,SEEK_SET);
    content=(char *)malloc(len+1);
    fread(content,1,len,f);
    fclose(f);
    json=cJSON_Parse(content);
    if (!json) {
        json=cJSON_CreateObject();
        printf("No database\nCreated a database.\n");
    }
    return json;
}
void addlist(cJSON * name,cJSON * photo,cJSON * status,char * name_i,int photo_i,int status_i){
        cJSON_AddItemToArray(name, cJSON_CreateString(name_i));
        cJSON_AddItemToArray(photo, cJSON_CreateNumber(photo_i));
        cJSON_AddItemToArray(status, cJSON_CreateNumber(status_i));
}
void changelist(cJSON * name,cJSON * photo,cJSON * status,char * name_i,int status_i,int type){
    int i;
    for (i=0; i<cJSON_GetArraySize(name); i++) {
        if (strcmp(name_i, cJSON_GetArrayItem(name, i)->valuestring)==0) {
            break;
        }
    }
    int photo_i = cJSON_GetArrayItem(photo, i)->valueint;
        if (type==1) {//insert
            cJSON_DeleteItemFromArray(name, i);
            cJSON_DeleteItemFromArray(photo, i);
            cJSON_DeleteItemFromArray(status, i);
            cJSON_InsertItemInArray(name, 0, cJSON_CreateString(name_i));
            cJSON_InsertItemInArray(photo, 0, cJSON_CreateNumber(photo_i));
            cJSON_InsertItemInArray(status, 0, cJSON_CreateNumber(status_i));
        }else{
            cJSON_DeleteItemFromArray(name, i);
            cJSON_DeleteItemFromArray(photo, i);
            cJSON_DeleteItemFromArray(status, i);
            cJSON_AddItemToArray(name, cJSON_CreateString(name_i));
            cJSON_AddItemToArray(photo, cJSON_CreateNumber(photo_i));
            cJSON_AddItemToArray(status, cJSON_CreateNumber(status_i));
        }

    
}
void write_database(cJSON * database){
    char *buf = NULL;
    buf = cJSON_Print(database);
    remove("./database.json");
    FILE *fp = fopen("./database.json","w+");
    fwrite(buf,strlen(buf),1,fp);
    fclose(fp);
    free(buf);
}
void add_database(cJSON * database, char * user, char *password, int clientfd){
    cJSON *detail=cJSON_CreateObject();
    cJSON_AddItemToObject(detail, "password", cJSON_CreateString(password));
    cJSON_AddNumberToObject(detail, "clientfd", clientfd);
    cJSON_AddItemToObject(database, user, detail);
    Key key=createPuKey(user, password);
    int PuKEY[2]={key.k,key.n};
    cJSON_AddItemToObject(detail, "PuKeyK", cJSON_CreateNumber(PuKEY[0]));
    cJSON_AddItemToObject(detail, "PuKeyN", cJSON_CreateNumber(PuKEY[1]));
}
char * get_password(cJSON * database, char * user){
    cJSON * newdata=cJSON_GetObjectItem(database, user);
    char * position = cJSON_GetObjectItem(newdata, "password")->valuestring;
    return position;
}
int get_clientfd(cJSON * database, char * user){
    cJSON * newdata=cJSON_GetObjectItem(database, user);
    int position = cJSON_GetObjectItem(newdata, "clientfd")->valueint;
    return position;
}
//char * get_name(cJSON * database, char * user){
//    cJSON * newdata=cJSON_GetObjectItem(database, user);
//    char * position = cJSON_GetObjectItem(newdata, "name")->valuestring;
//    return position;
//}
void get_pukey(cJSON * database, char * user,int * k,int * n){
    cJSON * newdata=cJSON_GetObjectItem(database, user);
    *k = cJSON_GetObjectItem(newdata, "PuKeyK")->valueint;
    *n = cJSON_GetObjectItem(newdata, "PuKeyN")->valueint;
}

void modify_database(cJSON * database, int num,char * user,char * new_data,int * a){
    cJSON * newdata=cJSON_GetObjectItem(database, user);
    if (num==1) {
        strcpy(cJSON_GetObjectItem(newdata, "password")->valuestring,new_data);
    }else if(num==2){
        cJSON_GetObjectItem(newdata, "clientfd")->valueint=*a;
    }else if(num==3){
        strcpy(cJSON_GetObjectItem(newdata, "name")->valuestring,new_data);
    }else{
        cJSON_GetObjectItem(newdata, "PuKeyK")->valueint=*a;
        cJSON_GetObjectItem(newdata, "PuKeyN")->valueint=*(a+1);
    }
}
void view_database(cJSON * database){
    char *json_data = NULL;
    printf("data:%s\n",json_data = cJSON_Print(database));
    free(json_data);
}
int online_geti_user(cJSON * onlinelist,char *user){
    cJSON * temp = cJSON_GetObjectItem(onlinelist, user);
    return cJSON_GetArrayItem(temp, 1)->valueint;
}
int online_getsfd_user(cJSON * onlinelist, char *user){
    cJSON * temp = cJSON_GetObjectItem(onlinelist, user);
    return cJSON_GetArrayItem(temp, 0)->valueint;
}
//char * online_getsip_user(cJSON * onlinelist,char *user){
//    cJSON * temp = cJSON_GetObjectItem(onlinelist, user);
//    return cJSON_GetArrayItem(temp, 0)->valueint;
//}
void online_changelist(cJSON * onlinelist, char * user, int sfd, int i){
    if (cJSON_HasObjectItem(onlinelist, user)) {
        if (sfd==-1) {
            cJSON * Array = cJSON_GetObjectItem(onlinelist, user);
            cJSON_ReplaceItemInArray(Array, 2, cJSON_CreateNumber(i));
        }else if(i==-1){
            cJSON * Array = cJSON_GetObjectItem(onlinelist, user);
            cJSON_ReplaceItemInArray(Array, 1, cJSON_CreateNumber(sfd));
        }else{
            cJSON * Array = cJSON_CreateArray();
            cJSON_AddItemToArray(Array, cJSON_CreateNumber(sfd));
            cJSON_AddItemToArray(Array, cJSON_CreateNumber(i));
            cJSON_ReplaceItemInObject(onlinelist, user, Array);
        }
    }else{
        cJSON * Array;
        cJSON_AddItemToObject(onlinelist, user, Array = cJSON_CreateArray());
        cJSON_AddItemToArray(Array, cJSON_CreateNumber(sfd));
        cJSON_AddItemToArray(Array, cJSON_CreateNumber(i));
    }
}
int findarray(cJSON * list,char * name){
    int i=0;
    for (; i<cJSON_GetArraySize(list); i++) {
        if (strcmp(name, cJSON_GetArrayItem(list, i)->valuestring)==0) {
            return i;
        }
    }
    return -1;
}
void checkLogin(char *id_s, char *pw_s, char * time, cJSON * MD5) {
    char * whole = (char *)malloc((strlen(id_s) + strlen(pw_s) + strlen(time)+1)*sizeof(char));
    strcpy(whole, id_s);
    strcat(whole, pw_s);
    strcat(whole, time);
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char result[16];
    MD5Update(&md5, (unsigned char *)whole, (unsigned int)strlen(whole));
    MD5Final(&md5, result);
    cJSON * Array = cJSON_CreateArray();
    int i;
    for (i=0; i<16; i++) {
        cJSON_AddItemToArray(Array, cJSON_CreateNumber(result[i]));
    }
    cJSON_AddItemToObject(MD5, "md5", Array);
}
#endif /* database_h */
