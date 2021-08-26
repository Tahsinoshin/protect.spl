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


#define ClientsNumber 100
#define Buffer_Size 2000
#define Name_Len 3200


void str_trim_file_name(char* array, int length);
char *strdup(const char *s);
//void client_add(client_t *clientObj);
void client_remove(int user_id );
void sendfile(FILE *fp, int user_id, const char *file_name);
void send_message(char *chat,int user_id);
void send_message_all(char *all);
void send_message_self(const char *s,int sockfd);
void send_message_client(char *s, int user_id);
void send_active_client(int sockfd);
void strip_newline(char *s);
void print_client_addr(struct sockaddr_in addr);
void *handle_client( void *arg);


static _Atomic unsigned int client_count=0;
static int user_id=1;
char name[Name_Len];
char file_name[100];
ssize_t total=0;
int listenfd,connectionfd;

//Trim file name from msg
void str_trim_file_name(char* array, int length) {
    bzero(file_name, 100);

    for(int i=5; i<length; i++) {
        if(array[i]=='\n') {
            file_name[i-5]='\0';
            break;
        } else {
            file_name[i-5] = array[i];
        }
    }
}







//creating client structure

typedef struct {

    struct sockaddr_in address;    //address lekha
    int sockfd;
    int user_id;
    char name[32];

} client_t;


client_t *client[ClientsNumber];

pthread_mutex_t client_mutex=PTHREAD_MUTEX_INITIALIZER;

static char topic[Buffer_Size];

pthread_mutex_t topic_mutex =PTHREAD_MUTEX_INITIALIZER ;


//clearing message strdup function is not available in c

char *strdup(const char *s) {

    size_t size =strlen(s)+1;
    char *p =malloc (size);
    if(p) {
        memcpy( p,s,size);
    }

    return p;
}



//completing chat  //eta best one e nai

/*void str_trim_lf(char* array,int length){
	for(int i=0;i<length;i++)
	{
		if(array[i]=='\n')
		{
			array[i]='\0';
			break;
		}
	}
}*/

//Adding Client	to the queue

void client_add(client_t *clientObj) {

    pthread_mutex_lock(&client_mutex);

    for(int i=0; i<ClientsNumber; i++) {
        if(!client[i]) {
            client[i]=clientObj;
            break;
        }
    }


    pthread_mutex_unlock(&client_mutex);
}

//Removing client

void client_remove(int user_id ) {

    pthread_mutex_lock(&client_mutex);

    for(int i=0; i<ClientsNumber; i++) {
        if(client[i]) {
            client[i]->user_id=user_id;
            client[i]=NULL;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

//Send File

void sendfile(FILE *fp, int user_id, const char *file_name) {
    int n;
    char sendpart[Buffer_Size] = {0};
    total =0;

    pthread_mutex_lock(&client_mutex);

    ////

    for(int i=0; i<ClientsNumber; i++) {
        if(client[i]) {
            if(client[i]->user_id!=user_id) {
//                if(write(client[i]->sockfd, file_name, strlen(file_name))<0) {
//                    printf("ERROR:error in writing\n");
//                    break;
//                }
                //bzero(sendpart, Buffer_Size);
                while ((n = fread(sendpart, sizeof(char), Buffer_Size, fp)) > 0) {
                    total+=n;
                    
                    if (n != Buffer_Size && ferror(fp)) {
                        perror("No file Exist or File name error");
                        exit(1);
                    }
                    if(!sendpart)break;
                    printf("File Content: %s\n", sendpart);
                    if(write(client[i]->sockfd, sendpart, n) < 0) {
                        printf("ERROR:error in writing\n");
                        break;
                    }
                    if(n<Buffer_Size)break;
                    bzero(sendpart, Buffer_Size);
                }
            }
        }
    }
    ////

    pthread_mutex_unlock(&client_mutex);
}


//Sending message to all clients but the sender

void send_message(char *chat,int user_id) {

    pthread_mutex_lock(&client_mutex);


    for(int i=0; i<ClientsNumber; i++) {
        if(client[i]) {
            if(client[i]->user_id!=user_id) {
                if(write(client[i]->sockfd,chat,strlen(chat))<0) {
                    printf("ERROR:error in writing\n");

                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&client_mutex);
}



//send message to all clients

void send_message_all(char *all) {

    pthread_mutex_lock(&client_mutex);

    for(int i=0; i<ClientsNumber; i++) {

        if(client[i]) {

            if( write(client[i]->sockfd,all,strlen(all))<0) {

                break ;
            }
        }
    }

    pthread_mutex_unlock(& client_mutex);

}

//send message to sender
void send_message_self(const char *s,int sockfd) {

    if(write(sockfd,s,strlen(s))<0) {

        printf("Error:error in writing to descriptor\n");

        exit(-1);

    }
}


//send message to client

void send_message_client(char *s, int user_id) {

    pthread_mutex_lock(& client_mutex);

    for(int i=0; i< ClientsNumber; i++) {

        if(client[i]) {

            if(client[i]->user_id==user_id) {

                if ( write ( client[i]->sockfd,s,strlen(s)<0))

                {
                    printf("Error:error in writing to descriptor\n");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&client_mutex);


}



//send list of the active clients

void send_active_client(int sockfd) {

    char s[64];

    pthread_mutex_lock(& client_mutex);

    for(int i=0 ; i< ClientsNumber ; i++) {
        if(client[i]) {

            sprintf( s, "<< [%d] %s\r\n",client[i]->user_id, client[i]->name);

            send_message_self ( s, sockfd);
        }
    }

    pthread_mutex_unlock(& client_mutex);
}


//strip crlf

void strip_newline(char *s) {
    while(*s !='\0') {
        if( *s == '\r' || *s == '\n') {
            *s ='\0';
        }
        s++;
    }
}


//print ip address

void print_client_addr(struct sockaddr_in addr) {

    printf("\rip:");

    printf("                             %d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
    //printf("\r");

}
//Send File



//Handle all communication with the client

void *handle_client( void *arg) {

    char buff_out[2*Buffer_Size];
    char buff_in[Buffer_Size];

    int rlen;
    client_count++;

    client_t *cli=(client_t*)arg;


    printf("<< accept ");
    print_client_addr(cli->address); //printf("loop running: %s\n", cli->sockfd);
    printf(" user id: %d\r", cli->user_id);

    read(cli->sockfd,buff_in,sizeof(buff_in)-1);

    sprintf( buff_out,"%s << has joined\r\n", buff_in);

    send_message_all(buff_out);


    pthread_mutex_lock( & topic_mutex);

    if( strlen(topic)) {

        buff_out[0] ='\0';

        sprintf(buff_out, "<< topic: %s\r\n", topic) ;

        send_message_self(buff_out,cli->sockfd) ;
    }

    pthread_mutex_unlock( & topic_mutex) ;

    send_message_self("<< see/help for assistance\r\n", cli->sockfd);

    //Receive input from client

    /*rlen = read ( cli->sockfd , buff_in , sizeof(buff_in)-1);*/

    while(( rlen =read(cli->sockfd,buff_in,sizeof(buff_in)-1) ) > 0) {

        buff_in[rlen] = '\0';

        buff_out[0] = '\0' ;
        printf("%s\n",buff_in);

        strip_newline( buff_in);
        //Ignore empty buffer

        if(!strlen(buff_in)) {

            continue;
        }
        //ajk ei porjnto 12.06.20


        //experimental area
        int start_index_of_messege;
        for(start_index_of_messege = 0; buff_in[start_index_of_messege] != ':'; start_index_of_messege++);
        start_index_of_messege+=1;

        int j = 0;
        char client_messege[Buffer_Size];
        for(int i = start_index_of_messege; buff_in[i] != '\0'; i++, j++) {
            client_messege[j] = buff_in[i];
        }
        client_messege[j] = '\0';

        //Special options

        if(buff_in[start_index_of_messege]=='/') {


            char *command,*param;

            command = strtok( client_messege, " ");  //modified version
            //command = strtok( buff_in , " ");

            if(strlen(command))
                printf("%s\n",command);

            if(!strcmp( command, "/quit")) {

                break;
            }

            else if(!strcmp(command, "/ping")) {

                send_message_self ( "<< ping\r\n", cli->sockfd);
            }

            else if (! strcmp( command, "/topic")) {

                param = strtok( NULL,"\0");

                if(param) {

                    pthread_mutex_lock (& topic_mutex);
                    topic[0] = '\0';

                    while(param!=NULL) {

                        strcat(topic, param);

                        strcat ( topic, " ");
                    }
                    pthread_mutex_unlock(& topic_mutex);

                    sprintf(buff_out, "<<topic changed to: %s\r\n", topic);

                    send_message_all(buff_out);
                }

                else
                    send_message_self ("<<message can not be null\r\n",cli->sockfd);
            }

            else if(!strcmp(command, "/nick")) {
                param = strtok ( NULL," ");
                if(param) {

                    char *old_name = strdup (cli->name);

                    /*mathay rakhte hobe*/
                    if(!old_name) {
                        printf("cant allocate memory\n");
                        continue;
                    }
                    strcpy(cli->name,param);

                    sprintf(buff_out, "<<%s is now known as %s\r\n", old_name, cli->name);

                    free( old_name);
                    send_message_all( buff_out);
                } else {
                    send_message_self( "<< name cannot be null\r\n", cli->sockfd);
                }
            } else if( !strcmp (command,"/msg")) {

                param= strtok( NULL, " ");

                if(param) {

                    int uid =atoi(param);

                    param = strtok( NULL, " ");

                    if(param) {

                        sprintf(buff_out, "[PM][%s]", cli->name);

                        while(param!=NULL) {

                            strcat(buff_out, " ");
                            strcat(buff_out, param);
                            param =strtok ( NULL," ");

                        }

                        strcat( buff_out, "\r\n");

                        send_message_client( buff_out, uid);
                    }

                    else {

                        send_message_self( "<< message cannot be null\r\n", cli->sockfd);
                    }
                }

                else {

                    send_message_self("<< user id cannot be null\r\n", cli->sockfd);
                }
            } else if( !strcmp(command, "/list")) {

                sprintf( buff_out, "<< clients %d\r\n", ClientsNumber);


                send_message_self(buff_out, cli->sockfd);
                send_active_client( cli->sockfd);
            } else if( !strcmp(command, "/help")) {

                strcat(buff_out, "<< /quit  Quit whiteboard\r\n");
                strcat(buff_out, "<< /ping  Server test\r\n");
                strcat( buff_out, "<< /topic  <message> Set chat topic\r\n");
                strcat( buff_out, "<< /nick  <name> Change nickname\r\n");

                strcat(buff_out, "<< /msg      <user id> <message> Send private message\r\n");
                strcat(buff_out, "<< /list     Show active clients\r\n");
                strcat(buff_out, "<< /help     Show help\r\n");

                send_message_self(buff_out, cli->sockfd);
            } else {
                send_message_self("<<unknown command\r\n", cli->sockfd);
            }
        }


        else {
            //Send File
            /*
            if(strncmp(buff_in,"file", 4)==0)
            {

            	total = 0;
            	printf("Sending File");
            	//printf("File Name: %s\n",buff_in);
            	str_trim_file_name(buff_in, Buffer_Size);

            	ssize_t n;
            	char receivepart[Buffer_Size] = {};
            	//snprintf(buff_out , 2*sizeof(buff_out), "[%s]%s\r\n" , cli->name , buff_in);
            	send_message(buff_in, cli->user_id);
            	//printf("Chunk0:%s\n",buff_in);
            	bzero(buff_in ,Buffer_Size);
            	while ((n = read(cli->sockfd,buff_in,Buffer_Size)) > 0)
            	{
            		total+=n;
            		send_message(buff_in, cli->user_id);
            		printf("Chunk: %s\n", buff_in);
            		if (n == -1)
            		{
            			perror("Connection Error, Receive File Error");
            			exit(1);
            		}

            		//memset(buff, 0, MAX_LINE);
            		//str_overwrite_stdout();
            		bzero(buff_in ,Buffer_Size);
            	}
            	printf("Receive Success, NumBytes = %ld\n", total);
            }
            */
            if(strncmp(buff_in,"file", 4)==0) {
                str_trim_file_name(buff_in, Buffer_Size);
                printf("%s\n", file_name);
                char temp_buffer[strlen(file_name) + 5];
                strcpy(temp_buffer, "file ");
                strcat(temp_buffer, file_name);
//                printf("%s\n",temp_buffer);
                //write(sockfd,file_name,strlen(file_name));
                //sprintf(message,"%s:%s\n",name,buffer);
//                write(sockfd,buffer,strlen(buffer));
//                bzero(buffer,Buffer_Size);
                send_message(temp_buffer,cli->user_id);
                int file_len = read(cli->sockfd,buff_in,sizeof(buff_in)-1);
                buff_in[file_len] = '\0';
                FILE *temp_fp = fopen("temp.txt","wb");
                fwrite(buff_in, sizeof(char), file_len,temp_fp);
                bzero(buff_in, file_len);
                fclose(temp_fp);
                FILE *fp = fopen("temp.txt", "rb");
                if (fp == NULL) {
                    perror("Can't open file");
                    exit(1);
                    return NULL;
                }
                sendfile(fp, cli->user_id, temp_buffer);
                fclose(fp);

                //printf("Receive Success, NumBytes = %ld\n", total);
                 printf("Receive Success\n");
            }
            //Send Message
            else {
                //snprintf(buff_out, 2*sizeof(buff_out), "[%s]%s\r\n", cli->name, buff_in);
                strcat(buff_out,cli->name);
                strcat(buff_out,buff_in);
                strcat(buff_out,"\r\n");
                send_message(buff_out, cli->user_id);
            }
        }


    }


    //Close connection

    sprintf( buff_out, "<< %s has left\r\n", cli->name);

    send_message_all(buff_out);
    close ( cli->sockfd);

    //Delete CLient from the queue and yield thread

    client_remove(cli->user_id);

    printf("<<quit ");

    print_client_addr(cli->address);
    printf(" user id: %d\r",cli->user_id);

    free(cli);
    client_count--;

    pthread_detach(pthread_self());

    return NULL;
}
















//Main Method

int main(int argc,char **argv) {

    if(argc!=2) {
        printf("%s port is in use\n",argv[0]);
        return EXIT_FAILURE;
    }


    int port=atoi(argv[1]);
    int option =1;


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

    if(setsockopt(listenfd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),/*(char*) */&option,sizeof(option))<0) {
        printf("ERROR: in setting setsockopt\n");
        return EXIT_FAILURE;
    }

    //Bind

    if(bind(listenfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0) {
        printf("ERROR: error in binding\n");
        return EXIT_FAILURE;
    }

   // printf("before connect\n");


    //listen

    if(listen(listenfd,10)<0) {
        printf("ERROR: error in listening\n");
        return EXIT_FAILURE;
    }

    printf("\n*****WHITEBOARD*****\n\n");
    int client_len=sizeof(client_addr);

    while(1) {

        connectionfd=accept(listenfd,(struct sockaddr*)&client_addr,&client_len);

        if(connectionfd<0) {

            //perror("accept");

            printf("Error:in accepting\n");

            exit(EXIT_FAILURE);
        }
        //Check for client capacity




        if((client_count+1)==ClientsNumber) {
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
