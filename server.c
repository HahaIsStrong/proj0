#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>      
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sqlite3.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/epoll.h>
#include "./head.h"
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#define MAXNUM 50
int msgid;
int msgid_stm32;
int shmid;
msg_t msgbuf;
shm_t *shmbufp = NULL;
pthread_t tid_user_management;
pthread_t tid_client_cmd_handeler;
pthread_t tid_CheckLimit_FlushStatus;
pthread_t tid_Auto_Generate_CMD_to_STM32;
pthread_t tid_send_cmd_to_STM32;
pthread_t tid_recv_data_from_stm32;
pthread_t tid_init_camera;

pthread_mutex_t lock;

static sqlite3 *dbp = NULL;
static char * errmsg = NULL;
static char sqlcmd[128]={0};
static char **result = NULL;
static int rows ;
static int columns;
float tem;
float hum;
char flag_abnormal_tem=0;
char flag_abnormal_hum=0;
char flag_busy=0;
stm32_cmd_t cmdbufc; //from client
stm32_cmd_t cmdbufa; //from auto gen.

/*********************************
 * 		用户是否是登录状态 : 
 * 		1:已登录  0:未登录 
 *********************************/
static int islogin(char *name)    
{
	char name0[NAMESIZE]={0};
	strncpy(name0,name,sizeof(name0));
	if(name0[strlen(name0)-1]=='\n')
		name0[strlen(name0)-1]='\0';
	
	if(SQLITE_OK != sqlite3_open("./mydb.db",&dbp))
	{
		printf("function:%s line%d :sqlite3_open fail!\n",__func__,__LINE__);
		return 0;
	}
	
	snprintf(sqlcmd,sizeof(sqlcmd),"select * from user where name='%s' and islogin=1;",name0);
	if(SQLITE_OK != sqlite3_get_table(dbp,sqlcmd,&result,&rows,&columns,&errmsg))
	{
		printf("function:%s line%d :%s\n",__func__,__LINE__,errmsg);
		sqlite3_free(errmsg);
		return 0;
	}
	sqlite3_free_table(result);
	sqlite3_close(dbp);
	if(rows > 0)  
		return 1;
	else 
		return 0;
}

/*************** 注册 *****************/
static int do_register(tcp_t * tcpbufp)   // 1 : success 0: fail
{
	
	if(SQLITE_OK != sqlite3_open("./mydb.db",&dbp))
	{
		printf("function:%s line%d :sqlite3_open fail!\n",__func__,__LINE__);
		tcpbufp->type = RGS_FAILURE;
		return 0;
	}
	
	snprintf(sqlcmd,sizeof(sqlcmd),"select * from user where name='%s';",tcpbufp->name);
	if(SQLITE_OK != sqlite3_get_table(dbp,sqlcmd,&result,&rows,&columns,&errmsg))
	{
		printf("function:%s line%d :%s\n",__func__,__LINE__,errmsg);
		sqlite3_free(errmsg);
		tcpbufp->type = RGS_FAILURE;
		return 0;
	}

	if(rows > 0)  // 新用户名与已有用户重名,注册失败
	{
		strcpy(tcpbufp->data,"error");
		printf("!!! fail to register ! user %s is exsist.\n",tcpbufp->name);
		sqlite3_free_table(result);
		tcpbufp->type = RGS_FAILURE;
		return 0;
	}
	else
	{
		snprintf(sqlcmd,sizeof(sqlcmd),"insert into user values ('%s','%s',0);",tcpbufp->name,tcpbufp->data);
		if(SQLITE_OK != sqlite3_exec(dbp,sqlcmd,NULL,NULL,&errmsg))
		{
			printf("function:%s line%d :%s\n",__func__,__LINE__,errmsg);
			sqlite3_free(errmsg);
			tcpbufp->type = RGS_FAILURE;
			return 0;
		}
		

		snprintf(sqlcmd,sizeof(sqlcmd),"update user set islogin=0 where name='%s';",tcpbufp->name);
		if(SQLITE_OK != sqlite3_exec(dbp,sqlcmd,NULL,NULL,&errmsg))
		{
			printf("function:%s line%d :%s\n",__func__,__LINE__,errmsg);
			sqlite3_free(errmsg);
			tcpbufp->type = RGS_FAILURE;
			return 0;
		}
	}
	sqlite3_free_table(result);
	printf("register success! New user is %s.\n",tcpbufp->name);
	tcpbufp->type = RGS_SUCCESS;
	strcpy(tcpbufp->data,"success");												
	return 1;
}

/************** 登录 ****************/
static int do_login(tcp_t * tcpbufp)
{
	//	
	if(SQLITE_OK != sqlite3_open("./mydb.db",&dbp))
	{
		printf("function:%s line%d :sqlite3_open fail!\n",__func__,__LINE__);
		tcpbufp->type = LOG_FAILURE;
		return 0;
	}
	
	snprintf(sqlcmd,sizeof(sqlcmd),"select * from user where name='%s' and passwd='%s';",tcpbufp->name,tcpbufp->data);
	if(SQLITE_OK != sqlite3_get_table(dbp,sqlcmd,&result,&rows,&columns,&errmsg))
	{
		printf("function:%s line%d :%s\n",__func__,__LINE__,errmsg);
		sqlite3_free(errmsg);
		tcpbufp->type = LOG_FAILURE;
		return 0;
	}

	if(rows > 0)  // 登录成功
	{
		printf("login success ! user is %s.\n",tcpbufp->name);
		snprintf(sqlcmd,sizeof(sqlcmd),"update user set islogin=1 where name='%s';",tcpbufp->name);
		if(SQLITE_OK != sqlite3_exec(dbp,sqlcmd,NULL,NULL,&errmsg))
		{
			printf("function:%s line%d :%s\n",__func__,__LINE__,errmsg);
			sqlite3_free(errmsg);
			tcpbufp->type = LOG_FAILURE;
			return 0;
		}
		sqlite3_free_table(result);
		tcpbufp->type = LOG_SUCCESS;
		strcpy(tcpbufp->data,"success");
		return 1;
	}
	else
	{
		strcpy(tcpbufp->data,"error");
		printf("!!! fail to login ! user %s is not exsist or passwd is wrong.\n",tcpbufp->name);
		sqlite3_free_table(result);
		tcpbufp->type = LOG_FAILURE;
		return 0;
	}
}

/***************** 登出  *****************/
static int do_logout(tcp_t * tcpbufp)
{
	if(SQLITE_OK != sqlite3_open("./mydb.db",&dbp))
	{
		printf("function:%s line%d :sqlite3_open fail!\n",__func__,__LINE__);
		tcpbufp->type = OUT_FAILURE;
		return 0;
	}

	snprintf(sqlcmd,sizeof(sqlcmd),"update user set islogin=0 where name='%s';",tcpbufp->name);
	if(SQLITE_OK != sqlite3_exec(dbp,sqlcmd,NULL,NULL,&errmsg))
	{
		printf("function:%s line%d :%s\n",__func__,__LINE__,errmsg);
		sqlite3_free(errmsg);
		tcpbufp->type = OUT_FAILURE;
		return 0;
	}

	printf("logout success! user %s is offline.\n",tcpbufp->name);
	strcpy(tcpbufp->data,"success");
	tcpbufp->type = OUT_SUCCESS;
	return 1;
}

/*************************************
 *  	 线程 : 用户管理
 *
 *  epoll实现的tcp并发服务器
 *
 ************************************/
void * user_management(void * arg)   
{
	// socket
	int listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == listenfd)
	{
		perror("fail to socket");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}
	// bind 
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(50000);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	int opt = 1;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	if(-1 == bind(listenfd,(struct sockaddr*)&server_addr,sizeof(server_addr)))
	{
		perror("fail to bind");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}
	printf("Server : I am %s:%d\n",inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port));

	// listen
	if(-1 == listen(listenfd,10))
	{	
		perror("fail to listen");
		printf("error: function %s line%d\n",__func__,__LINE__);
		return NULL;
	}

	//accept 
	int epfd = epoll_create(MAXNUM);

	struct epoll_event tmpev;
	tmpev.events = EPOLLIN;
	tmpev.data.fd = listenfd;
	if(-1 == epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&tmpev))
	{
		perror("fail to epoll_ctl");
		printf("error: function %s line%d\n",__func__,__LINE__);
		return NULL;
	}

	tmpev.events = EPOLLIN;
	tmpev.data.fd = 0;
	if(-1 == epoll_ctl(epfd,EPOLL_CTL_ADD, 0 ,&tmpev))
	{
		perror("fail to epoll_ctl");
		printf("error: function %s line%d\n",__func__,__LINE__);
		return NULL;
	}

	struct sockaddr_in client_addr;
	socklen_t len=sizeof(client_addr);

	tcp_t tcpbuf;

	struct epoll_event readyevs[MAXNUM];
	int readynum;
	int i;
	char quit_server[8];
	

	while(1)
	{
		readynum = epoll_wait(epfd,readyevs,MAXNUM, -1) ;     // -1 : blocked

		for(i=0;i<readynum;i++)
		{
			if(readyevs[i].data.fd == listenfd )
			{
				int acceptfd = accept(listenfd,(struct sockaddr*)&client_addr,&len);
				tmpev.events = EPOLLIN;
				tmpev.data.fd = acceptfd;
				epoll_ctl(epfd,EPOLL_CTL_ADD,acceptfd,&tmpev);
				printf("accept clent:  %s:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
			}

			else if(readyevs[i].data.fd == 0 )  
			{
				memset(quit_server,0,sizeof(quit_server));
				fgets(quit_server,sizeof(quit_server),stdin);
				if(!strncmp(quit_server,"quit",4))   // 当检测输入了"quit"程序正常退出
				{
					pthread_cancel(tid_recv_data_from_stm32);
					pthread_cancel(tid_send_cmd_to_STM32);
					pthread_cancel(tid_Auto_Generate_CMD_to_STM32);
					pthread_cancel(tid_CheckLimit_FlushStatus);
					pthread_cancel(tid_client_cmd_handeler);
					pthread_cancel(tid_user_management);
					msgctl(msgid,IPC_RMID,NULL);
					msgctl(msgid_stm32,IPC_RMID,NULL);
					shmdt(shmbufp);
					shmctl(shmid,IPC_RMID,NULL);
					pthread_mutex_destroy(&lock);
					pthread_cancel(tid_init_camera);
					exit(0);
				}
				else 
					printf("enter \"quit\" to quit Server.\n");
			}

			else   // 某一个"acceptfd"
			{

				ssize_t ret;
				memset(&tcpbuf,0,sizeof(tcpbuf));
				ret = recv(readyevs[i].data.fd ,&tcpbuf,sizeof(tcpbuf),0 );
				if(ret<0)
				{	
					perror("fail to recv");
					printf("error: function %s line%d\n",__func__,__LINE__);
					return NULL;
				}

				else if(ret == 0)
				{
					printf("client fd=%d is offline!\n",readyevs[i].data.fd);
					epoll_ctl(epfd,EPOLL_CTL_DEL,readyevs[i].data.fd,NULL);
					close(readyevs[i].data.fd);
				}
				else 
				{
					if(tcpbuf.name[strlen(tcpbuf.name)-1]== '\n')
						tcpbuf.name[strlen(tcpbuf.name)-1]='\0';
					if(tcpbuf.data[strlen(tcpbuf.data)-1]== '\n')
						tcpbuf.data[strlen(tcpbuf.data)-1]='\0';
					printf("recv: %c\n",tcpbuf.type);
					if(!tcpbuf.name[0])
					{
						strcpy(tcpbuf.data ,"error");
						printf("!!!The name is empty.\n");
						switch(tcpbuf.type)
						{
						case LOG_REQUEST:
							tcpbuf.type = LOG_FAILURE;break;
						case RGS_REQUEST:
							tcpbuf.type = RGS_FAILURE;break;
						case OUT_REQUEST:
							tcpbuf.type = OUT_FAILURE;break;
						default:
							;
						}
						goto haha;
					}


					switch(tcpbuf.type)
					{

					case RGS_REQUEST:
						do_register(&tcpbuf);
						break;
					case LOG_REQUEST:
						do_login(&tcpbuf);
						break;
					case OUT_REQUEST:
						do_logout(&tcpbuf);
						break;
					default:
						printf("Not defined TCP_CMD\n");
						strcpy(tcpbuf.data ,"Server: I don't know\n");
					}
		haha:			send(readyevs[i].data.fd,&tcpbuf,sizeof(tcpbuf),0);
				}

			}

		}

	}
	pthread_exit(NULL);
}

/*****************************************************
 * 			接受客户端命令线程
 *
 *  使用消息队列接受来自客户端的命令请求
 *
 *****************************************************/
static void * client_cmd_handeler(void * arg)
{	
	int ret;
	while(1)
	{
		ret = msgrcv(msgid,&msgbuf,sizeof(msg_t)-sizeof(long),client_to_server,0);
		if( -1 == ret)
		{
			perror("fail to msgrcv");
			printf("error: function %s line%d\n",__func__,__LINE__);
			return NULL;
		}
		switch(msgbuf.cmd)   
		{
		case LED1_ON: 	
			printf("get cmd [ LED1_ON ] from client\n");break;
		case LED1_OFF: 	
			printf("get cmd [ LED1_OFF ] from client\n");break;
		case LED2_ON: 
			printf("get cmd [ LED2_ON ] from client\n");break;
		case LED2_OFF: 
			printf("get cmd [ LED2_OFF ] from client\n");break;
		case LED3_ON: 
			printf("get cmd [ LED3_ON ] from client\n");break;
		case LED3_OFF: 
			printf("get cmd [ LED3_OFF ] from client\n");break;
		case BEEP_ON: 
			printf("get cmd [ BEEP_ON ] from client\n");break;
		case BEEP_OFF: 
			printf("get cmd [ BEEP_OFF ] from client\n");break;
		case BEEP_ALARM_ON: 
			printf("get cmd [ BEEP_ALARM_ON ] from client\n");break;
		case BEEP_ALARM_OFF: 
			printf("get cmd [ BEEP_ALARM_OFF ] from client\n");break;
		case GET_TEM_HUM: 
			printf("get cmd [ GET_TEM_HUM ] from client\n");break;
		default :
			printf("get [ unknown cmd ] from client , ignored! \n");
			continue;
		}
		// lock
		cmdbufc.type = server_waiting_cmd_to_stm32;
		cmdbufc.cmd = msgbuf.cmd;
		msgsnd(msgid_stm32,&cmdbufc,sizeof(cmdbufc)-sizeof(long),0);
		//unlock 
	}
	pthread_exit(NULL);
}

/*****************************************************
 * 	 		   自动命令生成线程
 *
 *   检查阀值火焰等状态,发现异常就产生报警命令.
 *
*****************************************************/
void * Auto_Generate_CMD_to_STM32(void * arg)
{
	
 	while(1)
	{
		//  超阀值报警
		if( 1 == flag_abnormal_tem || 1 == flag_abnormal_hum || !(V(shmbufp->status,NO_FIRE)))
		{
			//lock
			cmdbufa.type = server_waiting_cmd_to_stm32;
			cmdbufa.cmd = BEEP_ALARM_ON;
			msgsnd(msgid_stm32,&cmdbufa,sizeof(cmdbufa)-sizeof(long),0);

			printf("T : [%s,%s] %s ",shmbufp->lower_tem,shmbufp->higher_tem,shmbufp->tem);
			if( 1 == flag_abnormal_tem)
				printf("Temperature is out of ranger ! BEEP_ALARM_ON !!!\n");
			else 
				printf("\n");

			printf("H : [%s,%s] %s ",shmbufp->lower_hum,shmbufp->higher_hum,shmbufp->hum);
			if( 1 == flag_abnormal_hum)
				printf("Humidity is out of ranger ! BEEP_ALARM_ON !!!\n");
			else 
				printf("\n");

			if(!V(shmbufp->status,NO_FIRE))
				printf("fire ... fire ... fire !!!\n");
			sleep(3);
			//unlock
		}
		
	}
	pthread_exit(NULL);
}

/*****************************************************
 *     		线程 : 检查阀值 刷新状态
 *
 *  根据阀值决定是否为为异常 , 刷新温湿度状态
 * 
*****************************************************/
void * CheckLimit_FlushStatus(void * arg)
{
	float lower_tem;
	float higher_tem;
	float tem;
	float lower_hum;
	float higher_hum;
	float hum;

	/****** init status ******/
	SET0(shmbufp->status,LED1);
	SET0(shmbufp->status,LED2);
	SET0(shmbufp->status,LED3);
	SET0(shmbufp->status,BEEP);
	SET0(shmbufp->status,BEEP_ALARM);

	SET0(shmbufp->status,TEM_HIGH);
	SET1(shmbufp->status,TEM_OK);
	SET0(shmbufp->status,TEM_LOW);

	SET0(shmbufp->status,HUM_HIGH);
	SET1(shmbufp->status,HUM_OK);
	SET0(shmbufp->status,HUM_LOW);

	strcpy(shmbufp->lower_tem,"18.0C");
	strcpy(shmbufp->higher_tem,"30.0C");
	strcpy(shmbufp->lower_hum,"40.0%");
	strcpy(shmbufp->higher_hum,"90.0%");


	while(1)
	{
		pthread_mutex_lock(&lock);
		lower_tem  = atof(shmbufp->lower_tem);
		higher_tem = atof(shmbufp->higher_tem);
		lower_hum  = atof(shmbufp->lower_hum);
		higher_hum = atof(shmbufp->higher_hum);
		if(lower_tem > higher_tem )
		{
			printf("!!! Please check the lower or higher Temperature value\n");//阀值设置有误
			SET0(shmbufp->status,TEM_LOW);
			SET0(shmbufp->status,TEM_OK);
			SET0(shmbufp->status,TEM_HIGH);
			flag_abnormal_tem = 0;
			goto hhh;
		}

		tem = atof(shmbufp->tem);
		if(tem < lower_tem)
		{
				SET1(shmbufp->status,TEM_LOW);
				SET0(shmbufp->status,TEM_OK);
				SET0(shmbufp->status,TEM_HIGH);
				flag_abnormal_tem = 1;
		}
		else if(tem > higher_tem)
		{
				SET0(shmbufp->status,TEM_LOW);
				SET0(shmbufp->status,TEM_OK);
				SET1(shmbufp->status,TEM_HIGH);
				flag_abnormal_tem = 1;
		}
		else {
				SET0(shmbufp->status,TEM_LOW);
				SET1(shmbufp->status,TEM_OK);
				SET0(shmbufp->status,TEM_HIGH);
				flag_abnormal_tem = 0;
		}	

hhh:    // .......................................................
		if(lower_hum > higher_hum)
		{
			printf("!!! Please check the lower or higher Humidity value\n");//阀值设置有误
			flag_abnormal_hum = 0;
			SET0(shmbufp->status,TEM_LOW);
			SET0(shmbufp->status,TEM_OK);
			SET0(shmbufp->status,TEM_HIGH);
			sleep(3);
			continue;
		}

		hum = atof(shmbufp->hum);
		if( hum < lower_hum)
		{
			SET1(shmbufp->status,HUM_LOW);
			SET0(shmbufp->status,HUM_OK);
			SET0(shmbufp->status,HUM_HIGH);
			flag_abnormal_hum = 1;
		}
		else if(hum > higher_hum)
		{
				SET0(shmbufp->status,HUM_LOW);
				SET0(shmbufp->status,HUM_OK);
				SET1(shmbufp->status,HUM_HIGH);
				flag_abnormal_hum = 1;
		}
		else {
				SET0(shmbufp->status,HUM_LOW);
				SET1(shmbufp->status,HUM_OK);
				SET0(shmbufp->status,HUM_HIGH);
				flag_abnormal_hum = 0;
		}	
		pthread_mutex_unlock(&lock);
	}
	pthread_exit(NULL);
}

/****************** 设置串口  ******************/
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop ,int a)  
{
	struct termios newtio,oldtio;
	if  ( tcgetattr( fd,&oldtio)  !=  0) 
	{ 
		perror("!!! fail to init serial");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag  |=  CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	switch( nBits ) 	/*** 数据位 ***/
	{
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch( nEvent ) 	/*** 校验 ***/
	{
	case 'O':   // 奇校验
		newtio.c_cflag |= PARENB;  
		newtio.c_cflag |= PARODD;  
		newtio.c_iflag |= (INPCK | ISTRIP); 
		break;
	case 'E':   // 偶校验
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;  
		break;
	case 'N':  // 不校验
		newtio.c_cflag &= ~PARENB; 
		break;
	}

	switch( nSpeed ) 	/*** 波特率 ***/
	{
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	case 460800:
		cfsetispeed(&newtio, B460800);
		cfsetospeed(&newtio, B460800);
		break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}
	if( nStop == 1 ) 	/*** 停止位位数 ***/
		newtio.c_cflag &=  ~CSTOPB;// 1
	else if ( nStop == 2 )
		newtio.c_cflag |=  CSTOPB; // 2

	newtio.c_cc[VTIME]  = 0;
	newtio.c_cc[VMIN] = a;
	tcflush(fd,TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("!!! fail to init serial");
		printf("error: function %s line%d\n",__func__,__LINE__);
		return -1;
	}
	printf("init serial success!\n");
	return 0;
}

/*****************************************************
 *    		线程 : 从STM32接受数据
 *
 * 	    从STM32获取温湿度 火焰 灯光强度等等信息 
 *    刷新当前的温湿度值,刷新灯光强度值 , 刷新火焰状态
 *
*****************************************************/
void *  recv_data_from_stm32(void * arg)
{

	int fdr;
	if((fdr = open(SERIAL_DEVICE, O_RDWR|O_NOCTTY|O_NDELAY))<0)
	{
		perror("Can't find serial device");
		printf("error: function %s line%d\n",__func__,__LINE__);
		return NULL;
	}	
	set_opt(fdr, BaudRate , 8, 'N', 1 , 0); 

	int ret;
	char data_from_stm32[16];
	while(1)
	{
		memset(data_from_stm32,0,sizeof(data_from_stm32));
		do
		{
			ret = read(fdr,data_from_stm32,sizeof(data_from_stm32));
		}while(ret <=0);

		printf("recv_ret=%d ",ret);
		data_from_stm32[ret]='\0';
		printf("recv:%s\n",data_from_stm32);
		if(ret != 13)
			continue;
		pthread_mutex_lock(&lock);
		// lock
		memset(shmbufp->hum,0,sizeof(shmbufp->hum));
		shmbufp->hum[0]= data_from_stm32[0];
		shmbufp->hum[1]= data_from_stm32[1];
		shmbufp->hum[2]= '.';
		shmbufp->hum[3]= data_from_stm32[2];
		shmbufp->hum[4]= '%';

		memset(shmbufp->tem,0,sizeof(shmbufp->tem));
		shmbufp->tem[0]= data_from_stm32[3];
		shmbufp->tem[1]= data_from_stm32[4];
		shmbufp->tem[2]= '.';
		shmbufp->tem[3]= data_from_stm32[5];
		shmbufp->tem[4]= 'C';

		if(data_from_stm32[6] == '1')
			SET1(shmbufp->status,NO_FIRE);
		else 
			SET0(shmbufp->status,NO_FIRE);
				
		memset(shmbufp->light_v,0,sizeof(shmbufp->light_v));
		shmbufp->light_v[0]= data_from_stm32[7];
		shmbufp->light_v[1]= data_from_stm32[8];
		shmbufp->light_v[2]= data_from_stm32[9];
		shmbufp->light_v[3]= data_from_stm32[10];
		shmbufp->light_v[4]= 'v';

		// unlock
		pthread_mutex_unlock(&lock);
	}

	return ;
}

/*****************************************************
 *    		线程 : 发送指令给STM32
 *
 * 	         读取指令并发送给STM32
 *
*****************************************************/
void * send_cmd_to_STM32(void * arg)
{
	stm32_cmd_t cmdbuf0;

	int fdw;
	if((fdw = open(SERIAL_DEVICE, O_RDWR|O_NOCTTY|O_NDELAY))<0)
	{
		perror("Can't find serial device");
		printf("error: function %s line%d\n",__func__,__LINE__);
		return NULL;
	}	
	set_opt(fdw, BaudRate , 8, 'N', 1 , 0); 
	
 	char cmd0[2]={0,0};
	int ret;

	while(1)
	{
		// 读取消息队列中的命令
		msgrcv(msgid_stm32,&cmdbuf0,sizeof(cmdbuf0)-sizeof(long),server_waiting_cmd_to_stm32,0);
		// 发送给STM , 等待STM32执行
		cmd0[0]=cmdbuf0.cmd;
		do 
		{
			ret = write(fdw,cmd0,1); 
		}while(ret <=0 );
		printf("send cmd: %d ---> STM32\n",cmdbuf0.cmd);
		// 刷新LED1 LED2  LED3 BEEP 的状态
		switch(cmdbuf0.cmd)
		{
		case LED1_ON:
			SET1(shmbufp->status,LED1);break;
		case LED1_OFF:
			SET0(shmbufp->status,LED1);break;

		case LED2_ON:
			SET1(shmbufp->status,LED2);break;
		case LED2_OFF:
			SET0(shmbufp->status,LED2);break;

		case LED3_ON:
			SET1(shmbufp->status,LED3);break;
		case LED3_OFF:
			SET0(shmbufp->status,LED3);break;

		case BEEP_ON:
			SET1(shmbufp->status,BEEP);break;
		case BEEP_OFF:
			SET0(shmbufp->status,BEEP);break;

		case GET_TEM_HUM: break;

		default:     ;
		}
		sleep(1);
	}
	pthread_exit(NULL);
}


/*****************************************************
 *         线程 : 开启摄像头视频采集服务
*****************************************************/
void * init_camera(void * arg) 
{
	system("cd mjpg_streamer/ & ./start.sh");

	while(1)
	{

	}
	pthread_exit(NULL);
}


int main(int argc, const char *argv[])
{	


	system("/etc/boa/boa");
	//****************** server <------ msg <-------- client
	key_t keym = ftok(MYFILE,'m');
	if(-1 == keym)
	{
		perror("fail to fork");
		printf("error: function %s line%d\n",__func__,__LINE__);
		return -1;
	}
	msgid = msgget(keym,IPC_CREAT | IPC_EXCL | 0666);
	if(-1 == msgid)
	{
		if(EEXIST == errno)
			msgid = msgget(keym,0666);
		else 
		{
			perror("fail to msgget");
			printf("error: function %s line%d\n",__func__,__LINE__);
			return -1;
		}	
	}

 	//***************** server cmd pointing to stm32 
	key_t key_stm32 = ftok(MYFILE,'M');
	msgid_stm32 = msgget(key_stm32,IPC_CREAT | IPC_EXCL | 0666);
	if(-1 == msgid_stm32)
	{
		if(EEXIST == errno)
			msgid_stm32 = msgget(key_stm32,0666);
		else 
		{
			perror("fail to msgget");
			return -1;
		}	
	}

	// ********************** server ----> shm ----> client
	key_t keys=ftok(MYFILE, 's');
	if( -1 == keys )
	{
		perror("fail to ftok");
		exit(1);
	}
	shmid = shmget(keys,sizeof(shm_t),IPC_CREAT | IPC_EXCL | 0666);
	if( -1 == shmid)		// 申请共享RAM空间
	{
		if( EEXIST == errno)
			shmid = shmget(keys,sizeof(shm_t),0666);
		else
		{
			perror("fail to shmget");
			return -1;
		}
	}	
	shmbufp = shmat(shmid,NULL,0);
	

	int ret;

	ret = pthread_mutex_init(&lock,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for pthread_mutex_init\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}

	ret = pthread_create(&tid_user_management,NULL,user_management,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for user_management\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}

	ret = pthread_create(&tid_client_cmd_handeler,NULL,client_cmd_handeler,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for client_cmd_handeler\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}


	ret = pthread_create(&tid_CheckLimit_FlushStatus,NULL,CheckLimit_FlushStatus,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for CheckLimit_FlushStatus\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}

	ret = pthread_create(&tid_Auto_Generate_CMD_to_STM32,NULL,Auto_Generate_CMD_to_STM32,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for Auto_Generate_CMD_to_STM32\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}

	ret = pthread_create(&tid_send_cmd_to_STM32,NULL,send_cmd_to_STM32,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for send_cmd_to_STM32\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}

	ret = pthread_create(&tid_recv_data_from_stm32,NULL,recv_data_from_stm32,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for recv_data_from_stm32\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}

	ret = pthread_create(&tid_init_camera,NULL,init_camera,NULL);
	if( 0 != ret )
	{
		printf("fail to pthread_create for init_camera\n");
		printf("error: function %s line%d\n",__func__,__LINE__);
		exit(1);
	}


	pthread_join(tid_user_management,NULL); // 等待线程结束
	pthread_join(tid_client_cmd_handeler,NULL); 
	pthread_join(tid_CheckLimit_FlushStatus,NULL); 
	pthread_join(tid_Auto_Generate_CMD_to_STM32,NULL); 
	pthread_join(tid_send_cmd_to_STM32,NULL);
	pthread_join(tid_recv_data_from_stm32,NULL);
	pthread_join(tid_init_camera,NULL); 
	pthread_mutex_destroy(&lock);

	return 0;

}
