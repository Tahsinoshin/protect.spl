

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include<sys/types.h>
#include<signal.h>
#include<arpa/inet.h>
#include<sys/stat.h>



#define ClientsNumber 100
#define Buffer_Size 2000
#define Name_Len 32


volatile sig_atomic_t flag=0;
int sockfd=0;
char name[Name_Len];






//clearing message

void str_overwrite_stdout(){
	printf("\r%s",">");
	fflush(stdout);
	}

//completing chat

void str_trim_lf(char* array,int length){

	for(int i=0;i<length;i++)
	{
		if(array[i]=='\n')
		{
			array[i]='\0';
			break;
		}
	}

}

//creating condition to be true or false

void catch_message_return(){

	flag=1;
}


//handle receiving messages

void receiving_message_handler(){

	char message[Buffer_Size]={};
	while(1){
		//int receive=recv(sockfd,message,Buffer_Size,0);
		int receive = read(sockfd,message,Buffer_Size);
		printf("%s\n",message);
		if(receive>0){

		//

		str_overwrite_stdout();

		}

		else if(receive<=0){

			break;
		}

		bzero(message,Buffer_Size );
	}
}


//Message Sending function

void sending_message_handler(){

	char buffer[Buffer_Size]={};
	//int sockfd;

	char message[Buffer_Size+Name_Len]={};

	while(1){

		str_overwrite_stdout();

		fgets(buffer,Buffer_Size,stdin);

		str_trim_lf(buffer,Buffer_Size);

		if(strcmp(buffer,"bye")==0){

		break;
		}

		/*else if(strcmp(buffer,"file")==0)
		{
			recvFile(sockfd);
		}*/
		else
		{
			sprintf(message,"%s:%s\n",name,buffer);
			//send(sockfd,message,strlen(message),0);
			write(sockfd,message,strlen(message));
	}

	bzero(buffer ,Buffer_Size);
	bzero(message,Buffer_Size+Name_Len);

	}

	catch_message_return(2);

}


	//Main method

int main(int argc,char **argv){

	if(argc!=3)
	{
		printf("%s port is in use\n",argv[0]);
		return EXIT_FAILURE;
	}

	int port=atoi(argv[2]);


	signal(SIGINT,catch_message_return);

	printf("Enter the client's name: ");

	fgets(name,Name_Len,stdin);

	str_trim_lf(name,strlen(name));

	if(strlen(name)>32-1||strlen(name)<2){

		printf("Enter name correctly\n");

		return EXIT_FAILURE;
	}


	struct sockaddr_in server_addr;


	//socket creating

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(port);

	printf("before connecting\n");

	//Connect to the server

	int value=connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));

	printf("%d values",value);
	if(value<0)
	{
		printf("ERROR in connecting\n");

		return EXIT_FAILURE;
	}


	printf("%s",name);

	//Send Client Name

	send(sockfd,name,Name_Len,0);

	printf("***********WHITEBOARD**********\n\n");

	pthread_t message_sending_thread;

	//while(1){
	if(pthread_create(&message_sending_thread,NULL,(void*)sending_message_handler,NULL)!=0){

	printf("ERROR:in creating pthread\n");

	return EXIT_FAILURE;
	}

	printf("after connecting\n");


	pthread_t message_receiving_thread;

	if(pthread_create(&message_receiving_thread,NULL,(void*)receiving_message_handler,NULL)!=0){

	printf("ERROR:in creating pthread\n");

	return EXIT_FAILURE;
	}


	while(1){
		if(flag){

			printf("\nExit\n");

			break;

			}

			//sending_message_handler();
			//sreceiving_message_handler();
		}


	close(sockfd);

	return EXIT_SUCCESS;
}
