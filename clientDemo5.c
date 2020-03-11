/*
filename server_ipaddress portno
argv[0] filename
argv[1] server-ipaddress 
argv[2] port no
*/


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>



void error(const char *errmsg)//function for showing error msg
{
	perror(errmsg);
        exit(1);
}


int main(int argc,char *argv[])
{
	int sockfd ,portNo,input;
	
	struct sockaddr_in server_addr;
	
	struct hostent *server;
	
	char buffer[256];
	
	char appendText[5000];
	bzero(appendText,5000);
	
	//argument for providing port number
	
	if(argc<3)
	{
		fprintf(stderr,"usage %s hostname port\n",argv[0]);
	exit(0);
	}
	
	//providing port number
	
	portNo=atoi(argv[2]);
	
	//creating socket(store value from socket system call)
	
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	
	if(sockfd<0)
	{
		error("Error opening socket!\n");
	}
	
	//providing host's port
	
	server = gethostbyname(argv[1]);
	
	if(server==NULL)
	{
		fprintf(stderr,"Error!no existing host\n");
	}
	

	bzero((char *)&server_addr,sizeof(server_addr));
	
	server_addr.sin_family=AF_INET;
	
	//copies n bytes from s1 area to s2(as pointer)
	
	bcopy((char *) server->h_addr,(char *) &server_addr.sin_addr.s_addr,server->h_length);
	
	server_addr.sin_port=htons(portNo);
	
	//connection request
	
	if(connect(sockfd,(struct sockaddr *)& server_addr,sizeof(server_addr))<0)
	{
		error("Connection failed!\n");
	}

	//reading and writing
	
	while(1)
	{
		bzero(buffer,256);
		
		fgets(buffer,256,stdin);
		
		strcat(appendText,buffer);
		
		input=write(sockfd,appendText,strlen(appendText));
		
		if(input<0)
		{
			error("error on writing!\n");
		}
		
		
		
		bzero(buffer,256);
		
		input=read(sockfd,buffer,256);
		if(input<0)
		{
			error("Error on reading!\n");
		}
		strcat(appendText,buffer);
		printf("Server:%s\n",appendText);
		
		int i=strncmp("Bye",buffer,3);
		
		if(i==0)
		{
			break;
		}
		
	}
	
	close(sockfd);
	
	return 0;
}


