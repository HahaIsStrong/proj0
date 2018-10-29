#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/epoll.h>
#include <time.h>
#include "./head.h"





int main(int argc, const char *argv[])
{

	int msgid;
	key_t keym = ftok(MYFILE,'m');
	msgid = msgget(keym,IPC_CREAT | IPC_EXCL | 0666);
	if(-1 == msgid)
	{
		if(EEXIST == errno)
			msgid = msgget(keym,0666);
		else 
		{
			perror("fail to msgget");
			return -1;
		}	
	}
	
	msg_t msgbuf;

	int cmd ;
	char ch;
	while(1)
	{
		msgbuf.type = client_to_server;
		printf("\n-----------------------------\nClient cmd to Server:\n");
		printf(" %d: LED1_ON       %d: LED1_OFF \n",LED1_ON,LED1_OFF);
		printf(" %d: LED2_ON       %d: LED2_OFF \n",LED2_ON,LED2_OFF);
		printf(" %d: LED3_ON       %d: LED3_OFF \n",LED3_ON,LED3_OFF);
		printf(" %d: BEEP_ON       %d: BEEP_OFF \n",BEEP_ON,BEEP_OFF);
		printf(" %d: BEEP_ALARM_ON %d: BEEP_ALARM_OFF\n",BEEP_ALARM_ON,BEEP_ALARM_OFF);
		printf(" %d: GET_TEM_HUM \n",GET_TEM_HUM);
		scanf("%d",&cmd);
		do{ch=getchar();}while(ch!='\n');
		
		msgbuf.cmd=cmd;

		int ret;
		ret = msgsnd(msgid,&msgbuf,sizeof(msgbuf)-sizeof(long),0);
		if( -1 == ret)
		{
			perror("fail to msgsnd\n");
			return -1;
		}
	}
	return 0;
}





















