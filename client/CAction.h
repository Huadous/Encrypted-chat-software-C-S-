//
//  CAction.h
//  authentic
//
//  Created by 石雁航 on 2018/11/30.
//  Copyright © 2018 石雁航. All rights reserved.
//

#ifndef CAction_h
#define CAction_h
//封装出错函数
void sys_err(const char *ptr,int num)
{
    perror(ptr);
    exit(num);
}
//长整形转换为字符
void long2str (char *str, long int li) {
    int j = 0;
    long int div = 1000000000;
    for (int i = 0; i < 10; i++) {
        str[j++] = (li/div) + '0';
        li = li % div;
        div /= 10;
    }
}
char* int2str (int li) {
    int j = 0;
    int div = 10000;
    char *str = (char *)malloc(6*sizeof(char));
    for (int i = 0; i < 5; i++) {
        str[j++] = (li/div) + '0';
        li = li % div;
        div /= 10;
    }
    str[5] = '\0';
    return str;
}
//login
char * createLogin(char *id_s, char *pw_s) {
    time_t ut = time(NULL) + 60; //时间戳秒数
    
    char uts[11];
    //uts为时间戳的字符形式
    long2str(uts, ut);
    uts[10] = '\0';
    char *whole = (char *)malloc((strlen(id_s) + strlen(pw_s) + strlen(uts)+1)*sizeof(char));
    strcpy(whole, id_s);
    strcat(whole, pw_s);
    strcat(whole, uts);
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char result[16];  //result保存加密后的字符串
    MD5Update(&md5, (unsigned char *)whole, (unsigned int)strlen(whole));
    MD5Final(&md5, result);
    //printf("{%s}\n", result);
    cJSON * Array = cJSON_CreateArray();
    int i;
    for (i=0; i<16; i++) {
        cJSON_AddItemToArray(Array, cJSON_CreateNumber(result[i]));
    }
    //cJSON对象info保存登陆信息
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CS_Login"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id_s));
    cJSON_AddItemToObject(info, "time", cJSON_CreateString(uts));
    cJSON_AddItemToObject(info, "md5", Array);
    return cJSON_Print(info);
}
//request "Mes_type#TDme#len#M(IDto+req)" //now to add a friend
char * createRequest(char *id_me, char *id_to, Key myKey) {
    char *insert = "#";
    int minglen = (int)(strlen(id_to)+11);
    char *ming = (char *)malloc((minglen+1)*sizeof(char));
    strcpy(ming, id_to);
    strcat(ming, insert);
    strcat(ming, "CS_Request");
    ming[minglen] = '\0';
    int * secret = RSA_Encode(ming, minglen, myKey.k, myKey.n);
//    int reqlen = (int)strlen(secret) + 18 + (int)strlen(id_me);
//    char * req = (char *)malloc((reqlen+1)*sizeof(char));
//    strcpy(req, "CS_Request");
//    strcat(req, insert);
//    strcat(req, id_me);
//    strcat(req, insert);
//    strcat(req, secret);
//    strcat(req, insert);
//    strcat(req, int2str(minglen));
//    req[reqlen] = '\0';
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CS_Request"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id_me));
    
    cJSON *array = NULL;
    cJSON_AddItemToObject(info,"Msg",array=cJSON_CreateArray());
    for (int i = 0; i < minglen; i++) {
        cJSON_AddItemToArray(array,cJSON_CreateNumber(secret[i]));
    }
    return cJSON_Print(info);
}
//change password
char * createChange(char * id, char * password, char * newpassword) {
    time_t ut = time(NULL) + 60; //时间戳秒数
    
    char uts[11];
    //uts为时间戳的字符形式
    long2str(uts, ut);
    uts[10] = '\0';
    char *whole = (char *)malloc((strlen(id) + strlen(password) + strlen(uts)+1)*sizeof(char));
    strcpy(whole, id);
    strcat(whole, password);
    strcat(whole, uts);
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char result[16];  //result保存加密后的字符串
    MD5Update(&md5, (unsigned char *)whole, (unsigned int)strlen(whole));
    MD5Final(&md5, result);
    //printf("{%s}\n", result);
    cJSON * Array = cJSON_CreateArray();
    int i;
    for (i=0; i<16; i++) {
        cJSON_AddItemToArray(Array, cJSON_CreateNumber(result[i]));
    }
    //cJSON对象info保存登陆信息
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CS_Change"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id));
    cJSON_AddItemToObject(info, "time", cJSON_CreateString(uts));
    cJSON_AddItemToObject(info, "newpassword", cJSON_CreateString(newpassword));
    cJSON_AddItemToObject(info, "md5", Array);
    return cJSON_Print(info);
}
char * parseMsg(cJSON * info, Key myPrKey, Key myPuKey) {
    cJSON * N = cJSON_GetObjectItem(info, "n");
    const int randN = decodeSessionNumber(N->valueint, myPrKey);
    unsigned char * sessionKey = createSessionKey(randN, myPuKey);
    cJSON * secret = cJSON_GetObjectItem(info, "msg");
    cJSON * hash = cJSON_GetObjectItem(info, "md5");
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char result[16];  //result保存加密后的字符串
    MD5Update(&md5, (unsigned char *)(secret->valuestring), (unsigned int)strlen(secret->valuestring));
    MD5Final(&md5, result);
    cJSON * Array = cJSON_CreateArray();
    int i;
    for (i=0; i<16; i++) {
        cJSON_AddItemToArray(Array, cJSON_CreateNumber(result[i]));
    }
    int j=1;
    for (i=0; i<16; i++) {
       if (cJSON_GetArrayItem(Array, i)->valueint!=cJSON_GetArrayItem(hash, i)->valueint) {
            j=0;
	    break;
        }
    }
    if (j==0) {
        perror("消息不完整！");
        exit(1);
    }
    char * ming = RC4(secret->valuestring, initS(sessionKey));
    return ming;
}
//提取对方的key
cJSON * parseKey(cJSON * info, Key myPrKey) {
    cJSON * msg = cJSON_GetObjectItem(info, "Msg");
    int len = cJSON_GetArraySize(msg);
    int *arr = (int *)malloc(len*sizeof(int));
    for (int i = 0; i < len; i++) {
        arr[i] = cJSON_GetArrayItem(msg, i)->valueint;
    }
    char * ming = RSA_Decode(arr, len, myPrKey.k, myPrKey.n);
    cJSON * keyJSON = cJSON_Parse(ming);
    return keyJSON;
}
//注册
char * createRegister(char *id_s, char *pw_s, int photo) {
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CS_Register"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id_s));
    cJSON_AddItemToObject(info, "password", cJSON_CreateString(pw_s));
    cJSON_AddItemToObject(info, "photo", cJSON_CreateNumber(photo));
    return cJSON_Print(info);
}

//send a emoji
char * createEmoji(char * id_me, char * id_to, int emoji) {
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CC_Emoji"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id_me));
    cJSON_AddItemToObject(info, "fd", cJSON_CreateString(id_to));
    cJSON_AddItemToObject(info, "emoji", cJSON_CreateNumber(emoji));
    return cJSON_Print(info);
}

//ask for sent a file
char * createUpload(char * id_me, char * id_to, char * filename) {
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CS_Upload"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id_me));
    cJSON_AddItemToObject(info, "fd", cJSON_CreateString(id_to));
    cJSON_AddItemToObject(info, "filename", cJSON_CreateString(filename));
    return cJSON_Print(info);
}

char * createDownload(char * id_me, char * filename) {
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CS_Download"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id_me));
    //cJSON_AddItemToObject(info, "fd", cJSON_CreateString(id_to));
    cJSON_AddItemToObject(info, "filename", cJSON_CreateString(filename));
    return cJSON_Print(info);
}
//send the file asked before
void sendFile(const char * filename, const char * ip) {
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
    printf("open success\n");
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
    char recvf[30];
    strcpy(recvf, "./download/");
    strcat(recvf, filename);
    int accefd, sockfd;
    struct sockaddr_in seraddr,cliaddr;
    socklen_t len;
    
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
    seraddr.sin_port = htons(3000);
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
    while(1)
    {
        char buf[4096];
        len = sizeof(cliaddr);
        accefd = accept(sockfd,(struct sockaddr *)&cliaddr,&len);
        if(accefd < 0)
        {
            if(errno == EINTR)  //判断阻塞等待客户端的链接;是被信号打断还是其它因素
                continue;
            else
                sys_err("accept",-4);
        }
        //开始文件的读写操作
        memset(buf,0x00,sizeof(buf));
        int filefd = open(recvf,O_WRONLY |O_CREAT |O_TRUNC,0777);
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


#endif /* CAction_h */
