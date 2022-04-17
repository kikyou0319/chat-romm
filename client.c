
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <termios.h>
#include <sqlite3.h>

#define MINSIZE 20
#define MAXSIZE 1024
#define SERVPORT 5000
char online_user[10][20];
sqlite3 *pdb;
typedef struct msg_node
{
    char chat_mode;
    int connfd;
    int flag;
    int cmd;
    char msg[100];
    int choice;
    char username[MINSIZE];
    char password[MINSIZE];
    char uid[MINSIZE];
    char toid[MINSIZE];
    char online[MAXSIZE];
    char time[100];
    char banid[100];
    int flag1; //禁言，解除禁言
    int flag3; //关闭线程，踢人
    char filename[MINSIZE];
    char filetext[MAXSIZE];
} Client;
int flag2; //判断是否进入聊天室
void managerface()
{
    printf("|*****************************|\n");
    printf("|**************主菜单**********|\n");
    printf("|*****************************|\n");
    printf("|********3.显示在线好友*********\n");
    printf("|************4.私聊************\n");
    printf("|*************5.群聊***********\n");
    printf("|********6.查看私聊记录**********\n");
    printf("|********7.禁止某人发言**********\n");
    printf("|*******8.禁止所有人发言*********\n");
    printf("|***********9.解禁某人**********\n");
    printf("|**********10.解禁所有人********\n");
    printf("|**********11.踢人*************\n");
    printf("|***********12.传文件**********\n");
    printf("|***********13.退出************\n");
    printf("|********14.查看群聊记录*********\n");
    printf("|*****************************\n");
}
void userface()
{

    printf("|*****************************\n");
    printf("|***********主菜单*************\n");
    printf("|*****************************\n");
    printf("|      3.显示在线好友           \n");
    printf("|      4.私聊                  \n");
    printf("|      5.群聊                  \n");
    printf("|      6.查看私聊记录            \n");
    printf("|      6.查看群聊记录            \n");
    printf("|      12.传文件                \n");
    printf("|      13.退出                 \n");
    printf("|*****************************\n");
    printf("请输入您要进行的操作:\n");
}
int getch(void) //读取键盘输入无回显
{
    int ch;
    struct termios tm, tm_old;
    tcgetattr(STDIN_FILENO, &tm);
    tm_old = tm;
    tm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tm);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &tm_old);
    return ch;
}
void insert_records(sqlite3 *pdb, Client client)
{
    char sql[MAXSIZE];
    char *errmsg = NULL;
    int ret;
    sprintf(sql, "insert into records (uid,toname,msg,time) values ('%s','%s','%s','%s');", client.uid, client.toid, client.msg, client.time);
    ret = sqlite3_exec(pdb, sql, NULL, NULL, &errmsg);
    if (ret != SQLITE_OK)
    {
        printf("插入聊天记录失败！%s\n", errmsg);
        return;
    }
}
void insert_records_all(sqlite3 *pdb,Client client)
{
    char sql[MAXSIZE];
    char *errmsg = NULL;
    int ret;
    sprintf(sql,"insert into recordsall (uid,toname,msg,time) values ('%s','all','%s','%s');",client.uid,client.msg,client.time);
    ret = sqlite3_exec(pdb,sql,NULL,NULL,&errmsg);
    if (ret != SQLITE_OK)
    {
        printf("insert into table error!%s\n",errmsg);
        return;
    }
}
void get_records_all(sqlite3 *pdb, Client client)
{
    char sql[MAXSIZE];
    char *errmsg = NULL;
    char **resultp = NULL;
    int nrow, ncolumn;
    int ret;
    sprintf(sql, "select * from recordsall where uid = '%s' ;", client.uid);
    ret = sqlite3_get_table(pdb, sql, &resultp, &nrow, &ncolumn, &errmsg);
    if (ret != SQLITE_OK)
    {
        printf("查找失败！%s\n", errmsg);
        return;
    }
    for (int i = 0; i < (nrow + 1) * ncolumn; i++)
    {
        if (i % ncolumn == 0)
        {
            printf("\n");
        }
        printf("%s  ", resultp[i]);
    }
    printf("\n");
}
void get_records(sqlite3 *pdb, Client client)
{
    char sql[MAXSIZE];
    char *errmsg = NULL;
    char **resultp = NULL;
    int nrow, ncolumn;
    int ret;
    sprintf(sql, "select * from records where uid = '%s' or toname = '%s';", client.uid, client.uid);
    ret = sqlite3_get_table(pdb, sql, &resultp, &nrow, &ncolumn, &errmsg);
    if (ret != SQLITE_OK)
    {
        printf("查找失败！%s\n", errmsg);
        return;
    }
    for (int i = 0; i < (nrow + 1) * ncolumn; i++)
    {
        if (i % ncolumn == 0)
        {
            printf("\n");
        }
        printf("%s   ", resultp[i]);
    }
    printf("\n");
}
void sendfile(int sockfd, Client client)
{
    FILE *fp;
    char ch;
    int i;
    printf("请您输入您想发给谁！\n");
    scanf("%s", client.toid);
    printf("输入您想传输的文件！\n");
    scanf("%s", client.filename);
    if ((fp = fopen(client.filename, "r")) == NULL)
    {
        perror("error!\n");
        return;
    }
    ch = fgetc(fp);
    while (ch != EOF)
    {
        client.filetext[i] = ch;
        ch = fgetc(fp);
        i++; 
    }
    client.filetext[i] = '\0';
    send(sockfd,&client,sizeof(Client),0);
    fclose(fp);
}
void recv_file(Client client)
{
    FILE *fp ;
    int i = 0;
    if ((fp=fopen("test2.txt","a+")) == NULL)
    {
        perror("error!\n");
        return;
    }
    while (client.filetext[i] != '\0')
    {
        fputc(client.filetext[i],fp);
        i++;
    }
    fclose(fp);
}
void *read_thread(void *var)
{
    int n;
    int q = 0;
    char recvline[1000];
    int sockfd;
    Client client;
    sockfd = *((int *)var);
    while (1)
    {
        n = recv(sockfd, &client, sizeof(Client), 0);
        switch (client.choice)
        {
        case 1:
        {
            printf("您已经注册成功，您的UID是%s\n", client.uid);
            printf("您的昵称是%s\n", client.username);
            printf("您的密码是%s\n", client.password);
        }
        break;
        case 2:
        {
            flag2 = client.cmd;
        }
        break;
        case 3:
        {
            strcpy(online_user[q], client.online);
            printf("%s\n", online_user[q]);
            q++;
        }
        break;
        case 4:

            if (client.flag1 == 1)
            {
                printf("您已经被禁言了！\n");
            }
            else
            {
                printf("recv:%s\n", client.msg);
                insert_records(pdb, client);
            }
            break;
        case 5:
        {
            printf("recv:%s\n", client.msg);
            
        }
        break;
        case 6:
        {
            printf("------\n");
        }
        break;
        case 7:
        {
            printf("禁言成功\n");
        }
        break;
        case 8:
        {
            printf("集体禁言！\n");
        }
        break;
        case 9:
        {
            printf("解禁某一个\n");
        }
        break;
        case 10:
        {
            printf("解禁所有人！\n");
        }
        break;
        case 11:
        {
            if (client.flag3 == 1)
            {
                printf("您被强制下线！\n");
                client.choice = 13;
                send(sockfd, &client, sizeof(Client), 0);
                pthread_exit(NULL);
            }
        }
        break;
        case 12:
        {
            printf("%s\n",client.msg);
            recv_file(client);
        }
        break;
        case 13:
        {
            printf("退出");

            return NULL;
        }
        break;
        default:
            break;
        }
    }
    pthread_exit(NULL);
}
void chatroom(int sockfd, Client client)
{
    if ((strcmp(client.uid, "10000")) == 0)
    {

        while (1)
        {
            managerface();
            char *buf1, *buf2;
            time_t Time1, Time2;
            scanf("%d", &client.choice);
            switch (client.choice)
            {
                //查看在线
            case 3:
                send(sockfd, &client, sizeof(Client), 0);
                break;
                //私聊
            case 4:
                Time1 = time(NULL);
                buf1 = ctime(&Time1);
                *(buf1 + 24) = '\0';
                strcpy(client.time, buf1);
                printf("请您输入您想聊天的内容！\n");
                scanf("%s", client.msg);
                printf("请输入您想聊天的对象的UID！\n");
                scanf("%s", client.toid);
                send(sockfd, &client, sizeof(Client), 0);
                break;
            case 5:
                Time2 = time(NULL);
                buf2 = ctime(&Time2);
                *(buf2 + 24) = '\0';
                strcpy(client.time, buf2);
                printf("请输入您想输入的内容\n");
                scanf("%s", client.msg);
                insert_records_all(pdb, client);
                send(sockfd, &client, sizeof(Client), 0);
                break;
            case 6:
                printf("私聊聊记录为\n");
                get_records(pdb, client);
                break;
            //禁某一个用户发言
            case 7:
                printf("输入您想禁言的用户\n");
                scanf("%s", client.banid);
                send(sockfd, &client, sizeof(Client), 0);
                break;
            //禁止所有人发言
            case 8:
                send(sockfd, &client, sizeof(Client), 0);
                break;
            //解禁某人
            case 9:
                printf("输入您想解禁的账号！\n");
                scanf("%s", client.banid);
                send(sockfd, &client, sizeof(Client), 0);
                break;
            //解禁所有人
            case 10:
                send(sockfd, &client, sizeof(Client), 0);
                break;
            //踢人
            case 11:
                printf("请输入您想踢出的人的账号！\n");
                scanf("%s", client.toid);
                send(sockfd, &client, sizeof(Client), 0);
                system("clear");
                break;
            //传文件
            case 12:
                sendfile(sockfd,client);
                break;
            case 13:
                send(sockfd, &client, sizeof(Client), 0);
                system("clear");
                break;
            case 14:
                printf("群聊记录为！\n");
                get_records_all(pdb,client);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        while (1)
        {
            userface();
            char *buf1, *buf2;
            time_t Time1, Time2;
            scanf("%d", &client.choice);
            switch (client.choice)
            {
            case 3:
                send(sockfd, &client, sizeof(Client), 0);
                break;
            case 4:
                Time1 = time(NULL);
                buf1 = ctime(&Time1);
                *(buf1 + 24) = '\0';
                strcpy(client.time, buf1);
                printf("请您输入您想聊天的内容！\n");
                scanf("%s", client.msg);
                printf("请输入您想聊天的对象的UID！\n");
                scanf("%s", client.toid);
                send(sockfd, &client, sizeof(Client), 0);
                break;
            case 5:
                Time2 = time(NULL);
                buf2 = ctime(&Time2);
                *(buf2 + 24) = '\0';
                strcpy(client.time, buf2);
                printf("请输入您想输入的内容\n");
                scanf("%s", client.msg);
                send(sockfd, &client, sizeof(Client), 0);
                break;
            case 6:
            {
                printf("聊天记录为\n");
                printf("如果输出的内容里面没有toname则代表该聊天记录是群聊记录\n");
                get_records(pdb, client);
            }
            break;
            case 12:
            {
                sendfile(sockfd,client);
            }break;
            case 13:
            {
                send(sockfd, &client, sizeof(Client), 0);
                system("clear");
                return;
            }
            break;
            case 14:
            {
                printf("群聊记录！\n");
                get_records_all(pdb,client);
            }break;
            default:
                break;
            }
        }
    }
    pthread_exit(NULL);
}
void *write_thread(void *var)
{
    char sendline[1000];
    Client client;
    int sockfd;
    sockfd = *((int *)var);
    while (1)
    {

        time_t Time = time(NULL);
        char *buf = ctime(&Time);
        system("clear");
        printf("%s", buf);
        sleep(1);
        printf("*********1.注册*********\n");
        printf("*********2.登录*********\n");
        scanf("%d", &client.choice);
        getchar();
        switch (client.choice)
        {
        case 1:
            printf("请您输入您的密码！\n");
            char password1[MINSIZE], password2[MINSIZE], username[MINSIZE], uid[MINSIZE];
            char password[MINSIZE], password3[MINSIZE];
            int i = 0;
            while (1)
            {
                password[i] = getch();
                if (password[i] == '\n')
                {
                    break;
                }
                else
                {
                    password1[i] = password[i];
                    i++;
                    putchar('*');
                }
            }
            password1[i] = '\0';
            printf("\n");
            i = 0;
            printf("请您确认您的密码！\n");
            while (1)
            {
                password3[i] = getch();
                if (password3[i] == '\n')
                {
                    break;
                }
                else
                {
                    password2[i] = password3[i];
                    i++;
                    putchar('*');
                }
            }
            password2[i] = '\0';
            if ((strcmp(password1, password2)) != 0)
            {
                printf("两次密码输入不一致,请重新输入！\n");
                continue;
            }
            printf("\n");
            printf("输入您的昵称:\n");
            scanf("%s", username);
            getchar();
            srand((unsigned)time(NULL));
            sprintf(uid, "%d", rand());
            strcpy(client.uid, uid);
            strcpy(client.username, username);
            strcpy(client.password, password1);
            send(sockfd, &client, sizeof(Client), 0);
            break;
        case 2:
            printf("请输入您的UID！\n");
            scanf("%s", uid);
            strcpy(client.uid, uid);
            printf("请输入您的密码!\n");
            scanf("%s", password1);
            strcpy(client.password, password1);
            send(sockfd, &client, sizeof(Client), 0);
            printf("登录成功!\n");
            sleep(1);
            if (flag2 == 3)
            {
                chatroom(sockfd, client);
                break;
            }
        case 13:
            return NULL;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{

    int sockfd;
    int ret;
    char *sql;
    char *sql1;
    struct sockaddr_in servaddr, cliaddr;
    pthread_t tid_read;
    pthread_t tid_write;
    if (argc != 2)
    {
        printf("需要输入IP地址！>\n");
        exit(1);
    }

    ret = sqlite3_open("mydatabase.db", &pdb); //打开数据库
    if (ret != SQLITE_OK)
    {
        printf("数据库打开失败！\n");
        return -1;
    }
    //创建聊天记录表
    char *errmsg;
    sql = "create table if not exists records(turn integer primary key,uid text,toname text,msg text,time text);";

    ret = sqlite3_exec(pdb, sql, NULL, NULL, &errmsg);
    if (ret != SQLITE_OK)
    {
        printf("聊天记录表创建失败%s\n", errmsg);
        return -1;
    }
    sql1 = "create table if not exists recordsall(turn integer primary key,uid text,toname text,msg text,time text);";
    ret = sqlite3_exec(pdb,sql1,NULL,NULL,&errmsg);
    if (ret != SQLITE_OK)
    {
        printf("创建群聊记录表失败！\n");
        return -1;
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(SERVPORT);

    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    ret = pthread_create(&tid_write, NULL, (void *)write_thread, (void *)(&sockfd));

    ret = pthread_create(&tid_read, NULL, (void *)read_thread, (void *)(&sockfd));

    pthread_join(tid_write, NULL);
    pthread_join(tid_read, NULL);

    close(sockfd);
    sqlite3_close(pdb);
    return 0;
}
