/*a
 * tcp_client.c
 *
 *  Created on: 29 kwi 2026
 *      Author: mmazu
 */

#include "tcp_client.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "string.h"
#include "stdio.h"
#include "stdint.h"
#include "sys/types.h"
#include "sys/socket.h"
#include <netinet/in.h>
#include "wifi.c"

static unsigned int ip_to_uint32(const char ipv4[]){
	static unsigned int uint_address=0;
	static char buffer[32];
	strcpy(buffer,ipv4);
	static uint8_t pom=0;
	static char* j;
	if((j=strtok(buffer,"."))==NULL) return -1;
	for(int i=4;i>0;i--){
		pom=atoi(j);
		uint_address=pom+uint_address*256;
		if((j=strtok(NULL,"."))==NULL&&i>1) return -1;
	}	
	return uint_address;
}

int tcp_client_init(const char ipv4[],uint16_t* port){
	int n,sockfd;
	unsigned int ipv4_uint32=ip_to_uint32(ipv4);
	uint16_t portno;
	socklen_t servlen;
	char ipv4_uint32_string[30];
	char buffer[256];
	struct sockaddr_in server_addr;
	struct hostent* server;
	portno=*port;
	wifiCli_init();
	printf("Host IP Address: %s in uint: %u\n",ipv4,ipv4_uint32);
	sprintf(ipv4_uint32_string,"%u",ipv4_uint32);
	//create socker
	sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sockfd<0){
		printf("Critical connection error: [SOCK_INIT]\n");
		return -1;
	}
	//server structure init
	if((server=gethostbyname(ipv4))==NULL){
		printf("Critical connection error: [SERV_STRUCT_INIT]\n");
		return -1;	
	}
	bzero((char*)&server_addr,sizeof(server_addr));
	
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(portno);
	
	//connection attempt
	if(connect(sockfd,(struct sockaddr*)&server_addr,servlen=sizeof(server_addr))<0){
		printf("Critical connection error: [SERV_CONNECTION]\n");
		return -1;	
	}else{
		printf("Successfully connected to server %s on port %u\n",ipv4,portno);
	}
	while(1){
		printf("Message to RPi: \n");
		bzero(buffer,strlen(buffer));
		fgets(buffer,strlen(buffer)-1,stdin);
		if((n=write(sockfd,buffer,strlen(buffer)))<0) return -1;
		bzero(buffer,strlen(buffer));
		if((n=read(sockfd,buffer,strlen(buffer)))<0) return -1;
		printf("Confirmation from RPi: %s\n",buffer);
	}
	close(sockfd);
	return 1;
}

