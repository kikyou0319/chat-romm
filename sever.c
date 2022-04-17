
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sqlite3.h>
#include<signal.h>
#define MINSIZE 20
#define MAXSIZE 1024
#define SERVPORT 5000

typedef struct online_client
{
	int connfd;
	char uid[MINSIZE];
	int chat;
	struct online_client *next;
} Online;

struct thread_node
{
	int connfd;
	char uid[MINSIZE];
	Online *head;
};

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
	int flag1;
	int flag3;
	char filename[MINSIZE];
	char filetext[MAXSIZE];
} Client;

int create_sql(sqlite3 *pbd) //创建表
{
	char *sql;
	char *errmsg = NULL;
	int ret;

	sql = "create table if not exists mytable (uid text primary key,username text, password text);";
	ret = sqlite3_exec(pbd, sql, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK)
	{
		printf("error!%s\n", errmsg);
		return -1;
	}
	else
	{
		return SQLITE_OK;
	}
}
int reg(sqlite3 *pdb, Client *client)
{
	char sql[150];
	char *errmsg = NULL;
	sprintf(sql, "insert into mytable (uid,username,password) values('%s','%s','%s');", client->uid, client->username, client->password);
	int ret = sqlite3_exec(pdb, sql, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK)
	{
		printf("注册插入出错！\n");
	}
	return SQLITE_OK;
}
void is_malloc_ok(Online *new_node)
{
	if (new_node == NULL)
	{
		printf("malloc error!\n");
		exit(0);
	}
}

void create_new_node(Online **new_node)
{
	(*new_node) = (Online *)malloc(sizeof(Online));
	is_malloc_ok(*new_node);
}

void create_link(Online **head)
{
	create_new_node(head);
	(*head)->next = NULL;
}

void insert_node_head(Online *head, Online *new_node)
{
	new_node->next = head->next;
	head->next = new_node;
}

void release_link(Online **head)
{
	Online *temp;
	temp = *head;

	while (*head != NULL)
	{
		*head = temp->next;
		free(temp);
		temp = *head;
	}
}
void diaplay(Online *head)
{
	Online *p;
	p = head->next;
	while (p != NULL)
	{
		printf("id = %s--------connfd = %d\n", p->uid, p->connfd);
		p = p->next;
	}
}
void all_chat(struct thread_node connfd_node, Client client)
{
	Online *p = NULL;
	p = connfd_node.head->next;
	while (p != NULL)
	{
		printf("%d\n", p->connfd);
		send(p->connfd, &client, sizeof(Client), 0);
		sleep(0.5);
		p = p->next;
	}
}
void ban_one(struct thread_node connfd_node, Client client)
{
	Online *p = NULL;
	p = connfd_node.head->next;
	while (p != NULL && strcmp(p->uid, client.banid) != 0)
	{
		p = p->next;
	}
	if (p != NULL)
	{
		p->chat = 1;
		send(p->connfd, &client, sizeof(Client), 0);
	}
	else
	{
		printf("client is office!\n");
	}
}
void ban_all(struct thread_node connfd_node, Client client)
{
	Online *p = NULL;
	p = connfd_node.head->next;
	while (p != NULL)
	{
		if (strcmp(p->uid,client.uid) == 0)
		{
			p = p->next;
			continue;
		}
		p->chat = 1;
		send(p->connfd, &client, sizeof(Client), 0);
		p = p->next;
	}
}
void unlock_one(struct thread_node connfd_node, Client client)
{
	Online *p = NULL;
	p = connfd_node.head->next;
	while (p != NULL && strcmp(p->uid,client.banid) != 0)
	{
		p = p->next;
	}
	if (p != NULL)
	{
		p->chat = 0;
		send(p->connfd,&client,sizeof(Client),0);
		return;
	}
	else
	{
		printf("client is off!\n");
	}
}
void unlock_all(struct thread_node connfd_node, Client client)
{
	Online *p = NULL;
	p = connfd_node.head->next;
	while (p != NULL)
	{
		if (strcmp(p->uid,client.uid)==0)//字符型比较！找了半天注意！
		{
			p = p->next;
			continue;
		}
		else
		{
			p->chat = 0;
			send(p->connfd, &client, sizeof(Client), 0);
			p = p->next;
		}
	}
}
void remove_person(struct thread_node connfd_node, Client client)
{
	Online *p = NULL;
	p = connfd_node.head->next;
	if (p == NULL)
	{
		printf("已无在线！\n");
	}
	else
	{
		while (p->next != NULL && strcmp(p->uid, client.toid) != 0)
		{
			p = p->next;
		}
		if (p != NULL)
		{
			client.flag3 = 1;
			send(p->connfd, &client, sizeof(Client), 0);
		}
		else
		{
			if (strcmp(p->uid, client.toid) == 0)
			{
				client.flag3 = 1;
				send(p->connfd, &client, sizeof(Client), 0);
			}
			else
			{
				sprintf(client.msg, "未找到该用户%s", client.toid);
				send(p->connfd, &client, sizeof(Client), 0);
			}
		}
	}
}
void delete_node(struct thread_node connfd_node)
{
	Online *p = NULL;
	Online *q = NULL;
	q = connfd_node.head;
	p = q->next;
	if (p == NULL)
	{
		printf("已无用户在线！\n");
	}
	else
	{
		while (p != NULL && p->connfd != connfd_node.connfd)
		{
			q = p;
			p = p->next;
		}
		if (p->next != NULL)
		{
			q->next = p->next;
			free(p);
		}
		else
		{
			if (p->connfd == connfd_node.connfd)
			{
				q->next = NULL;
				free(p);
			}
			else
			{
				printf("为找到！\n");
			}
		}
	}
	pthread_exit(NULL);
}
void msg_send_recv(struct thread_node connfd_node)
{

	Client client;
	int n;
	int ret;
	sqlite3 *pdb;
	ret = sqlite3_open("mydatabase.db", &pdb);
	if (ret != SQLITE_OK)
	{
		printf("打开数据库失败!%s\n", sqlite3_errmsg(pdb));
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("打开数据库成功！\n");
	}
	create_sql(pdb);
	while (1)
	{
		// printf("\n");
		sleep(1);
		n = recv(connfd_node.connfd, &client, sizeof(Client), 0);
		if (n == 0)
		{
			printf("用户%s下线\n", client.uid);
			pthread_exit(NULL);
		}
		switch (client.choice)
		{
		case 1:
		{
			char sql[150];
			char *errmsg = NULL;
			sprintf(sql, "insert into mytable (uid,username,password) values('%s','%s','%s');", client.uid, client.username, client.password);
			int ret = sqlite3_exec(pdb, sql, NULL, NULL, &errmsg);
			if (ret != SQLITE_OK)
			{
				printf("注册出错！\n");
			}
			send(connfd_node.connfd, &client, sizeof(Client), 0);
		}
		break;
		case 2:
		{
			Online *new_node;
			char *errmsg = NULL;
			char **resultp = NULL;
			int nrow, ncolumn;
			char *sql = "select * from mytable";
			int ret = sqlite3_get_table(pdb, sql, &resultp, &nrow, &ncolumn, &errmsg);
			if (ret != SQLITE_OK)
			{
				printf("数据库操作失败：%s\n", errmsg);
				break;
			}
			int i;
			for (i = 0; i < (nrow + 1) * ncolumn; i++)
			{
				if (strcmp(resultp[i], client.uid) == 0 && strcmp(resultp[i + 2], client.password) == 0)
				{
					printf("%s\n登录成功\n", client.uid);
				}
			}
			create_new_node(&new_node);
			strcpy(new_node->uid, client.uid);
			new_node->connfd = connfd_node.connfd;	 //套接字赋值
			new_node->next = connfd_node.head->next; //插入
			connfd_node.head->next = new_node;
			client.cmd = 3;
			send(connfd_node.connfd, &client, sizeof(Client), 0);
		}
		break;
		case 3:
		{
			diaplay(connfd_node.head);
			Online *p = NULL;
			p = connfd_node.head->next;
			while (p != NULL)
			{
				sprintf(client.online, "用户id:%s", p->uid);
				send(connfd_node.connfd, &client, sizeof(Client), 0);
				p = p->next;
			}
		}
		break;
		case 4:
		{
			Online *p = NULL;
			p = connfd_node.head->next;
			while (p != NULL && strcmp(p->uid, client.uid) != 0)
			{
				p = p->next;
			}
			if (p->chat == 1)
			{
				client.flag1 = 1;
				send(connfd_node.connfd, &client, sizeof(Client), 0);
				break;
			}
			Online *q = NULL;
			q = connfd_node.head->next;
			while (q != NULL && strcmp(q->uid, client.toid) != 0)
			{
				q = q->next;
			}
			if (q != NULL)
			{

				send(q->connfd, &client, sizeof(Client), 0);
			}
			else
			{
				printf("client is office!\n");
			}
		}
		break;
		case 5:
		{
			Online *p = NULL;
			p = connfd_node.head->next;
			while (p != NULL && strcmp(p->uid, client.uid) != 0)
			{
				p = p->next;
			}
			if (p->chat == 1)
			{
				client.flag1 = 1;
				send(connfd_node.connfd, &client, sizeof(Client), 0);
				break;
			}
			all_chat(connfd_node, client);
		}
		break;
		case 6:
			printf("聊天记录发送至客户端!\n");
			break;
		case 7:
			ban_one(connfd_node, client);
			break;
		case 8:
			ban_all(connfd_node, client);
			break;
		case 9:
			unlock_one(connfd_node, client);
			break;
		case 10:
			unlock_all(connfd_node, client);
			break;
		case 11:
			remove_person(connfd_node, client);
			system("clear");
			break;
		case 12:
		{
			Online *p = NULL;
			p = connfd_node.head->next;
			while (p != NULL && strcmp(p->uid, client.toid))
			{
				p = p->next;
			}
			if (p != NULL)
			{
				strcpy(client.msg, "您收到一个文件！");
				send(p->connfd, &client, sizeof(Client), 0);
			}
			else
			{
				printf("client is off!\n");
			}
			break;
		}
		break;
		case 13:
		{
			delete_node(connfd_node);
			close(connfd_node.connfd);
			pthread_exit(NULL);
		}
		break;
		default:
			break;
		}
	}

	close(connfd_node.connfd);
	return;
}

void *client_chart(void *arg)
{
	struct thread_node connfd_node;
	socklen_t clilen;
	struct sockaddr_in cliaddr;

	clilen = sizeof(cliaddr);

	connfd_node = *((struct thread_node *)arg);

	msg_send_recv(connfd_node);

	close(connfd_node.connfd);

	return NULL;
}

// // 定长线程池实现
// struct job
// {
//     void *(*func)(void *arg);
//     void *arg;
//     struct job *next;
// };

// struct threadpool
// {
//     int thread_num;  //已开启线程池已工作线程
//     pthread_t *pthread_ids;  // 薄脆线程池中线程id


//     struct job *head;
//     struct job *tail;  // 任务队列的尾
//     int queue_max_num;  //任务队列的最多放多少个
//     int queue_cur_num;  //任务队列已有多少个任务

//     pthread_mutex_t mutex;
//     pthread_cond_t queue_empty;    //任务队列为空
//     pthread_cond_t queue_not_emtpy;  //任务队列不为空
//     pthread_cond_t queue_not_full;  //任务队列不为满

//     int pool_close;
// };

// void * threadpool_function(void *arg)
// {
//     struct threadpool *pool = (struct threadpool *)arg;
//     struct job *pjob = NULL;

//     while (1)
//     {
//         pthread_mutex_lock(&(pool->mutex));

//         while(pool->queue_cur_num == 0)
//         {
//             pthread_cond_wait(&(pool->queue_not_emtpy), &(pool->mutex));

//             if (pool->pool_close == 1)
//             {
//                 pthread_exit(NULL);
//             }
//         }

//         pjob = pool->head;
//         pool->queue_cur_num--;

//         if (pool->queue_cur_num != pool->queue_max_num)
//         {
//             pthread_cond_broadcast(&(pool->queue_not_full));
//         }
        
//         if (pool->queue_cur_num == 0)
//         {
//             pool->head = pool->tail = NULL;
//             pthread_cond_broadcast(&(pool->queue_empty));
//         }
//         else
//         {
//             pool->head = pjob->next;
//         }
        
//         pthread_mutex_unlock(&(pool->mutex));

//         (*(pjob->func))(pjob->arg);
//         free(pjob);
//         pjob = NULL;
//     }
// }

// struct threadpool * threadpool_init(int thread_num, int queue_max_num)
// {
//     struct threadpool *pool = (struct threadpool *)malloc(sizeof(struct threadpool));
//     // malloc

//     pool->queue_max_num = queue_max_num;
//     pool->queue_cur_num = 0;
//     pool->pool_close = 0;
//     pool->head = NULL;
//     pool->tail = NULL;

//     pthread_mutex_init(&(pool->mutex), NULL);
//     pthread_cond_init(&(pool->queue_empty), NULL);
//     pthread_cond_init(&(pool->queue_not_emtpy), NULL);
//     pthread_cond_init(&(pool->queue_not_full), NULL);

//     pool->thread_num = thread_num;
//     pool->pthread_ids = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
//     // malloc

//     for (int i = 0; i < pool->thread_num; i++)
//     {
//         pthread_create(&pool->pthread_ids[i], NULL, (void *)threadpool_function, (void *)pool);
//     }

//     return pool;
// }

// void threadpool_add_job(struct threadpool *pool, void *(*func)(void *), void *arg)
// {
//     pthread_mutex_lock(&(pool->mutex));
//     while (pool->queue_cur_num == pool->queue_max_num)
//     {
//         pthread_cond_wait(&pool->queue_not_full, &(pool->mutex));
//     }
    
    
//     struct job *pjob = (struct job *)malloc(sizeof(struct job));
//     //malloc
    
//     pjob->func = func;
//     pjob->arg = arg;
//     pjob->next = NULL;
    
//     // pjob->func(pjob->arg);
//     if (pool->head == NULL)
//     {
//         pool->head = pool->tail = pjob;
//         pthread_cond_broadcast(&(pool->queue_not_emtpy));
//     }
//     else
//     {
//         pool->tail ->next = pjob;
//         pool->tail = pjob;
//     }

//     pool->queue_cur_num++;
//     pthread_mutex_unlock(&(pool->mutex));
// }

// void thread_destroy(struct threadpool *pool)
// {
//     pthread_mutex_lock(&(pool->mutex));

//     while (pool->queue_cur_num != 0)
//     {
//          pthread_cond_wait(&(pool->queue_empty),&(pool->mutex));
//     }

//     pthread_mutex_unlock(&(pool->mutex));

//     pthread_cond_broadcast(&(pool->queue_not_full));

//     pool->pool_close = 1;

//     for (int i = 0; i < pool->thread_num; i++)
//     {
//         pthread_cond_broadcast(&(pool->queue_not_emtpy));
//         // pthread_cancel(pool->pthread_ids[i]); //有系统调用，才能销毁掉；有bug
//         printf("thread exit!\n");
//         pthread_join(pool->pthread_ids[i], NULL);
//     }

//     pthread_mutex_destroy(&(pool->mutex));
//     pthread_cond_destroy(&(pool->queue_empty));
//     pthread_cond_destroy(&(pool->queue_not_emtpy));
//     pthread_cond_destroy(&(pool->queue_not_full));

//     free(pool->pthread_ids);

//     struct job *temp;
//     while(pool->head != NULL)
//     {
//         temp = pool->head;
//         pool->head = temp->next;
//         free(temp);
//     }

//     free(pool);

//     printf("destroy finish!\n");
// }

int main()
{
	sqlite3 *pdb;
	int listenfd, connfd, n;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t clilen;
	char msg[1000];
	pid_t pid;
	int ret;
	pthread_t tid;
	Online *head;
	Online *new_node;

	struct thread_node connfd_node;

	create_link(&head);

	connfd_node.head = head;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERVPORT);

	int opt = 1;
	//setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	listen(listenfd, 100);

    //线程池初始化
    //struct threadpool *pool = threadpool_init(10, 100);

	while (1)
	{
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

		printf("connfd = %d\n", connfd);

		connfd_node.connfd = connfd;

		ret = pthread_create(&tid, NULL, (void *)client_chart, (void *)&connfd_node);
        //threadpool_add_job(pool, (void *)client_chart, (void *)&connfd_node);
	}

	pthread_join(tid, NULL);
	close(listenfd);
	release_link(&head);
	//thread_destroy(pool);

	return 0;
}
