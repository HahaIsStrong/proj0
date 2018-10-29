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
	tcp_t tcpbuf;
	int sockfd;
	int ret;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0)
	{
		perror("socket");
		return -1;
	}
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(50000);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	socklen_t len = sizeof(server_addr);

	ret = connect(sockfd,(struct sockaddr *)&server_addr,len);
	if(ret < 0)
	{
		perror("connect");
		return -2;
	}


	while(1)
	{
		tcpbuf.type = RGS_REQUEST;	
		///////////////////////////////////////////////////////
		printf("[register demo]\nyour name  : ");
		fflush(stdout);
		fgets(tcpbuf.name,sizeof(tcpbuf.name),stdin);
		printf("your passwd: ");
		fflush(stdout);
		fgets(tcpbuf.data,sizeof(tcpbuf.data),stdin);
		///////////////////////////////////////////////////////

		send(sockfd,&tcpbuf,sizeof(tcpbuf),0);
		memset(&tcpbuf,0,sizeof(tcpbuf));
		recv(sockfd,&tcpbuf,sizeof(tcpbuf),0);
		if(!strncmp(tcpbuf.data,"success",7))
				printf("注册成功\n----------------\n\n");
		else
				printf("注册失败\n----------------\n\n");

	}
	return 0;
}





















