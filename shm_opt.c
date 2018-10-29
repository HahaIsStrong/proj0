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
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/epoll.h>
#include "./head.h"

int main(int argc, const char *argv[])
{


	key_t keys=ftok(MYFILE, 's');
	if( -1 == keys )
	{
		perror("fail to ftok");
		exit(1);
	}
	int shmid = shmget(keys,sizeof(shm_t),IPC_CREAT | IPC_EXCL | 0666);
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

	shm_t *shmbufp;
	shmbufp = shmat(shmid,NULL,0);


	
	while(1)
	{
		printf("------------------------------------------------\nSelect opt type:\n");
		printf("1 . write shm      2 . read shm\n" );
		int i;
		scanf("%d",&i);
		getchar();
		switch(i)
		{
			case 1:
				// 设置数据
				printf("lower_tem=");fflush(stdout);
				fgets(shmbufp->lower_tem,sizeof(shmbufp->lower_tem),stdin);
				printf("higher_tem=");fflush(stdout);
				fgets(shmbufp->higher_tem,sizeof(shmbufp->higher_tem),stdin);
				printf("lower_hum=");fflush(stdout);
				fgets(shmbufp->lower_hum,sizeof(shmbufp->lower_hum),stdin);
				printf("higher_hum=");fflush(stdout);
				fgets(shmbufp->higher_hum,sizeof(shmbufp->higher_hum),stdin);
				printf("tem=");fflush(stdout);
				fgets(shmbufp->tem,sizeof(shmbufp->tem),stdin);
				printf("hum=");fflush(stdout);
				fgets(shmbufp->hum,sizeof(shmbufp->hum),stdin);
				break;
				
			case 2:  
				// 显示状态
				printf("------------------------------------------------\n1 .Status:\n");
				printf("LED1=%d\n",V(shmbufp->status,LED1));
				printf("LED2=%d\n",V(shmbufp->status,LED2));
				printf("LED3=%d\n",V(shmbufp->status,LED3));
				printf("BEEP=%d\n",V(shmbufp->status,BEEP));
				printf("BEEP_ALARM=%d\n",V(shmbufp->status,BEEP_ALARM));

				printf("TEM_HIGH=%d\n",V(shmbufp->status,TEM_HIGH));
				printf("  TEM_OK=%d\n",V(shmbufp->status,TEM_OK));
				printf(" TEM_LOW=%d\n",V(shmbufp->status,TEM_LOW));
				printf("HUM_HIGH=%d\n",V(shmbufp->status,HUM_HIGH));
				printf("  HUM_OK=%d\n",V(shmbufp->status,HUM_OK));
				printf(" HUM_LOW=%d\n",V(shmbufp->status,HUM_LOW));
				printf(" NO_FIRE=%d\n",V(shmbufp->status,NO_FIRE));
			    printf("\n2 .数据:\n");	
				// 显示数据
				printf(" lower_tem=%s\n",shmbufp->lower_tem);
				printf("       tem=%s\n",shmbufp->tem);
				printf("higher_tem=%s\n",shmbufp->higher_tem);
				printf(" lower_hum=%s\n",shmbufp->lower_hum);
				printf("       hum=%s\n",shmbufp->hum);
				printf("higher_hum=%s\n",shmbufp->higher_hum);
				printf("   light_v=%s\n",shmbufp->light_v);
				// 设置数据
#if 0	
				printf("请设定当前值:\n");
				printf("tem=");fflush(stdout);
				fgets(shmbufp->tem,sizeof(shmbufp->tem),stdin);
				printf("hum=");fflush(stdout);
				fgets(shmbufp->hum,sizeof(shmbufp->hum),stdin);
#endif 
				break;
			default :
				;


		}
	}
	return 0;
}
