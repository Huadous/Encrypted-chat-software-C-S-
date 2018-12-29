

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include "math.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "stdlib.h"
#include "errno.h"
#include "unistd.h"
#include "netinet/in.h"
#include "strings.h"
#include "database.h"
#include "pthread.h"
#include "cJSON.h"
#include "md5.h"
#include "createKEY.h"
#include "RSA.h"
#include "time.h"

#define PORT 4000//端口号
#define FILEPORT 3000
#define MAXSIZE 2000//数据长度
#define BACKLOG 10//最大队列长度
#define MAXONLINE 20//最大线程数
#define    SO_NOSIGPIPE    0x1022

int thread_num=0;
int service = 0;
cJSON * online;
cJSON * checklist_id;
cJSON * checklist_photo;
cJSON * checklist_status;
int threadstate[MAXONLINE]={0};
cJSON * login_socketfd;
void sys_err(const char *ptr, int num) {
    perror(ptr);
    exit(num);
}

char *find_file_name(char *name) {
    char * name_start = NULL;
    int sep = '/';
    if (NULL == name) {
        return NULL;
    }
    name_start = strrchr(name, sep);
    
    return (NULL == name_start)? name:(name_start + 1);
}

struct thread_data
{
    int threadid;
    char * ip;
    int clientfd;
    char * message;
    cJSON * database;};
void SendCheck(int no){
    int i=0;
    view_database(login_socketfd);
    if (cJSON_GetArraySize(login_socketfd)<1) {
        return;
    }
    printf("------------------------------------------------>\n");
    view_database(checklist_id);
    view_database(checklist_photo);
    view_database(checklist_status);
    cJSON * id = cJSON_CreateArray();
    cJSON * photo = cJSON_CreateArray();
    cJSON * status = cJSON_CreateArray();
    int j = 0;
    for (; j<cJSON_GetArraySize(checklist_id); j++) {
        cJSON_AddItemToArray(id, cJSON_CreateString(cJSON_GetArrayItem(checklist_id, j)->valuestring));
        cJSON_AddItemToArray(photo, cJSON_CreateNumber(cJSON_GetArrayItem(checklist_photo, j)->valueint));
        cJSON_AddItemToArray(status, cJSON_CreateNumber(cJSON_GetArrayItem(checklist_status, j)->valueint));
    }
    for (; i<cJSON_GetArraySize(login_socketfd); i++) {
        if (cJSON_GetArrayItem(login_socketfd, i)->valueint==no) {
            continue;
        }
        cJSON * Check = cJSON_CreateObject();
        cJSON_AddItemToObject(Check, "Mes_type", cJSON_CreateString("Check"));
        cJSON_AddItemToObject(Check, "KEY", cJSON_CreateNumber(0));
        cJSON_AddItemToObject(Check, "id", id);
        cJSON_AddItemToObject(Check, "photo", photo);
        cJSON_AddItemToObject(Check, "status", status);
        char * response = cJSON_Print(Check);
        printf("------------------------------>sendcheck:{%s}\n",response);
        send(cJSON_GetArrayItem(login_socketfd, i)->valueint, response, MAXSIZE, 0);
    }
    return;
}
int  service_selection(char * message,int clientfd){
    cJSON * temp = cJSON_Parse(message);
    if (cJSON_GetObjectItem(temp, "Mes_type")==0) {
        return -1;
    }
    
    char * id = cJSON_GetObjectItem(temp, "id")->valuestring;
    char * message1 = (char *)malloc(sizeof(message)*strlen(message));
    strcpy(message1, message);
    
    char * mes_type=cJSON_GetObjectItem(temp, "Mes_type")->valuestring;
    if (strcmp(mes_type, "CS_Register")==0) {
        return 1;
    }else if (strcmp(mes_type, "CS_Login")==0){
        if (cJSON_HasObjectItem(online, id)==0) {
            return 0;
        }
//        if (cJSON_GetArrayItem(cJSON_GetObjectItem(online, id), 1)->valueint!=0) {
//            return 0;
//        }
        return 2;
    }else if (strcmp(mes_type, "CS_Request")==0){
        if (cJSON_HasObjectItem(online, id)==0) {
            return 0;
        }
        if (cJSON_GetArrayItem(cJSON_GetObjectItem(online, id), 1)->valueint<1) {
            return 0;
        }
        return 3;
    }else if (strcmp(mes_type, "CS_Message")==0){
        if (cJSON_HasObjectItem(online, id)==0) {
            return 0;
        }
        if (cJSON_GetArrayItem(cJSON_GetObjectItem(online, id), 1)->valueint<1) {
            return 0;
        }
        printf("c\n");
        return 4;
    }else if (strcmp(mes_type, "CC_Emoji")==0){
        if (cJSON_HasObjectItem(online, id)==0) {
            return 0;
        }
        if (cJSON_GetArrayItem(cJSON_GetObjectItem(online, id), 1)->valueint<1) {
            return 0;
        }
        printf("c\n");
        return 10;
    }
    else if (strcmp(mes_type, "Check")==0){
        return 5;
    }else if (strcmp(mes_type, "Exit")==0){
        return 6;
    }else if (strcmp(mes_type, "CS_Change")==0){
        return 7;
    }else if (strcmp(mes_type, "CS_Upload")==0){
        if (cJSON_HasObjectItem(online, id)==0) {
            return 0;
        }
        if (cJSON_GetArrayItem(cJSON_GetObjectItem(online, id), 1)->valueint<1) {
            return 0;
        }
        return 8;
    }else if (strcmp(mes_type, "CS_Download")==0){
        if (cJSON_HasObjectItem(online, id)==0) {
            return 0;
        }
        if (cJSON_GetArrayItem(cJSON_GetObjectItem(online, id), 1)->valueint<1) {
            return 0;
        }
        return 9;
    }else{
        return 0;
    }
    return 0;
}

int Login(void * in){
    struct thread_data * exchange=(struct thread_data *)in;
    char * message = exchange->message;
    cJSON * database = exchange->database;
    int clientfd = exchange->clientfd;
    cJSON * temp = cJSON_Parse(message);
    char * mes_type=cJSON_GetObjectItem(temp, "Mes_type")->valuestring;
    if (strcmp(mes_type, "CS_Login")==0) {
        char * user=cJSON_GetObjectItem(temp, "id")->valuestring;
        if (cJSON_HasObjectItem(database, user)) {
            char * time=cJSON_GetObjectItem(temp, "time")->valuestring;
            cJSON * MD5=cJSON_GetObjectItem(temp, "md5");
            cJSON * data=cJSON_GetObjectItem(database, user);
            char * password=cJSON_GetObjectItem(data, "password")->valuestring;
            cJSON * secret = cJSON_CreateObject();
            checkLogin(user, password, time, secret);
            cJSON * MD51 = cJSON_GetObjectItem(secret, "md5");
            int i;
            int j=1;
            for (i=0; i<16; i++) {
                if (cJSON_GetArrayItem(MD5, i)->valueint!=cJSON_GetArrayItem(MD51, i)->valueint) {
                    j=0;
                    break;
                }
            }
            if (j==1) {
                cJSON * SMES =cJSON_CreateObject();
                cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Login_Success"));
                cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(1));
                cJSON_AddItemToObject(SMES, "photo", cJSON_CreateNumber(cJSON_GetArrayItem(checklist_photo, findarray(checklist_id, user))->valueint)) ;
                char * response = cJSON_Print(SMES);
                send(clientfd, response, MAXSIZE, 0);
//                printf("1:%d\n",cJSON_HasObjectItem(online, user));
//                view_database(cJSON_GetObjectItem(online, user));
//                printf("2:%d\n",cJSON_GetArrayItem(cJSON_GetObjectItem(online, user), 1)->valueint);
                online_changelist(online, cJSON_GetObjectItem(temp, "id")->valuestring, clientfd, 1);
//                int i=0;
//                int j=0;
//                for (; i<cJSON_GetArraySize(checklist); i++) {
////                    printf("user:%s,array:%s\n",user,cJSON_GetArrayItem(checklist, i)->valuestring);
//                    if (strcmp(user, cJSON_GetArrayItem(checklist, i)->valuestring)==0) {
//                        j=1;
//                        break;
//                    }
//                }
////                printf("---->%d\n",j);
//                if (j==0) {
//                    cJSON_AddItemToArray(checklist, cJSON_CreateString(cJSON_GetObjectItem(temp, "id")->valuestring));
//                }
                cJSON_AddItemToArray(login_socketfd, cJSON_CreateNumber(clientfd));
                changelist(checklist_id, checklist_photo, checklist_status, cJSON_GetObjectItem(temp, "id")->valuestring, 1, 1);
                view_database(online);
                printf("Response:%s\n",response);
                SendCheck(clientfd);
                return 0;
            }
            
        }
    }
    cJSON * SMES =cJSON_CreateObject();
    cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Login_Success"));
    cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(0));
    char * response = cJSON_Print(SMES);
    send(clientfd, response, MAXSIZE, 0);
    printf("Response:%s\n",response);
    return 0;
}

int Register(void * in){
    struct thread_data * exchange=(struct thread_data *)in;
    char * message = exchange->message;
    cJSON * database = exchange->database;
    int clientfd = exchange->clientfd;
    cJSON * temp = cJSON_Parse(message);
    if (cJSON_HasObjectItem(database, cJSON_GetObjectItem(temp, "id")->valuestring)) {
        cJSON * SMES =cJSON_CreateObject();
        cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Regist_Success"));
        cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(0));
        char * response = cJSON_Print(SMES);
        send(clientfd, response, MAXSIZE, 0);
        view_database(database);
        return 0;
    }else{
        printf("%s\n",cJSON_GetObjectItem(temp, "id")->valuestring);
        add_database(database, cJSON_GetObjectItem(temp, "id")->valuestring, cJSON_GetObjectItem(temp, "password")->valuestring, clientfd);
        cJSON * SMES =cJSON_CreateObject();
        cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Regist_Success"));
        cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(1));
        char * response = cJSON_Print(SMES);
        printf("response:%s\n",response);
        send(clientfd, response, MAXSIZE, 0);
        online_changelist(online, cJSON_GetObjectItem(temp, "id")->valuestring, 0, 0);
        addlist(checklist_id, checklist_photo, checklist_status, cJSON_GetObjectItem(temp, "id")->valuestring, cJSON_GetObjectItem(temp, "photo")->valueint, 0);
        view_database(login_socketfd);
        view_database(checklist_id);
        view_database(checklist_photo);
        view_database(checklist_status);
        view_database(online);
        SendCheck(clientfd);
        view_database(database);
        return 0;
    }
}
int Request(void * in){
    struct thread_data * exchange=(struct thread_data *)in;
    char * message = exchange->message;
    cJSON * database = exchange->database;
    int clientfd = exchange->clientfd;
    cJSON * temp = cJSON_Parse(message);
    int k,n;
    get_pukey(database, cJSON_GetObjectItem(temp, "id")->valuestring,&k,&n);
    cJSON * Array;
    Array = cJSON_GetObjectItem(temp, "Msg");
    int len = cJSON_GetArraySize(Array);
    int secret[200];
    int i;
    for ( i=0; i<len; i++) {
        secret[i]=cJSON_GetArrayItem(Array, i)->valueint;
    }
    char * Message = RSA_Decode(secret, len, k, n);
    printf("Msg:%s\n",Message);
    char delims[] = "#";
    char *idb = NULL;
    char *req = NULL;
    idb = strtok( Message, delims );
    req = strtok( NULL, delims );
//    printf("req:%s\n",req);
//    printf("ans:%d\n",strcmp(req,"CS_Request"));
    if (strcmp(req,"CS_Request")==0) {
        if (cJSON_HasObjectItem(online, idb)==0) {
            cJSON * SMES =cJSON_CreateObject();
            cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Response"));
            cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(-1));
            char * response = cJSON_Print(SMES);
            send(clientfd, response, MAXSIZE, 0);
            printf("offline user\n");
            printf("Response:%s\n",response);
            return 0;
        }
        cJSON * SMES =cJSON_CreateObject();
        cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Response"));
        cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(1));
        cJSON_AddItemToObject(SMES, "id", cJSON_CreateString(idb));
        int k1,n1;
        get_pukey(database, idb, &k1, &n1);
        cJSON * key=cJSON_CreateObject();
        
        cJSON_AddItemToObject(key, "K", cJSON_CreateNumber(k1));
        cJSON_AddItemToObject(key, "N", cJSON_CreateNumber(n1));
        cJSON * to_id=cJSON_CreateObject();
        cJSON_AddItemToObject(to_id, "Mes_type", cJSON_CreateString("SC_NewKey"));
        cJSON * key1;
        cJSON_AddItemToObject(to_id, "id", cJSON_CreateString(cJSON_GetObjectItem(temp, "id")->valuestring));
        cJSON_AddItemToObject(to_id, "New_Key", key1 = cJSON_CreateObject());
        cJSON_AddItemToObject(key1, "K", cJSON_CreateNumber(k));
        cJSON_AddItemToObject(key1, "N", cJSON_CreateNumber(n));
        char * position1 = cJSON_Print(to_id);
//        view_database(key);
        char * position = cJSON_Print(key);
//        printf("k,n:%d,%d\n",k,n);
//        printf("MsgArray:%s\n",position);
        int * secret1 = RSA_Encode(position, (int)strlen(position), k, n);
        cJSON * MsgArray = cJSON_CreateArray();
        int i;
        for (i=0; i<(int)strlen(position); i++) {
            cJSON_AddItemToArray(MsgArray, cJSON_CreateNumber(secret1[i]));
        }
        cJSON_AddItemToObject(SMES, "Msg", MsgArray);
//        view_database(SMES);
        char * response = cJSON_Print(SMES);
        send(clientfd, response, MAXSIZE, 0);
        printf("Response:%s\n",response);
        printf("idb:%d",online_getsfd_user(online, idb));
        send(online_getsfd_user(online, idb), position1, MAXSIZE, 0);
        printf("ResponseNewKey:%s\n",position1);
//        view_database(database);
//测试
//        online_changelist(online, cJSON_GetObjectItem(temp, "id")->valuestring, -1, 2);
//        char data[10]={'0','1','2','3','4','5','6','7','8','9'};
//        while (1) {
//            int i;
//            for (i=0; i<10; i++) {
//                send(clientfd, data+i, 1, 0);
//                sleep(1);
//            }
//            sleep(10);
//        }
        
        printf("Request successful!\n");
        return 0;
    }else{
        cJSON * SMES =cJSON_CreateObject();
        cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Response"));
        cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(0));
        char * response = cJSON_Print(SMES);
        send(clientfd, response, MAXSIZE, 0);
//        view_database(database);
        printf("Response:%s\n",response);
        printf("Request failed!\n");
        return 0;
    }
    
}

int Exchange(void * in){
    struct thread_data * exchange=(struct thread_data *)in;
    char * message = exchange->message;
//测试
//    cJSON * temp = cJSON_Parse(message);
//    int fd_to = cJSON_GetObjectItemCaseSensitive(temp, "fd")->valueint;
//    char * message1 = cJSON_GetObjectItemCaseSensitive(temp, "message")->valuestring;

//正式
    cJSON * temp = cJSON_Parse(message);
    char * fd =cJSON_GetObjectItemCaseSensitive(temp, "fd")->valuestring;
    int fd_to = online_getsfd_user(online, fd);

    send(fd_to, message, MAXSIZE, 0);
    printf("exchange successful!\n");
    return 0;
}
int Emoji(void *in) {
    struct thread_data * exchange=(struct thread_data *)in;
    char * message = exchange->message;
    //测试
    //    cJSON * temp = cJSON_Parse(message);
    //    int fd_to = cJSON_GetObjectItemCaseSensitive(temp, "fd")->valueint;
    //    char * message1 = cJSON_GetObjectItemCaseSensitive(temp, "message")->valuestring;
    
    //正式
    cJSON * temp = cJSON_Parse(message);
    char * fd =cJSON_GetObjectItemCaseSensitive(temp, "fd")->valuestring;
    int fd_to = online_getsfd_user(online, fd);
    
    send(fd_to, message, MAXSIZE, 0);
    printf("exchange successful!\n");
    return 0;
}
int Check(void  * in){
    sleep(1);
    struct thread_data * exchange=(struct thread_data *)in;
//    char * message = exchange->message;
//    cJSON * database = exchange->database;
    int clientfd = exchange->clientfd;
//    cJSON * temp = cJSON_Parse(message);
    cJSON * Check = cJSON_CreateObject();
    view_database(checklist_id);
    view_database(checklist_photo);
    view_database(checklist_status);
    cJSON * id = cJSON_CreateArray();
    cJSON * photo = cJSON_CreateArray();
    cJSON * status = cJSON_CreateArray();
    int i = 0;
    for (; i<cJSON_GetArraySize(checklist_id); i++) {
        cJSON_AddItemToArray(id, cJSON_CreateString(cJSON_GetArrayItem(checklist_id, i)->valuestring));
        cJSON_AddItemToArray(photo, cJSON_CreateNumber(cJSON_GetArrayItem(checklist_photo, i)->valueint));
        cJSON_AddItemToArray(status, cJSON_CreateNumber(cJSON_GetArrayItem(checklist_status, i)->valueint));
    }
    
    cJSON_AddItemToObject(Check, "Mes_type", cJSON_CreateString("Check"));
    cJSON_AddItemToObject(Check, "KEY", cJSON_CreateNumber(1));
    cJSON_AddItemToObject(Check, "id", id);
    cJSON_AddItemToObject(Check, "photo", photo);
    cJSON_AddItemToObject(Check, "status", status);
    
    char * response = cJSON_Print(Check);
    printf("Response:%s\n",response);
    send(clientfd, response, MAXSIZE, 0);
    return 0;
}
int Merror(void  * in){
    struct thread_data * exchange=(struct thread_data *)in;
    int clientfd = exchange->clientfd;
    cJSON * Check = cJSON_CreateObject();
    cJSON_AddItemToObject(Check, "Mes_type", cJSON_CreateString("error"));
    char * response = cJSON_Print(Check);
    printf("Response:%s\n",response);
    send(clientfd, response, MAXSIZE, 0);
    return 0;
}
int Change(void * in) {
    struct thread_data * exchange=(struct thread_data *)in;
    char * message = exchange->message;
    cJSON * database = exchange->database;
    int clientfd = exchange->clientfd;
    cJSON * temp = cJSON_Parse(message);
    char * mes_type=cJSON_GetObjectItem(temp, "Mes_type")->valuestring;
    if (strcmp(mes_type, "CS_Change")==0) {
        char * user=cJSON_GetObjectItem(temp, "id")->valuestring;
        if (cJSON_HasObjectItem(database, user)) {
            char * time=cJSON_GetObjectItem(temp, "time")->valuestring;
            cJSON * MD5=cJSON_GetObjectItem(temp, "md5");
            view_database(MD5);
            cJSON * data=cJSON_GetObjectItem(database, user);
            char * password=cJSON_GetObjectItem(data, "password")->valuestring;
            cJSON * secret = cJSON_CreateObject();
            checkLogin(user, password, time, secret);
            cJSON * MD51 = cJSON_GetObjectItem(secret, "md5");
            view_database(MD51);
            int i;
            int j=1;
            for (i=0; i<16; i++) {
                if (cJSON_GetArrayItem(MD5, i)->valueint!=cJSON_GetArrayItem(MD51, i)->valueint) {
                    j=0;
                    break;
                }
            }
            if (j==1) {
                cJSON * SMES =cJSON_CreateObject();
                cJSON_DeleteItemFromObject(database, cJSON_GetObjectItem(temp, "id")->valuestring);
                add_database(database, cJSON_GetObjectItem(temp, "id")->valuestring, cJSON_GetObjectItem(temp, "newpassword")->valuestring, clientfd);
                cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Change_Success"));
                cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(1));
                char * response = cJSON_Print(SMES);
                send(clientfd, response, MAXSIZE, 0);
                online_changelist(online, cJSON_GetObjectItem(temp, "id")->valuestring, 0, 0);
                view_database(database);
                printf("Response:%s\n",response);
                return 0;
            }
            
        }
    }
    return 0;
}
//send the file asked before
void SendFile(const char * filename, const char * ip) {
    struct hostent* host;
    struct sockaddr_in serv_addr;
    int sockfd;
    if ((host = gethostbyname(ip)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    int value = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(3000);
    serv_addr.sin_addr = *((struct in_addr *)host -> h_addr);
    bzero(&(serv_addr.sin_zero), 8);
    //connect
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == 1) {
        perror("connect");
        exit(1);
    }
    printf("----------》file服务器连接成功《----------\n");
    int fd = open(filename, O_RDONLY);
    char buf[4096];
    if(fd < 0){
        perror("open");
        exit(-3);
    }
    while(1) {
        int len = read(fd, buf, sizeof(buf));
        if(len == 0) {
            break;
        }
        int _tmp = 0;
        while(1) {
            int ret = write(sockfd, buf + _tmp, len - _tmp);
            if(ret > 0) {
                _tmp += ret;
            }
            if(_tmp == ret) {
                break;
            }
            if (_tmp < 0) {
                perror("write");
                
                break;
            }
        }
        break;
    }
    close(sockfd);
    printf("----------->file serve closed<------------\n");
}
//recv the file
//filename is the path where you gonna save your file and name it
void recvFile(const char * filename) {
        //signal(SIGPIPE,SIG_IGN);
        int accefd, sockfd;
        struct sockaddr_in seraddr,cliaddr;
        socklen_t len;
        char recf[30];
        strcpy(recf, "./");
        strcat(recf, find_file_name(filename));
        printf("%s\n", recf);
        bzero(&seraddr,sizeof(seraddr));
        bzero(&cliaddr,sizeof(cliaddr));
        
        //socket
        sockfd = socket(AF_INET,SOCK_STREAM,0);
        if(sockfd < 0)
        {
            sys_err("socket",-1);
        }
        
        //初始化ip地址+port端口号
        seraddr.sin_family = AF_INET;
        seraddr.sin_port = htons(FILEPORT);
        seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        
        //bind
        if(bind(sockfd,(struct sockaddr *)&seraddr,sizeof(seraddr)) < 0)
        {
            sys_err("bind",-2);
        }
        
        //listen
        if(listen(sockfd,128) < 0 )
        {
            sys_err("listen",-3);
        }
        //accept
        accefd = accept(sockfd,(struct sockaddr *)&cliaddr,&len);
        while(1)
        {
            char buf[4096];
            len = sizeof(cliaddr);
            printf("ok ok ok ok \n");
            printf("open file recv!\n");
            if(accefd < 0)
            {
                if(errno == EINTR)  //判断阻塞等待客户端的链接;是被信号打断还是其它因素
                    continue;
                else
                    sys_err("accept",-4);
            }
            //开始文件的读写操作
            memset(buf,0x00,sizeof(buf));
            int filefd = open(recf,O_WRONLY |O_CREAT |O_TRUNC,0777);
            while(1)
            {
                if(filefd < 0)
                    sys_err("open",-5);
                
                int leng = read(accefd,buf,sizeof(buf));
                if(leng == 0)
                {
                    printf("Opposite have close the socket.\n");
                    break; //表示文件已经读到了结尾,也意味着客户端关闭了socket
                }
                if(leng == -1 && errno == EINTR)
                    continue;
                if(leng == -1 )
                    break; //表示出现了严重的错误
                write(filefd,buf,leng);
                
            }
            
            //若文件的读写已经结束,则关闭文件描述符
            close(filefd);
            close(accefd);
            break;
        }
        close(sockfd);
}
int Upload(void *in){
    struct thread_data * exchange=(struct thread_data *)in;
    int clientfd = exchange->clientfd;
    char *message = exchange->message;
    cJSON * temp = cJSON_Parse(message);
    char * filename = cJSON_GetObjectItem(temp, "filename")->valuestring;
    cJSON * upload = cJSON_CreateObject();
    cJSON_AddItemToObject(upload, "Mes_type", cJSON_CreateString("SC_Upload_Response"));
    cJSON_AddItemToObject(upload, "KEY", cJSON_CreateNumber(1));
    cJSON_AddItemToObject(upload, "filename", cJSON_GetObjectItem(temp, "filename"));
    char * response = cJSON_Print(upload);
    printf("Response:%s\n",response);
    send(clientfd, response, MAXSIZE, 0);
    recvFile(filename);
    return 0;
}
int Push(void *in) {
    struct thread_data * exchange=(struct thread_data *)in;
    char *message = exchange->message;
    cJSON * temp = cJSON_Parse(message);
    cJSON * mes = cJSON_CreateObject();
    cJSON_AddItemToObject(mes, "Mes_type", cJSON_CreateString("CC_File"));
    cJSON_AddItemToObject(mes, "KEY", cJSON_CreateNumber(1));
    //cJSON_AddItemToObject(mes, "fd", cJSON_GetObjectItem(temp, "id"));
    cJSON_AddItemToObject(mes, "filename", cJSON_GetObjectItem(temp, "filename"));
    char * fd =cJSON_GetObjectItemCaseSensitive(temp, "fd")->valuestring;
    int fd_to = online_getsfd_user(online, fd);
    //printf("%d\n", fd_to);
    send(fd_to, cJSON_Print(mes), MAXSIZE, 0);
    return 0;
}
int Download(void *in){
    struct thread_data * exchange=(struct thread_data *)in;
    //int clientfd = exchange->clientfd;
    char *message = exchange->message;
    cJSON * temp = cJSON_Parse(message);
    //char * filename = ;
    SendFile(find_file_name(cJSON_GetObjectItem(temp, "filename")->valuestring), exchange->ip);
    return 0;
}
int Exit(void  * in){
    struct thread_data * exchange=(struct thread_data *)in;
    char * message = exchange->message;
//    cJSON * database = exchange->database;
    int clientfd = exchange->clientfd;
    cJSON * temp = cJSON_Parse(message);
    if (cJSON_HasObjectItem(online, cJSON_GetObjectItem(temp, "id")->valuestring)) {
        online_changelist(online, cJSON_GetObjectItem(temp, "id")->valuestring, 0, 0);
        changelist(checklist_id, checklist_photo, checklist_status, cJSON_GetObjectItem(temp, "id")->valuestring, 0, 2);
        int i=0;
        for (; i<cJSON_GetArraySize(login_socketfd); i++) {
            if (clientfd==cJSON_GetArrayItem(login_socketfd, i)->valueint) {
                cJSON_DeleteItemFromArray(login_socketfd, i);
                break;
            }
        }
    }
    SendCheck(clientfd);
    view_database(login_socketfd);
    view_database(checklist_id);
    view_database(checklist_photo);
    view_database(checklist_status);
    view_database(online);
    return 0;
}
void * recv_lock(void * in){
    int maxcycle=5;
    int sign=1;
    int recvbytes;
    char message[MAXSIZE];//传输数据
    time_t t;
    char buf[1024];
    struct thread_data * exchange=(struct thread_data *)in;
    printf("--------------->%d,%s<---------------\n",exchange->clientfd,cJSON_Print(exchange->database));
    while (1) {
        memset(message, 0, sizeof(message));
//        signal(SIGPIPE, SIG_IGN);
        if((recvbytes=recv(exchange->clientfd, message, MAXSIZE, 0))==-1) {
            printf("2\n");
            maxcycle--;
            perror("Error(recv)");
        }
        if (strlen(message)==0) {
            continue;
        }
        printf("clientfd:%d\n",exchange->clientfd);
        printf("recvbytes:%d\n",recvbytes);
        printf("message:{%s}\n",message);
        printf("len:%d\n",(int)strlen(message));
        exchange->message=message;
        
        switch (service_selection(message,exchange->clientfd)) {
            case 0:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Merror((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 1:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Register((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 2:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Login((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 3:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Request((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 4:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Exchange((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 5:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Check((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 6:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Exit((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 7:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Change((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 8:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Upload((void *)exchange);
                Push((void*)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 9:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Download((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            case 10:
                printf("--------------------->time:%d,thread:%d,all:%d<---------------------\n",++service,exchange->threadid,thread_num);
                time(&t);
                ctime_r(&t,buf);
                printf("---->%s",buf);
                printf("clientfd:%d\n",exchange->clientfd);
                printf("received a message:\n{%s}\n",message);
                Emoji((void *)exchange);
                printf("--------------------->-------end-------<---------------------\n");
                sign=1;
                break;
            default:
                if (sign==0) {
                    maxcycle--;
                }
                sign = 0;
                break;
        }
        if (maxcycle==1) {
            break;
        }
    }
    threadstate[exchange->threadid]=0;
    thread_num--;
    pthread_exit(NULL);
}
void CS_Connect(cJSON * database){
    pthread_t thread[MAXONLINE];
    memset(&thread, 0, sizeof(thread));
    start:

    
    view_database(database);
    struct sockaddr_in server,client;//声明客户端和服务器的socket存储结构
    int sockfd,clientfd;//socket描述符
    socklen_t sin_size;//客户端ip
    char ip[INET_ADDRSTRLEN];
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1){//验证socketl连接建立是否成功
        perror("Error(socket)");
        sleep(5);
        goto start;
    }
    int value = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
    printf("Socket->success\n");//socket连接建立成功
    
    server.sin_family=AF_INET;//使用ipv4
    server.sin_port=htons(PORT);//端口设定
    server.sin_addr.s_addr=INADDR_ANY;//本机的ip均可使用
    bzero(&(server.sin_zero),8);//填充0
    printf("%d\n",sockfd);
    if((bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)))==-1){//验证bind是否成功
        perror("Error(bind)");
        sleep(5);
        goto start;
    }
    
    printf("Bind->success\n");//bind建立成功
    
    if(listen(sockfd, BACKLOG)==-1) {//listen监听建立成功验证
        perror("Error(listen)");
        sleep(5);
        goto start;
    }
    
    printf("listen->success\nlistening...\n");//建立监听成功
    
    while(1){
        struct thread_data *in;
        in = (struct thread_data *)malloc(sizeof(struct thread_data));
        sin_size=sizeof(struct sockaddr_in);
        if(((clientfd= accept(sockfd, (struct sockaddr *)&client, &sin_size)))==-1){//client连接成功验证
            perror("Error(accept)");
            sleep(5);
            continue;
        }
        int i=0;
        for (; i<MAXONLINE; i++) {
            if (threadstate[i]==0) {
                in->threadid=i;
                threadstate[i]=1;
                break;
            }
        }
        printf("----------------------------------------------->%d\n",in->threadid);
        socklen_t len = sizeof(client);
        getpeername(clientfd, (struct sockaddr *)&client, &len);
        
        in->ip = strdup(inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(client)));
        printf("ip is :%s\n", in->ip);
        in->database=database;
        in->clientfd=clientfd;
        printf("accept->success\n");//接收到请求
        if((pthread_create(&thread[in->threadid], NULL, recv_lock, (void *)in)) != 0) //comment2
            printf("线程创建失败!\n");
        else
            printf("线程被创建,当前线程数为：%d\n",++thread_num);
        
    }

}


int main() {
//    char a[10]="000123";
//    int n;
//    sscanf(a,"%d",&n);
//    printf("[%d]\n",n-123);
//    char str[] = "now # is the time for all # good men to come to the # aid of their country";
//    char delims[] = "#";
//    char *result = NULL;
//    result = strtok( str, delims );
//    while( result != NULL ) {
//        printf( "result is \"%s\"\n", result );
//        result = strtok( NULL, delims );
//    }
    
    
//    int a[1];
//    a[0]=2324;
//    printf("--------------->%c<--------------\n",RSA_Decode(a, 1, 3389, 2069));

    cJSON * database = open_database();
    cJSON * onlinelist = cJSON_CreateObject();
    cJSON * checkdata_id = cJSON_CreateArray();
    cJSON * checkdata_photo = cJSON_CreateArray();
    cJSON * checkdata_status = cJSON_CreateArray();
    cJSON * socket_fd = cJSON_CreateArray();
    online = onlinelist;
    checklist_id = checkdata_id;
    checklist_photo = checkdata_photo;
    checklist_status = checkdata_status;
    login_socketfd=socket_fd;

    
    //创建测试用户权限
//    online_changelist(onlinelist, "shiyan", 4, 2);
//    add_database(database, "huayu", "123456789", 2, "hahahaha");
//    add_database(database, "shiyan", "1574868663", 4, "zero");
    
//    int k,n;
//    get_pukey(database, "huayu",&k,&n);
//    printf("%s\n",get_password(database, "huayu"));
//    printf("%d\n",get_clientfd(database, "huayu"));
//    printf("%s\n",get_name(database, "huayu"));
//    printf("%d,%d\n",k,n);
//    view_database(database);
//    printf("----------------------%d\n",cJSON_HasObjectItem(database, "huayu"));
//    char * idb = "huayu";
//    cJSON * SMES =cJSON_CreateObject();
//    cJSON_AddItemToObject(SMES, "Mes_type", cJSON_CreateString("SC_Response"));
//    cJSON_AddItemToObject(SMES, "KEY", cJSON_CreateNumber(1));
//    int * kb = get_pukey(database, idb);
//    cJSON * key=cJSON_CreateObject();
//    cJSON_AddItemToObject(key, "K", cJSON_CreateNumber(*kb));
//    cJSON_AddItemToObject(key, "N", cJSON_CreateNumber(*(kb+1)));
//    view_database(key);
//    char * position = cJSON_Print(key);
//    char * secret1 = RSA_Encode(position, (int)strlen(position), 3389, 6319);
//    cJSON_AddItemToObject(SMES, "Msg", cJSON_CreateString(secret1));
//    view_database(SMES);
//    char * sign1 = "|";
//    char * id = "huayu";
//    char * sign2 = "#";
//    char * req = "CS_Request";
//    char * mess;
//    char * mess1;
//    mess = (char *)malloc((strlen(sign1)+strlen(id)+strlen(req)+1)*sizeof(char));
//    strcat(mess, id);
//    strcat(mess, sign2);
//    strcat(mess, req);
//    printf("MESS:[%s]\n",mess);
//    int * Msg= RSA_Encode(mess, (int)strlen(mess), 3389, 5429);
//    RSA_Decode(Msg, 16, 3389, 5429);
//    int test3[20];
//    for (int i=0; i<(int)strlen(mess); i++) {
//        test3[i]=Msg[i];
//        printf("%d.",i);
//        printf("%d.",*(int *)Msg);
//    }
//    printf("\n");
//    printf("----------------------------66666666-------------------------------\n");
//    RSA_Decode(test3, 16, 3389, 5429);
//    printf("\n");
//    mess1 = (char *)malloc((strlen(sign1)*2+strlen(Msg)+1)*sizeof(char));
//    strcat(mess1, sign1);
//    strcat(mess1, Msg);
//    strcat(mess1, sign1);
//    printf("MESS1:[%s]\n",mess1);
//    cJSON * test = cJSON_CreateObject();
//    cJSON_AddStringToObject(test, "Mes", mess1);
//    char * out = cJSON_Print(test);
//    printf("print1:[%s]\n",out);
//    cJSON * hello = cJSON_Parse(out);
//    char * out1 = cJSON_GetObjectItem(hello, "Mes")->valuestring;
//    printf("string:[%s]\n",out1);
//    printf("stringlen:%d\n",(int)strlen(out1));
//    printf("ans1:%d\n",strcmp(out1, mess1)==0);
//    char * in1 = (char *)malloc((strlen(out1)-1)*sizeof(char));
//    strncpy(in1, out1+1, strlen(out1)-2);
//    char * in2 = in1;
//
//
//    printf("in1:[%s]\n",in1);
//    printf("in1len:%d\n",(int)strlen(in1));
//    printf("ans2:%d\n",strcmp(in1, Msg)==0);
//    for (int i=0; i<(int)strlen(mess); i++) {
//        printf("%d.",i);
//        printf("%d.",test3[i]-*(int *)Msg);
//    }
//    printf("\n");
//    char * out2 = RSA_Decode((int *)in1, 16, 3389, 5429);
//    printf("out2:[%s]\n",out2);
//    printf("ans3:%d\n",strcmp(in1, RSA_Encode(mess,(int)strlen(mess), 3389, 5429))==0);
////    char * test3 =
//    char * test2 = RSA_Decode((int *)RSA_Encode(mess,(int)strlen(mess), 3389, 5429), 16,3389, 5429);
//    printf("secret:[%s]\n",test2);

    //modify_database(database, 3, "huayu", "是雁行");

    
//    view_database(database);
    CS_Connect(database);
//    pthread_t thread[MAXONLINE];
//    int temp;
//    memset(&thread, 0, sizeof(thread));
//    int a=1;
//    if((temp = pthread_create(&thread[0], NULL, CS_Connect, (void *)&a)) != 0) //comment2
//        printf("线程1创建失败!\n");
//    else
//        printf("线程1被创建\n");

    



    
    return 0;
}
