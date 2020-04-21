	
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


#define ClientsNumber 10
#define Buffer_Size 2000
#define Name_Len 32

static _Atomic unsigned int client_count=0;
static int user_id=10;

//creating client structure

typedef struct{

	struct sockaddr_in address;
	int sockfd;
	int user_id;
	char name[32];
	
	}client_t;

	
client_t *client[ClientsNumber];

pthread_mutex_t client_mutex=PTHREAD_MUTEX_INITIALIZER;


//clearing message

void str_overwrite_stdout(){
	printf("\r%s","> ");
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

//Adding Client	
	
void client_add(client_t *clientObj){

	pthread_mutex_lock(&client_mutex);
	
	for(int i=0;i<ClientsNumber;i++)
	{
		if(!client[i])
		{
			client[i]=clientObj;
			break;
		}
	}
	
	
	pthread_mutex_unlock(&client_mutex);
}

//Removing client

void client_remove(int user_id ){

	pthread_mutex_lock(&client_mutex);
	
	for(int i=0;i<ClientsNumber;i++)
	{
		if(client[i])
		{
			client[i]->user_id=user_id;
			client[i]=NULL;
			break;
		}
	}
	pthread_mutex_unlock(&client_mutex);	
}


//Sending message

void send_message(char *chat ,int user_id){
	
	pthread_mutex_lock(&client_mutex);
	
	for(int i=0;i<ClientsNumber;i++)
	{
		if(client[i])
		{
			if(client[i]->user_id!=user_id)
			{
				if(write(client[i]->sockfd,chat,strlen(chat))<0)
				{
					printf("ERROR:error in writing\n");
					
					break;
				}
			}
		}	
	}
	
	pthread_mutex_unlock(&client_mutex);
}


//Handling Clients

void handle_client(void *arg){

	char buffer[Buffer_Size];
	char name[32];
	int flag=0;
	client_count++;
	
	
	client_t *cli=(client_t*)arg;
	
	//Checking Client Name
	int chk=recv(cli->sockfd,name,Name_Len,0);
		//printf("%d is the chk\n",cli->sockfd);
	
	if(/*recv(cli->sockfd,name,Name_Len,0)*/chk<=0||strlen(name)<2||strlen(name)>=Name_Len-1){
	
	printf("Enter the name correctly\n");
	
	//printf("%ld\n",strlen(name));
	
	//printf("%s",name);
	
		flag=1;
	}
	
	else
	{
		strcpy(cli->name,name);
		
		sprintf(buffer,"%s has joined\n",cli->name);
		
		printf("%s",buffer);
		
		send_message(buffer,cli->user_id);
		
		//added from serverDemo5 for server to send msg
		
		fgets(buffer,256,stdin);
                 
		 printf("msg: %s",buffer);
	}
	
	bzero(buffer,Buffer_Size);
	
	while(1){
	
	if(flag){
	
	break;
	
	}
	
	int receive=recv(cli->sockfd,buffer,Buffer_Size,0);
	
	if(receive>0){
		if(strlen(buffer)>0){
			send_message(buffer,cli->user_id);
			
			str_trim_lf(buffer,strlen(buffer));
			
			printf("%s ->%s\n",buffer,cli->name);
			
			}
		}
	else if(receive==0||strcmp(buffer,"bye")==0){
	
		sprintf(buffer,"%s has left the board\n",cli->name);
		
		printf("%s " ,buffer);
		
		send_message(buffer,cli->user_id);
		
		flag=1;
		
	}
	else
	{
		printf("ERROR: error in program\n");
		flag=1;
	}
	bzero(buffer,Buffer_Size);
	
	} 
	
	close(cli->sockfd);
	
	client_remove(cli->user_id);
	
	free(cli);
	
	client_count--;
	
	pthread_detach(pthread_self());
	
	//return NULL;
				
}
	
//Main Method

int main(int argc,char **argv){
	
	if(argc!=2)
	{
		printf("%s port is in use\n",argv[0]);
		return EXIT_FAILURE;
	}
	
	
	int port=atoi(argv[1]);
	int option =1;
	int listenfd,connectionfd;
	
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	
	pthread_t newthread;
	
	//socket creating
	
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(port);
	
	//signal
	
	signal(SIGPIPE,SIG_IGN);
	
	if(setsockopt(listenfd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),/*(char*) */&option,sizeof(option))<0){
		printf("ERROR: in setting setsockopt\n");
		return EXIT_FAILURE;
	}
	
	//Bind
	
	if(bind(listenfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
		printf("ERROR: error in binding\n");
		return EXIT_FAILURE;
	}
		
	
	//listen
	
	if(listen(listenfd,10)<0){
		printf("ERROR: error in listening\n");
		return EXIT_FAILURE;
	}
	
	printf("*****WHITEBOARD*****\n\n");
	int client_len=sizeof(client_addr);
	
	while(1)
	{
	
		connectionfd=accept(listenfd ,(struct sockaddr*)&client_addr,&client_len);
		
	      if(connectionfd<0){
	      perror("accept"); 
        	exit(EXIT_FAILURE); 
	      }
	//Check for client capacity
		
		
	if((client_count+1)==ClientsNumber)
	{
		printf("Maximum clients connected.connecton denied\n");
	
	close(connectionfd);
	
	continue;
	
	}
	
	
	//Client setting
	
	client_t *cli=(client_t*) malloc (sizeof(client_t));
	
	cli->address=client_addr;
	
	cli->sockfd=connectionfd;
	
	cli->user_id=user_id++;
	
		
	//Add client to Queue
	
	client_add(cli);
	
	pthread_create (&newthread,NULL,&handle_client,(void*)cli);
	
	//reduce CPU uses
	
	sleep(1);
	
	}
	
		
	return EXIT_SUCCESS;
}


