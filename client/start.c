#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#define SO_NOSIGPIPE  0x1022
#include "createKEY.h"
#include "RSA.h"
#include "md5.h"
#include "cJSON.h"
#include "Session.h"
#include "CAction.h"

#include <gtk/gtk.h>
#define dest_width 80
#define dest_height 80
#define SERVPORT 4000
#define FILEPORT 3000
#define IP "127.0.0.1"
#define MAXDATASIZE 4096
#define BACKLOG 10

typedef struct registerinfo
{
    char * name;
    char * password;
    int photo;
}registerinf;
typedef struct logininfo
{
    char name[11];
    char password[11];
    int photo;
}logininf;
typedef struct message
{
    int type;
    char * name;
    char * text;
    int emoji;
}mes;
cJSON * windowinf;
typedef struct chat_data
{
    char * towho;
    GtkWidget * emoji1;
    GtkWidget * emoji2;
    GtkWidget * emoji3;
    GtkWidget * emoji4;
    GtkWidget * emoji5;
    GtkWidget * emoji6;
    GtkWidget * emoji7;
    GtkWidget * emoji8;
    GtkWidget * emoji9;
    GtkWidget * emoji10;
    GtkWidget * emoji11;
    GtkWidget * emoji12;
    GtkWidget * window1;
    GtkBuilder * builder11;
    GtkWidget * dialog;
    GtkWidget * emoji;
    GtkWidget * view;
    GtkWidget * adjustment;
    GtkWidget * button1;
    GtkWidget * button2;
    GtkWidget * button3;
    GtkWidget * parent_window;
    GtkWidget * scrolledwindow;
    GtkWidget * chat_entry1;
    GtkWidget * closebutton;
    GtkListStore* model;
}chat_data;
chat_data chat_window[50];
int chatwindownum=0;
registerinf inf1;
logininf inf2;
int chatsignal=0;

GtkWidget * online_num;
GtkWidget * all_num;
GtkTreeSelection * imp;

int Check_len;
GtkWidget * net;
GtkWidget * photo;
GtkWidget * registerphoto;
GtkWidget * revealer;
GtkWidget * revealer1;
GtkWidget * loginentry1;
GtkWidget * loginentry2;
GtkBuilder * builder;
GtkWidget * builder1;

enum{   
    PIXBUF_COL,  
    LIST_ITEM,
    PIXBUF_COL1,
    N_COLUMN 
};  
typedef struct listdata{
    int pic;
    char * name;
    int status;
}data;

pthread_t thread[1];
cJSON * pubKeyData;
data list_data[50];
int windowsignal;
int initsignal=0;
int conn = 0;
int registersignal=0;
int loginsignal=0;
int numofonline;
char myid[11] = "shiyanhang";
char pw[11] = "1574868663";

char id_to[11];
char mymsg[200]; //the msg i want to send
char buf[MAXDATASIZE];
char sentmsg[MAXDATASIZE];
char result[MAXDATASIZE];
int emojiN;
GtkWidget * list_view;
GtkWidget* list_scrolledwindow;

Client me;
struct hostent* host;
struct sockaddr_in serv_addr;
int sockfd,  sin_size;
long sendbytes, recvbytes;
int chatsign=0;

char *find_file_name(char *name)
{
    char *name_start = NULL;
    int sep = '/';
    if (NULL == name) {
        printf("the path name is NULL\n");
        return NULL;
    }
    name_start = strrchr(name, sep);
    
    return (NULL == name_start)?name:(name_start + 1);
}
//发消息
char * createMsg(char *msg, char *id_me, char *id_to) {
    cJSON *info = cJSON_CreateObject();
    const int randN = createSessionNumber();
//    printf("randN:%d\n",randN);
    Key ekey;
    cJSON * key_json = cJSON_GetObjectItem(pubKeyData, id_to);
    ekey.k =  cJSON_GetObjectItem(key_json, "K")->valueint;
    ekey.n =  cJSON_GetObjectItem(key_json, "N")->valueint;

    const int N = encodeSessionNumber(randN, ekey);
//    printf("N:%d\n",N);
//    printf("K:%d,N:%d\n",ekey.k,ekey.n);
    unsigned char * sessionKey = createSessionKey(randN, ekey);
//    printf("sessionkey:%s\n",sessionKey);
    char *secret = RC4(msg, initS(sessionKey));
//    printf("inits:{%s}\n",initS(sessionKey));
//    printf("len:%d\n",(int)strlen(initS(sessionKey)));
    
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("CS_Message"));
   // cJSON_AddItemToObject(info, "N", cJSON_CreateString(N)); //加密后的随机数
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(id_me));
    cJSON_AddItemToObject(info, "fd", cJSON_CreateString(id_to));
    cJSON_AddItemToObject(info, "n", cJSON_CreateNumber(N));
    cJSON_AddItemToObject(info, "msg", cJSON_CreateString(secret));//加密后的消息s
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char result[16];  //result保存加密后的字符串
    MD5Update(&md5, (unsigned char *)secret, (unsigned int)strlen(secret));
    MD5Final(&md5, result);
    cJSON * Array = cJSON_CreateArray();
    int i;
    for (i=0; i<16; i++) {
        cJSON_AddItemToArray(Array, cJSON_CreateNumber(result[i]));
    }
    cJSON_AddItemToObject(info, "md5", Array);
    return cJSON_Print(info);
}
char * createCheck() {
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("Check"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(inf2.name));
    return cJSON_Print(info);
}
char * createExit() {
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "Mes_type", cJSON_CreateString("Exit"));
    cJSON_AddItemToObject(info, "id", cJSON_CreateString(inf2.name));
    return cJSON_Print(info);
}
int Isitin(char * name)
{
    int i=0;
    for(;i<Check_len;i++){
        if(strcmp(name,list_data[i].name)==0)
            return 1;
    }
    return 0;
}
void chatwindow(char * towho);
void ssent(char * msg);
int findchatwindow(char * towho);
int num=0;
void list_handled(GtkTreeSelection *selection, gpointer data)
{       
        printf("--------------------------->%d\n",num++);
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *selected_name;
        if(chatsignal==0){
            chatsignal=1;
            int i=0;
            for(;i<Check_len-1;i++)
            {   
                chatwindow(list_data[i].name);
            }
            chatsignal=0;
        }
        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, LIST_ITEM, &selected_name, -1);
                printf("{selected%s}\n",selected_name);
                ssent(createRequest(inf2.name, selected_name, me.myPrKey));
                printf("%s\n",cJSON_Print(windowinf));
                gtk_widget_show_all(chat_window[findchatwindow(selected_name)].window1);
                printf("creat-chatwindow\n");
                
                printf("finished-chatwindow\n");
        }
        //
        
        
        gtk_tree_selection_unselect_all(selection);
        
}

GtkWidget * list_showPic(char *filename)
{
    GtkWidget *image;
    const GdkPixbuf *src_pixbuf;
    GdkPixbuf *dest_pixbuf;

    //读取图片参数
    src_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

    //将src_pixbuf设置成屏幕大小
    dest_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf, 
        dest_width, dest_height, GDK_INTERP_HYPER);

    //从dest_pixbuf中读取图片存于image中
    image = gtk_image_new_from_pixbuf(dest_pixbuf);

    return image;

}
GdkPixbuf * list_situation(int i)
{
    GdkPixbuf *src_pixbuf;

    //读取图片参数
    if(i==1){
        src_pixbuf = gdk_pixbuf_new_from_file("./situation/ok.png", NULL);
    }else{
        src_pixbuf = gdk_pixbuf_new_from_file("./situation/kill.png", NULL);
    }
    return src_pixbuf;

}
GdkPixbuf * list_Pic(char *filename)
{
    const GdkPixbuf *src_pixbuf;
    GdkPixbuf *dest_pixbuf;

    //读取图片参数
    src_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

    //将src_pixbuf设置成屏幕大小
    dest_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf, 
        50, 50, GDK_INTERP_HYPER);

    return dest_pixbuf;

}

char * list_picselection(int i){
    char * base="./pic/";
    char choose[2];
    sprintf(choose, "%d", i);
    char * end=".jpg";
    char * target = (char *)malloc((strlen(base) + strlen(choose) + strlen(end)+1)*sizeof(char));
    strcpy(target, base);
    strcat(target, choose);
    strcat(target, end);
    printf("{1%s}\n",target);
    return target;

}

GtkListStore * list_create_list_model(int onlinenum , data * onlinelist)  
{  
    GtkListStore *list_store;  
    GtkTreeIter iter;  
    gint i;  
     
    /*创建一个存储，行数和对应的数据类型列表*/ 
    printf("1\n"); 
    list_store = gtk_list_store_new(N_COLUMN,GDK_TYPE_PIXBUF,G_TYPE_STRING,GDK_TYPE_PIXBUF); 
    printf("1\n"); 
    printf("---%d\n",onlinenum);
    for(i = 0;i < onlinenum;i++){  
        /*向LIST_trore添加一个新行 
         * iter将指向这个新行，这个函数调用后事空的 
         * 需要gtk_list_store_set()函数来填写数值 
         * */  
    printf("{2%s}{%d}{%d}\n",onlinelist[i].name,onlinelist[i].pic,onlinelist[i].status);
        gtk_list_store_append(list_store,&iter);  
        gtk_list_store_set(list_store,&iter,PIXBUF_COL,list_Pic(list_picselection(onlinelist[i].pic)),  
                           LIST_ITEM,onlinelist[i].name, PIXBUF_COL1, list_situation(onlinelist[i].status),
                           -1); 
    }  
    printf("1\n");
    return list_store;  
} 

void refresh_item(GtkWidget *list, int onlinenum , data * onlinelist )
{
  GtkListStore *list_store;
  GtkTreeIter  iter;
    
  list_store = GTK_LIST_STORE(gtk_tree_view_get_model(
       GTK_TREE_VIEW(list)));
    int i;
    for(i = 0;i < onlinenum;i++){  
        /*向LIST_trore添加一个新行 
         * iter将指向这个新行，这个函数调用后事空的 
         * 需要gtk_list_store_set()函数来填写数值 
         * */  
        printf("----->{%s}{%d}{%d}\n",onlinelist[i].name,onlinelist[i].pic,onlinelist[i].status);
        gtk_list_store_append(list_store,&iter);  
        gtk_list_store_set(list_store,&iter,PIXBUF_COL,list_Pic(list_picselection(onlinelist[i].pic)),  
                           LIST_ITEM,onlinelist[i].name, PIXBUF_COL1, list_situation(onlinelist[i].status),
                           -1); 
    } 

}
void
remove_all(GtkWidget *list)
{
  GtkListStore *store;
  GtkTreeModel *model;
  GtkTreeIter  iter;


  store = GTK_LIST_STORE(gtk_tree_view_get_model(
      GTK_TREE_VIEW (list)));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

  if (gtk_tree_model_get_iter_first(model, &iter) == FALSE) 
      return;
  gtk_list_store_clear(store);
}

GtkWidget* list_create_list(GtkListStore* list_store)  
{  
    GtkWidget* view;  
    GtkTreeModel* model;  
    GtkCellRenderer* renderer;  
    GtkTreeViewColumn* column;  
    GtkWidget * button;
    printf("2\n");
   model = GTK_TREE_MODEL(list_store);  
    /*创建一个模型初始化model的控件*/  
    printf("2\n");
    view  = gtk_tree_view_new_with_model(model);
    printf("2\n");

    
    /*创建一个文本单元绘制器*/  
    renderer = gtk_cell_renderer_pixbuf_new();  
    /*创建一个视图列表*/  
    printf("2\n");
    column = gtk_tree_view_column_new_with_attributes(" photo ",renderer,"pixbuf", PIXBUF_COL,NULL);  
    /*附加一列列表*/  
    printf("2\n");
    gtk_tree_view_append_column(GTK_TREE_VIEW(view),column);  
      printf("2\n");
    renderer = gtk_cell_renderer_text_new();  
    //  printf("2\n");
    column = gtk_tree_view_column_new_with_attributes(" user name ",renderer,"text",LIST_ITEM,NULL);  
    gtk_tree_view_append_column(GTK_TREE_VIEW(view),column);  
    printf("2\n");
    //gtk_tree_view_column_set_clickable(column,TRUE);
    // renderer = gtk_cell_renderer_toggle_new();
    // column = gtk_tree_view_column_new_with_attributes("  On/Off  ",renderer,"active",TOGGLE_COLUMN,NULL);
    // renderer = gtk_cell_renderer_toggle_new();  
    // /*设置控件属性*/  
    // g_object_set(G_OBJECT(renderer),"activatable",TRUE,NULL); 
    // /*设置开关单元绘制器为：当为TURE时为单元按钮，为FAULE时为多选按钮*/  
    // gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer),TRUE);  
    renderer = gtk_cell_renderer_pixbuf_new();  
    /*创建一个视图列表*/  printf("2\n");
    column = gtk_tree_view_column_new_with_attributes(" On/Off",renderer,"pixbuf", PIXBUF_COL1,NULL);  
    /*附加一列列表*/  printf("2\n");
    gtk_tree_view_append_column(GTK_TREE_VIEW(view),column);
    
    return view;  
}  
void ssent(char * msg) {
    printf("%d %s\n", sockfd, msg);
    if ((sendbytes = send(sockfd, msg, MAXDATASIZE, 0)) == -1) {
        perror("send");
        exit(1);
    }
}
void closewindow(GObject *object, gpointer entry) {
    ssent(createExit());
    gtk_main_quit();
}

char * picselection(int i){
    char * base="./pic/";
    char choose[2];
    sprintf(choose, "%d", i);
    char * end=".jpg";
    char * target = (char *)malloc((strlen(base) + strlen(choose) + strlen(end)+1)*sizeof(char));
    strcpy(target, base);
    strcat(target, choose);
    strcat(target, end);
    printf("{3%s}\n",target);
    return target;

}
void list_changed(GtkTreeView * treeview,gpointer user_data){
    printf("--------------->\n");
}
void listwindow()
{
    GtkBuilder * builder;
    GtkWidget * window1;
    GtkWidget * hiname;
    GtkWidget * box; 
    GtkWidget * frame;  
    GtkListStore* model;
    GtkTreeSelection * select;

    data onlinelist[100];

    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "./ui/list/list.glade", NULL);//读取button.xml文件
    gtk_builder_connect_signals (builder, NULL);

    window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    window1 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder), "window1"));
    hiname = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder), "label8"));
    box = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder), "box5"));
    online_num = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder), "label6"));
    all_num = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder), "label7"));
    list_scrolledwindow = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder), "scrolledwindow1"));
    gtk_label_set_text(GTK_LABEL(hiname),inf2.name);
    frame = gtk_frame_new("Head portrait");   
    gtk_container_add(GTK_CONTAINER(frame),list_showPic(picselection(inf2.photo)));
    gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
//test
  

    list_view = list_create_list(list_create_list_model(0,NULL));
    imp = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
    //gtk_tree_selection_set_mode(GTK_TREE_SELECTION(imp),GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_set_mode (imp, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (list_view), "unselect-all", G_CALLBACK (list_changed), NULL);
	g_signal_connect (G_OBJECT (imp), "changed", G_CALLBACK (list_handled), NULL);
    gtk_container_add(GTK_CONTAINER(list_scrolledwindow),list_view); 

    gtk_widget_show_all(window1);
    printf("1\n");


    g_signal_connect(G_OBJECT(window1), "destroy",G_CALLBACK(closewindow), NULL);
}
int findchatwindow(char * towho){
    printf("%s\n",cJSON_Print(windowinf));
    printf("find%d\n",cJSON_GetArrayItem(cJSON_GetObjectItem(windowinf,towho),0)->valueint);
    return cJSON_GetArrayItem(cJSON_GetObjectItem(windowinf,towho),0)->valueint;
}
char * idexchange(char * name){
    char * base=" :";
    char * target = (char *)malloc((strlen(base)+ strlen(name) + 1)*sizeof(char));
    strcpy(target, name);
    strcat(target, base);
    return target;
}


GdkPixbuf * chat_Pic(char *filename)
{
    const GdkPixbuf *src_pixbuf;
    GdkPixbuf *dest_pixbuf;

    //读取图片参数
    src_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

    //将src_pixbuf设置成屏幕大小
    dest_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf, 16, 16, GDK_INTERP_HYPER);

    return dest_pixbuf;

}
char * emojiselection(int i){
    char * base="./emoji/";
    char choose[2];
    sprintf(choose, "%d", i);
    char * end=".png";
    char * target = (char *)malloc((strlen(base) + strlen(choose) + strlen(end)+1)*sizeof(char));
    strcpy(target, base);
    strcat(target, choose);
    strcat(target, end);
    printf("{4%s}\n",target);
    return target;

}

void addmessage(mes * message,char * towho) 
{  
    GtkListStore * list_store_now;  
    GtkTreeIter iter;  
    gint i;  
    time_t timep,Tim;
    struct tm *p;
    time(&timep);
    p =  localtime(&timep);
    int Hour = p->tm_hour;
    int Minute = p->tm_min;
    int Second = p->tm_sec;
    char * base=":";
    char h[2];
    char m[2];
    char s[2];
    sprintf(h, "%d", Hour);
    sprintf(m, "%d", Minute);
    sprintf(s, "%d", Second);
    char * target = (char *)malloc((strlen(base)*2 + strlen(h) + strlen(m) +strlen(s) + 1)*sizeof(char));
    strcpy(target, h);
    strcat(target, base);
    strcat(target, m);
    strcat(target, base);
    strcat(target, s);
    list_store_now = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(chat_window[findchatwindow(towho)].view)));    
    gtk_list_store_append(list_store_now,&iter);  
    if(message->type==1){
        gtk_list_store_set(list_store_now,&iter,0,message->name,2,target,-1);
        gtk_list_store_append(list_store_now,&iter); 
        gtk_list_store_set(list_store_now,&iter,2,message->text,-1);
    }else if(message->type==2){
        gtk_list_store_set(list_store_now,&iter,0,message->name,2,target,-1);
        gtk_list_store_append(list_store_now,&iter); 
        gtk_list_store_set(list_store_now,&iter,1,chat_Pic(emojiselection(message->emoji)),-1);
    }else{
        ;
    }
        
}
 
void sendwindow(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    gint res;
    res = gtk_dialog_run (GTK_DIALOG (chat_window[findchatwindow(a)].dialog));
    if (res == GTK_RESPONSE_ACCEPT){
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (chat_window[findchatwindow(a)].dialog);
        filename = gtk_file_chooser_get_filename (chooser);
        ssent(createUpload(inf2.name, a, filename));
        printf("{5%s}\n",filename);
        //g_free (filename);
        gtk_widget_hide(chat_window[findchatwindow(a)].dialog);
    }else{
        gtk_widget_hide(chat_window[findchatwindow(a)].dialog);
    }        
}

void emojiwindow(GObject *object, gpointer user_data){
        char * a = (char *)user_data; 
    if(gtk_revealer_get_child_revealed(GTK_REVEALER(chat_window[findchatwindow(a)].emoji))){
        gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
    }else{
        gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),TRUE);

    }

        
}
void emojisend1(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=1;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,1));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend2(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=2;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,2));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend3(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=3;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,3));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend4(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=4;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,4));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend5(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=5;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,5));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend6(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=6;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,6));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend7(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=7;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,7));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend8(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=8;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,8));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend9(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=9;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,9));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend10(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=10;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,10));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend11(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=11;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,11));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}
void emojisend12(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    mes message;
    message.emoji=12;
    message.type=2;
    message.name=idexchange(inf2.name);
    ssent(createEmoji(inf2.name,a,12));
    addmessage(&message,a);
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[findchatwindow(a)].emoji),FALSE);
}

GtkListStore * chat_create_list_model( )  
{  
    GtkListStore *list_store;  
    GtkTreeIter iter;  
    gint i;  
     
    /*创建一个存储，行数和对应的数据类型列表*/  
    list_store = gtk_list_store_new(3,G_TYPE_STRING,GDK_TYPE_PIXBUF,G_TYPE_STRING);  
    return list_store;  
} 



GtkWidget* chat_create_list(GtkListStore* list_store)  
{  
    GtkWidget* view;  
    GtkTreeModel* model;  
    GtkCellRenderer* renderer;  
    GtkTreeViewColumn* column;  
    GtkWidget * button;
    //GtkTreeSelection *select;

   model = GTK_TREE_MODEL(list_store);  
    /*创建一个模型初始化model的控件*/  
    view  = gtk_tree_view_new_with_model(model);

    

      
    renderer = gtk_cell_renderer_text_new();  
    //  
    column = gtk_tree_view_column_new_with_attributes("",renderer,"text",0,NULL);  
    gtk_tree_view_append_column(GTK_TREE_VIEW(view),column); 
    renderer = gtk_cell_renderer_pixbuf_new();  
    /*创建一个视图列表*/  
    column = gtk_tree_view_column_new_with_attributes("em",renderer,"pixbuf",1,NULL);  
    /*附加一列列表*/  
    gtk_tree_view_column_set_fixed_width(column,16);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view),column);
    renderer = gtk_cell_renderer_text_new();  
    //  
    column = gtk_tree_view_column_new_with_attributes("text",renderer,"text",2,NULL);  
    //gtk_tree_view_column_set_fixed_width(column,200);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view),column); 
  
    return view;  
}  

void changeview(GObject *object, gpointer user_data){
    char * a = (char *)user_data;
    double lower = gtk_adjustment_get_lower(GTK_ADJUSTMENT(chat_window[findchatwindow(a)].adjustment));
    double upper = gtk_adjustment_get_upper(GTK_ADJUSTMENT(chat_window[findchatwindow(a)].adjustment));
    double value = gtk_adjustment_get_value(GTK_ADJUSTMENT(chat_window[findchatwindow(a)].adjustment));
    double pages = gtk_adjustment_get_page_size(GTK_ADJUSTMENT(chat_window[findchatwindow(a)].adjustment));
    printf("{2%d,%d,%d,%d}\n",(int)lower,(int)upper,(int)value,(int)pages);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(chat_window[findchatwindow(a)].adjustment),upper-pages);
}
void entrymessage(GObject *object, gpointer user_data){
    char * a = (char *)user_data; 
    char *entry_text;
    mes message;
    message.text =  gtk_entry_get_text(GTK_ENTRY(chat_window[findchatwindow(a)].chat_entry1));
    message.type=1;
    message.name=idexchange(inf2.name);
    printf("%s\n",message.name);
    ssent(createMsg(message.text, inf2.name,a));
    addmessage(&message,a);
    gtk_entry_set_text(GTK_ENTRY(chat_window[findchatwindow(a)].chat_entry1),"");
}
void hidewindow(GObject *object, gpointer user_data){
    gtk_widget_hide(user_data);
}
void clean(GObject *object, gpointer user_data){
    char * towho = (char *)user_data; 
    cJSON_GetArrayItem(cJSON_GetObjectItem(windowinf,towho),1)->valueint=0;
}
void chatwindow(char * towho1)
{
    char * towho = (char *)malloc((strlen(towho1)+1)*sizeof(char));
    strcpy(towho,towho1); 
    int i=0;
    int j=0;
    // for(;i<Check_len-1;i++)
    // {
    //     if(strcmp(towho,list_data[i].name)==0){
    //         if(list_data[i].status==0)
    //         return;
    //     }
    // }
    int windowshow;
    if(cJSON_HasObjectItem(windowinf,towho))
    {
        if(cJSON_GetArrayItem(cJSON_GetObjectItem(windowinf,towho),1)->valueint==1)
        {
            printf("open->open\n");
            return;
        }
        windowshow=cJSON_GetArrayItem(cJSON_GetObjectItem(windowinf,towho),0)->valueint;   
        printf("open->close\n");
    }else{
        printf("close\n");
        windowshow=chatwindownum++;
        cJSON * array;
        cJSON_AddItemToObject(windowinf,towho, array = cJSON_CreateArray());
        cJSON_AddItemToArray(array,cJSON_CreateNumber(windowshow));
        cJSON_AddItemToArray(array,cJSON_CreateNumber(1));
        printf("%s\n",cJSON_Print(windowinf));
    }  
    printf("windowshow:%d\n",windowshow);
    printf("a11111111111111111111111111\n");
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    chat_window[windowshow].builder11 = gtk_builder_new ();
    gtk_builder_add_from_file (chat_window[windowshow].builder11, "./ui/chat/chatwindow.glade", NULL);//读取button.xml文件
    gtk_builder_connect_signals (chat_window[windowshow].builder11, NULL);
    printf("b11111111111111111111111111\n");
    chat_window[windowshow].window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    chat_window[windowshow].parent_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    chat_window[windowshow].window1 = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "window1"));
    gtk_window_set_title(GTK_WINDOW(chat_window[windowshow].window1),towho);
    chat_window[windowshow].chat_entry1 = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "entry1"));
    chat_window[windowshow].adjustment = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "adjustment1"));
    chat_window[windowshow].scrolledwindow = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "scrolledwindow1"));
    chat_window[windowshow].button1 = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "button1"));
    chat_window[windowshow].button2 = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "button2"));
    chat_window[windowshow].closebutton = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "button6"));
    chat_window[windowshow].emoji = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "revealer1"));
    gtk_revealer_set_reveal_child(GTK_REVEALER(chat_window[windowshow].emoji),FALSE);
    chat_window[windowshow].emoji1 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji1"));
    chat_window[windowshow].emoji2 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji2"));
    chat_window[windowshow].emoji3 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji3"));
    chat_window[windowshow].emoji4 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji4"));
    chat_window[windowshow].emoji5 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji5"));
    chat_window[windowshow].emoji6 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji6"));
    chat_window[windowshow].emoji7 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji7"));
    chat_window[windowshow].emoji8 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji8"));
    chat_window[windowshow].emoji9 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji9"));
    chat_window[windowshow].emoji10 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji10"));
    chat_window[windowshow].emoji11 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji11"));
    chat_window[windowshow].emoji12 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(chat_window[windowshow].builder11), "emoji12"));
    printf("c11111111111111111111111111\n");
    chat_window[windowshow].dialog = gtk_file_chooser_dialog_new ("Open File",
                                      GTK_WINDOW(chat_window[windowshow].parent_window),
                                      action,
                                      "_Cancel",
                                      GTK_RESPONSE_CANCEL,
                                      "_Open",
                                      GTK_RESPONSE_ACCEPT,
                                      NULL);
    printf("d11111111111111111111111111\n");
    chat_window[windowshow].button3 = GTK_WIDGET (gtk_builder_get_object (chat_window[windowshow].builder11, "button5"));
    //gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),"../");
    chat_window[windowshow].model = chat_create_list_model();
    chat_window[windowshow].view = chat_create_list(chat_window[windowshow].model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(chat_window[windowshow].view),FALSE);
    printf("e11111111111111111111111111\n");
    gtk_container_add(GTK_CONTAINER(chat_window[windowshow].scrolledwindow),chat_window[windowshow].view);
    g_signal_connect(G_OBJECT(chat_window[windowshow].adjustment), "changed",G_CALLBACK(changeview),(gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].button3), "clicked", G_CALLBACK(sendwindow), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].button2), "clicked", G_CALLBACK(emojiwindow), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].button1), "clicked", G_CALLBACK(entrymessage), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].chat_entry1), "activate", G_CALLBACK(entrymessage), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji1), "clicked", G_CALLBACK(emojisend1), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji2), "clicked", G_CALLBACK(emojisend2), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji3), "clicked", G_CALLBACK(emojisend3), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji4), "clicked", G_CALLBACK(emojisend4), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji5), "clicked", G_CALLBACK(emojisend5), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji6), "clicked", G_CALLBACK(emojisend6), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji7), "clicked", G_CALLBACK(emojisend7), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji8), "clicked", G_CALLBACK(emojisend8), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji9), "clicked", G_CALLBACK(emojisend9), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji10), "clicked", G_CALLBACK(emojisend10), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji11), "clicked", G_CALLBACK(emojisend11), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].emoji12), "clicked", G_CALLBACK(emojisend12), (gpointer)(towho));
    g_signal_connect(G_OBJECT(chat_window[windowshow].closebutton), "clicked", G_CALLBACK(hidewindow), chat_window[windowshow].window1);
    //gtk_window_set_decorated(GTK_WINDOW(window1),FALSE);
    printf("f11111111111111111111111111\n");
    gtk_widget_show_all(chat_window[windowshow].window1);
    gtk_widget_hide(chat_window[windowshow].window1);
    cJSON_GetArrayItem(cJSON_GetObjectItem(windowinf,towho),1)->valueint=1;
    
    //g_signal_connect(G_OBJECT(window1), "destroy", G_CALLBACK(hidewindow), window1);
    g_signal_connect(G_OBJECT(chat_window[windowshow].window1), "destroy",G_CALLBACK(clean), (gpointer)(towho));
    printf("chatwindow_finished");
}

GdkPixbuf * situation(int i)
{
    GdkPixbuf *src_pixbuf;

    //读取图片参数
    if(i==1){
        src_pixbuf = gdk_pixbuf_new_from_file("./situation/ok.png", NULL);
    }else{
        src_pixbuf = gdk_pixbuf_new_from_file("./situation/kill.png", NULL);
    }
    return src_pixbuf;

}
GdkPixbuf * Pic(char *filename)
{
    const GdkPixbuf *src_pixbuf;
    GdkPixbuf *dest_pixbuf;

    //读取图片参数
    src_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

    //将src_pixbuf设置成屏幕大小
    dest_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf, 
        80, 80, GDK_INTERP_HYPER);

    return dest_pixbuf;

}

char * parseRecv(char * rec, Client *me) {
    char * ret = (char *)malloc(200 * sizeof(char));
    strcpy(ret, "error");
    cJSON * recv = cJSON_Parse(rec);
    // oops! don't know what it is
    if (recv == NULL||cJSON_GetObjectItem(recv, "Mes_type")==0) {
        return "";
    }
    cJSON * key = cJSON_GetObjectItem(recv, "KEY");
    cJSON * mes_type = cJSON_GetObjectItem(recv, "Mes_type");
    char * type = mes_type->valuestring;
  //message
    if (!strcmp(type, "CS_Message") && me->Login) {
        //gtk_tree_selection_unselect_all(imp);
        cJSON * id_from = cJSON_GetObjectItem(recv, "id");
        printf("%s : ", id_from->valuestring);
        //msg is the correct message
        char *msg = parseMsg(recv, me->myPrKey, me->myPuKey);
        strcpy(result, msg);
        strcpy(id_to, cJSON_GetObjectItem(recv, "id")->valuestring);
        // int i=0;
        // int j=0;
        // for(;i<chatwindownum;i++)
        // {
        //     if(strcmp(id_to,chat_window1[i].towho)==0){
        //         j=1;
        //     }
        // }
        // if(j==0)
        // {
        printf("x11111111111111111111111111\n");
        //chatwindow(id_from->valuestring);
        //}s
        gtk_widget_show_all(chat_window[findchatwindow(id_from->valuestring)].window1);
        
        printf("y11111111111111111111111111\n");
        mes message;
        message.text = msg;
        message.type=1;
        message.name=idexchange(id_from->valuestring);
        addmessage(&message,id_from->valuestring);
        // printf("311111111111111111111111111\n");
    }
//emoji
    else if (!strcmp(type, "CC_Emoji")) {
        emojiN = cJSON_GetObjectItem(recv, "emoji")->valueint;
        strcpy(id_to, cJSON_GetObjectItem(recv, "id")->valuestring);
        printf("%d\n", emojiN);
        printf("%s\n", id_to);
        mes message;
        message.emoji=emojiN;
        message.type=2;
        message.name=idexchange(id_to);
        addmessage(&message,id_to);    
    }
 //check other clients' state
    else if (!strcmp(type, "Check")) {
        cJSON * idlist = cJSON_GetObjectItem(recv, "id");
        cJSON * photolist = cJSON_GetObjectItem(recv, "photo");
        cJSON * statelist = cJSON_GetObjectItem(recv, "status");
        Check_len = cJSON_GetArraySize(idlist);
        int j = 0;
        numofonline=0;
	    for(int i = 0; i < Check_len; i++){
            if(strcmp(cJSON_GetArrayItem(idlist, i)->valuestring, inf2.name) == 0)
                continue;
            list_data[j].name = cJSON_GetArrayItem(idlist, i)->valuestring;
            list_data[j].pic = cJSON_GetArrayItem(photolist, i)->valueint;
            list_data[j].status = cJSON_GetArrayItem(statelist, i)->valueint;
            if(list_data[j].status==1)
            numofonline++;
            j++;
	    }
        char num1[3];
        char num2[3];
        sprintf(num1, "%d", numofonline+1);
        sprintf(num2, "%d", Check_len);
        gtk_label_set_text(GTK_LABEL(online_num),num1);
        gtk_label_set_text(GTK_LABEL(all_num),num2);
        remove_all(list_view);
        refresh_item(list_view,Check_len-1,list_data);
        // int i=0;
        // int signal=1;
        // for(;i<Check_len-1;i++)
        // {
        //     if(list_data[i].status==1){
        //         printf("%d,{%s}\n",i,list_data[i].name);
        //         int j=0;
        //         for(;j<chatwindownum;j++)
        //         {
        //             if(strcmp(list_data[i].name,chat_window[j].towho)==0){
                        
        //                 signal=0;
        //                 break;
        //             }
        //         }
        //         if(signal==1)
        //         {   
        //             printf("creat");
        //             chatwindow(list_data[i].name);
        //             signal=1;
        //         }
        //     }
            
        // }
    printf("1111111111111111111111111111111\n");
    
	    return "update\n";
    }
//add friend response
    else if (!strcmp(type, "SC_Response")) {
	    if(key->valueint){
        	strcpy(ret, "开始聊天");
		    chatsign=1;
        	strcpy(id_to, cJSON_GetObjectItem(recv, "id")->valuestring);
        	strcpy(ret, "add friends");
            //ekey is the json-object pre the friend's pukey
        	cJSON * ekey = parseKey(recv, me->myPrKey);
//            me->eKey.k = cJSON_GetObjectItem(ekey, "K")->valueint;
//        	me->eKey.n = cJSON_GetObjectItem(ekey, "N")->valueint;
//            me->friendList[me->friendNum].id = strdup(id_to);
//            me->friendList[me->friendNum].puKey = me->eKey;
//            me->friendNum ++;
        	cJSON_AddItemToObject(pubKeyData, id_to, ekey);
	    }else if (key->valueint==-1) {
            strcpy(ret, "对方不在线");
        }else{
	        strcpy(ret, "没有该用户");
	    }
    }
//Login response
    else if (!strcmp(type, "SC_Login_Success") && key->valueint) {
        strcpy(ret, "登陆成功\n");
        me->Login = 1;
        loginsignal = 1;
        inf2.photo = cJSON_GetObjectItem(recv, "photo")->valueint;
        me->pic = cJSON_GetObjectItem(recv, "photo")->valueint;
    }
//Regist response
    else if (!strcmp(type, "SC_Regist_Success")) {
	    if(key->valueint) {
            registersignal = 1;
        	strcpy(ret, "注册成功\n");
        }
	    else
		    strcpy(ret, "该用户名已被注册\n");
    }
//friend add you
    else if (!strcmp(type, "SC_NewKey")) {
        strcpy(id_to, cJSON_GetObjectItem(recv, "id")->valuestring);
        if (cJSON_HasObjectItem(pubKeyData, id_to)) {
            strcpy(ret, "当前用户为好友\n");
        }else{
            strcpy(ret, "好友更新\n");
            cJSON * newkey = cJSON_GetObjectItem(recv, "New_Key");
            cJSON_AddItemToObject(pubKeyData, id_to, newkey);
//            me->friendList[me->friendNum].id = strdup(id_to);
//            me->friendList[me->friendNum].puKey = me->eKey;
//            me->friendNum ++;
            printf("pubkey:%s\n",cJSON_Print(newkey));
        }
    }
//change passwd
    else if(!strcmp(type, "SC_Change_Success") && key->valueint) {
        strcpy(ret, "change password success\n");
    }
//upload file response
    else if(!strcmp(type, "SC_Upload_Response") && key->valueint) {
        sendFile(cJSON_GetObjectItem(recv, "filename")->valuestring, IP);
    }
//Resv a file
    else if(!strcmp(type, "CC_File") && key->valueint) {
        //strcpy(result, cJSON_GetObjectItem(recv, "filename")->valuestring);
        //strcpy(id_to, cJSON_GetObjectItem(recv, "fd")->valuestring);
        ssent(createDownload(inf2.name, cJSON_GetObjectItem(recv, "filename")->valuestring));
        recvFile(find_file_name(cJSON_GetObjectItem(recv, "filename")->valuestring));
    }

    else if(!strcmp(type, "error")) {
        ;
    }
    return ret;
}
void rrecv() {
        while(1) {
        memset(buf, 0, sizeof(buf));
        if((recvbytes = recv(sockfd, buf, MAXDATASIZE,0)) == -1) {//接收客户端的请求
            perror("recv");
            exit(1);
        }
        if (strlen(buf)==0||cJSON_HasObjectItem(cJSON_Parse(buf), "Mes_type")==0) {
            continue;
        }
        printf("%s\n", buf);
        parseRecv(buf, &me);
        break;
    }
}
//线程2
void *thread1() {
    while (1) {
        memset(buf, 0, sizeof(buf));
        if((recvbytes = recv(sockfd, buf, MAXDATASIZE,0)) == -1) {//接收客户端的请求
            perror("recv");
            exit(1);
        }
        if (strlen(buf)==0||cJSON_HasObjectItem(cJSON_Parse(buf), "Mes_type")==0) {
            continue;
        }
        printf("%s\n", buf);
        parseRecv(buf, &me);
    }
}
void threadCreate(void) {
    int temp;
    memset(&thread, 0, sizeof(thread));
    if ((temp = pthread_create(&thread[0], NULL, thread1, NULL)) != 0)
        printf("线程创建失败\n");
    printf("create thread success\n");
}
void button1_clicked_cb(GObject *object, gpointer user_data)
{
    GtkWidget * window2;
    GtkBuilder * builder;
    builder = gtk_builder_new ();

    char * name = gtk_entry_get_text(GTK_ENTRY(loginentry1));
    char * password = gtk_entry_get_text(GTK_ENTRY(loginentry2));
    ssent(createLogin(name, password));
    rrecv();
    //------------------------------------------------------------------------------------------------>
    if(loginsignal==1)
    { 
        me.myPrKey = CreatePrKey(name, password);
        me.myPuKey = CreatePuKey(name, password);
        strcpy(inf2.name,name);
        strcpy(inf2.password,password);
        threadCreate();
        //printf("check:%s\n", createCheck());
        //sleep(3);
        ssent(createCheck());
        gtk_widget_hide(user_data);
        listwindow();
    }else{
        gtk_builder_add_from_file (builder, "./ui/login/login_failed.glade", NULL);//读取button.xml文件
        gtk_builder_connect_signals (builder, NULL);
        window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        window2 = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
        gtk_widget_show(window2);
        gtk_entry_set_text(GTK_ENTRY(loginentry1),"");
        gtk_entry_set_text(GTK_ENTRY(loginentry2),"");
    }
    
    
}
void button2_clicked_cb(GObject *object, gpointer user_data)
{
    GtkWidget * window2;
    GtkBuilder * builder;
    builder = gtk_builder_new ();
    char *entry_text;
    inf1.name = gtk_entry_get_text(GTK_ENTRY(loginentry1));
    inf1.password = gtk_entry_get_text(GTK_ENTRY(loginentry2));
    ssent(createRegister(inf1.name, inf1.password, inf1.photo));
    rrecv();
    //------------------------------------------------------------------------------------------------>
    if(registersignal==1)
    {
        gtk_builder_add_from_file (builder, "./ui/login/register_successful.glade", NULL);//register success
    }else{
        gtk_builder_add_from_file (builder, "./ui/login/register_failed.glade", NULL);
        gtk_entry_set_text(GTK_ENTRY(loginentry1),"");
        gtk_entry_set_text(GTK_ENTRY(loginentry2),"");
    }
    window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    window2 = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
    gtk_builder_connect_signals (builder, NULL);
    
    
    gtk_widget_show(window2);
}
void registerwindow(GObject *object, gpointer user_data){
    if(gtk_revealer_get_child_revealed(GTK_REVEALER(revealer))){
        gtk_revealer_set_reveal_child(GTK_REVEALER(revealer),FALSE);
        gtk_revealer_set_reveal_child(GTK_REVEALER(revealer1),FALSE);
        gtk_entry_set_visibility(GTK_ENTRY(loginentry2),FALSE);
    }else{
        gtk_revealer_set_reveal_child(GTK_REVEALER(revealer),TRUE);
        gtk_revealer_set_reveal_child(GTK_REVEALER(revealer1),TRUE);
        gtk_entry_set_visibility(GTK_ENTRY(loginentry2),TRUE);

    }

        
}


void test(GObject *object, gpointer user_data){
    gtk_image_set_from_pixbuf(GTK_IMAGE(registerphoto),Pic(picselection((int)user_data)));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo),Pic(picselection((int)user_data)));
    inf1.photo=(int)user_data;
    inf2.photo=(int)user_data;
    //gtk_widget_hide(windowselection);     
}

void photoselection(GObject *object, gpointer user_data){
    GtkWidget * windowselection;
    GtkWidget * photo1;
    GtkWidget * photo2;
    GtkWidget * photo3;
    GtkWidget * photo4;
    GtkWidget * photo5;
    GtkWidget * photo6;
    GtkWidget * photo7;
    GtkWidget * photo8;
    GtkWidget * photo9;
    GtkWidget * photo10;
    GtkWidget * photo11;
    GtkWidget * photo12;
    GtkWidget * button1;
    GtkWidget * button2;
    GtkWidget * button3;
    GtkWidget * button4;
    GtkWidget * button5;
    GtkWidget * button6;
    GtkWidget * button7;
    GtkWidget * button8;
    GtkWidget * button9;
    GtkWidget * button10;
    GtkWidget * button11;
    GtkWidget * button12;
    windowselection = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    builder1 = gtk_builder_new ();
    gtk_builder_add_from_file (GTK_BUILDER(builder1), "./ui/login/photoselect.glade", NULL);//读取button.xml文件
    
    windowselection = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "window2"));
    photo1 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo1"));
    photo2 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo2"));
    photo3 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo3"));
    photo4 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo4"));
    photo5 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo5"));
    photo6 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo6"));
    photo7 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo7"));
    photo8 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo8"));
    photo9 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo9"));
    photo10 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo10"));
    photo11 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo11"));
    photo12 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "photo12"));
    button1 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button1"));
    button2 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button2"));
    button3 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button3"));
    button4 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button4"));
    button5 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button5"));
    button6 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button6"));
    button7 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button7"));
    button8 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button8"));
    button9 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button9"));
    button10 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button10"));
    button11 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button11"));
    button12 = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER(builder1), "button12"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo1),Pic("./pic/1.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo2),Pic("./pic/2.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo3),Pic("./pic/3.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo4),Pic("./pic/4.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo5),Pic("./pic/5.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo6),Pic("./pic/6.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo7),Pic("./pic/7.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo8),Pic("./pic/8.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo9),Pic("./pic/9.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo10),Pic("./pic/10.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo11),Pic("./pic/11.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo12),Pic("./pic/12.jpg"));
    gtk_builder_connect_signals (GTK_BUILDER(builder1), NULL);
    gtk_widget_show(windowselection);
    int signal;
    g_signal_connect(G_OBJECT(button1), "clicked",G_CALLBACK(test), (gpointer)(signal=1));
    g_signal_connect(G_OBJECT(button2), "clicked",G_CALLBACK(test), (gpointer)(signal=2));
    g_signal_connect(G_OBJECT(button3), "clicked",G_CALLBACK(test), (gpointer)(signal=3));
    g_signal_connect(G_OBJECT(button4), "clicked",G_CALLBACK(test), (gpointer)(signal=4));
    g_signal_connect(G_OBJECT(button5), "clicked",G_CALLBACK(test), (gpointer)(signal=5));
    g_signal_connect(G_OBJECT(button6), "clicked",G_CALLBACK(test), (gpointer)(signal=6));
    g_signal_connect(G_OBJECT(button7), "clicked",G_CALLBACK(test), (gpointer)(signal=7));
    g_signal_connect(G_OBJECT(button8), "clicked",G_CALLBACK(test), (gpointer)(signal=8));
    g_signal_connect(G_OBJECT(button9), "clicked",G_CALLBACK(test), (gpointer)(signal=9));
    g_signal_connect(G_OBJECT(button10), "clicked",G_CALLBACK(test), (gpointer)(signal=10));
    g_signal_connect(G_OBJECT(button11), "clicked",G_CALLBACK(test), (gpointer)(signal=11));
    g_signal_connect(G_OBJECT(button12), "clicked",G_CALLBACK(test), (gpointer)(signal=12));
}
void setnetstatus(){
    gtk_image_set_from_file(GTK_IMAGE(net),"./situation/ok.png");
}
void loginwindow()
{
    GtkWidget * window1;
    GtkWidget * fixed1;
    GtkWidget * textview1;
    GtkWidget * button1;
    GtkWidget * button2;
    GtkWidget * button4;
    GtkWidget * frame;
    GtkWidget * buttondown;
    
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "./ui/login/login.glade", NULL);//读取button.xml文件
    gtk_builder_connect_signals (builder, NULL);

    window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    window1 = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
    loginentry1 = GTK_WIDGET (gtk_builder_get_object (builder, "entry1"));
    loginentry2 = GTK_WIDGET (gtk_builder_get_object (builder, "entry2"));
    button1 = GTK_WIDGET (gtk_builder_get_object (builder, "button1"));
    button2 = GTK_WIDGET (gtk_builder_get_object (builder, "button3"));
    button4 = GTK_WIDGET (gtk_builder_get_object (builder, "button4"));
    //registerentry2 = GTK_WIDGET (gtk_builder_get_object (builder, "entry4"));
    net = GTK_WIDGET (gtk_builder_get_object (builder, "image6"));
    photo = GTK_WIDGET (gtk_builder_get_object (builder, "image4"));
    registerphoto = GTK_WIDGET (gtk_builder_get_object (builder, "image5"));
    buttondown = GTK_WIDGET (gtk_builder_get_object (builder, "button2"));
    revealer = GTK_WIDGET (gtk_builder_get_object (builder, "revealer1"));
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer),FALSE);
    revealer1 = GTK_WIDGET (gtk_builder_get_object (builder, "revealer2"));
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer1),FALSE);
    inf1.photo = 2;
    inf2.photo = 1;
    //frame = gtk_frame_new("Head portrait");   
    //gtk_container_add(GTK_CONTAINER(frame),showPic("./pic/1.jpg"));
    //frame = gtk_button_new_with_label("fuck");
    //gtk_box_pack_start(GTK_BOX(photo), frame, FALSE, FALSE, 0);
    gtk_image_set_from_pixbuf(GTK_IMAGE(photo),Pic("./pic/1.jpg"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(registerphoto),Pic("./pic/2.jpg"));
    gtk_image_set_from_file(GTK_IMAGE(net),"./situation/kill.png");
    if(conn) setnetstatus();
    gtk_entry_set_visibility(GTK_ENTRY(loginentry2),FALSE);
    gtk_entry_set_invisible_char(GTK_ENTRY(loginentry2),'*');
    gtk_widget_show(window1);
    registersignal=0;
    loginsignal=0;
    
    //g_signal_connect(G_OBJECT(loginentry2), "delete-from-cursor", G_CALLBACK(setphoto),NULL);
    g_signal_connect(G_OBJECT(button1), "clicked", G_CALLBACK(button1_clicked_cb), window1);//login
    g_signal_connect(G_OBJECT(button2), "clicked", G_CALLBACK(button2_clicked_cb), NULL);//register
    g_signal_connect(G_OBJECT(window1), "destroy",G_CALLBACK(closewindow), NULL);
    g_signal_connect(G_OBJECT(buttondown), "clicked",G_CALLBACK(registerwindow), NULL);//toggle
    g_signal_connect(G_OBJECT(button4), "clicked",G_CALLBACK(photoselection), NULL);
}

int main (int argc, char *argv[])
{
    
    windowinf = cJSON_CreateObject();
    cJSON * keyData = cJSON_CreateObject();
    pubKeyData = keyData;
    //chatwindow("huayu");
    if ((host = gethostbyname(IP)) == NULL) {
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
    serv_addr.sin_port = htons(SERVPORT);
    serv_addr.sin_addr = *((struct in_addr *)host -> h_addr);
    bzero(&(serv_addr.sin_zero), 8);
    //connect
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == 1) {
        perror("connect");
        exit(1);
    }
    printf("----------》服务器连接成功《----------\n");
    printf("socketfd: %d\n", sockfd);
    conn = 1;
    gtk_init (&argc, &argv);//gtk初始化
    loginwindow();

    gtk_main();

return 0;
}


