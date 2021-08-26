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
#define Name_Len 3200



volatile sig_atomic_t flag=0;
int sockfd=0;
char name[Name_Len];
char file_name[100];
ssize_t total=0;




//clearing message

void str_overwrite_stdout() {
    printf("\r%s",">");
    fflush(stdout);
}

//completing chat

void str_trim_lf(char* array,int length) {

    for(int i=0; i<length; i++) {
        if(array[i]=='\n') {
            array[i]='\0';
            break;
        }
    }

}

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

//creating condition to be true or false

void catch_message_return() {

    flag=1;
}


//Send File
void sendfile(FILE *fp, int sockfd) {
    int n;
    char sendpart[Buffer_Size] = {0};
    total =0;
    while ((n = fread(sendpart, sizeof(char), Buffer_Size, fp)) > 0) {
        total+=n;
        if (n != Buffer_Size && ferror(fp)) {
            perror("No file Exist or File name error");
            exit(1);
        }
        printf("File Content: %s\n", sendpart);
        write(sockfd, sendpart, n);
        bzero(sendpart, Buffer_Size);
    }
    fclose(fp);
    return;
}

//Receive File
void writefile(int sockfd, FILE *fp) {
    ssize_t n;
    total =0;
    char receivepart[Buffer_Size] = {0};

    while (1) {
        n = read(sockfd,receivepart,Buffer_Size);
        if(n <= 0) break;
        total+=n;
        if (n == -1) {
            perror("Connection Error, Receive File Error");
            //exit(1);
            return;
        }
        printf("%s",receivepart);
        if (fwrite(receivepart, sizeof(char), n, fp) != n) {
            perror("Write File Error");
            return;;
            //exit(1);
        }
        //memset(buff, 0, MAX_LINE);
        str_overwrite_stdout();
        bzero(receivepart,Buffer_Size);
        if(n<Buffer_Size)break;
    }
}

//handle receiving messages
void receiving_message_handler() {

    char message[Buffer_Size]= {};
    bzero(message,Buffer_Size);
    while(1) {
        //int receive=recv(sockfd,message,Buffer_Size,0);
        int receive = read(sockfd,message,Buffer_Size);
        printf("Received: %s\n",message);
        if(receive>0 && strncmp(message,"file", 4)!=0) {

            str_overwrite_stdout();
        }

        else if(receive>0 && strncmp(message,"file", 4)==0) {
            str_trim_file_name(message, Buffer_Size);
            str_overwrite_stdout();
            printf("Received File Name: %s\n", file_name);

            //write(sockfd,file_name,strlen(file_name));
            /*
            if (recv(sockfd, message, Buffer_Size, 0) == -1)
            {
            	perror("Connection Problem. Can't send filename");
            	//exit(1);
            	return;
            }
            */
            char file_name_new[strlen(file_name)+3];
            strcpy(file_name_new,"1_");
            strcat(file_name_new,file_name);
            FILE *fp = fopen(file_name_new, "wb");
            if (fp == NULL) {
                perror("Can't open file");
                //exit(1);
                return;
            }

            writefile(sockfd, fp);
            //printf("Receive Success, NumBytes = %ld\n", total);
              printf("Receive Success\n");
            fclose(fp);

        }

        else if(receive<=0) {

            break;
        }

        bzero(message,Buffer_Size );
    }
}


//Message Sending function

void sending_message_handler() {

    char buffer[Buffer_Size]= {};
    //int sockfd;
    int b;
    char message[Buffer_Size+Name_Len]= {};

    while(1) {

        str_overwrite_stdout();

        fgets(buffer,Buffer_Size,stdin);

        str_trim_lf(buffer,Buffer_Size);

        if(strcmp(buffer,"bye")==0) {

            break;
        }


        else if(strncmp(buffer,"file", 4)==0) {
            str_trim_file_name(buffer, Buffer_Size);
//            printf("%s\n",buffer);
            printf("%s\n", file_name);
//            write(sockfd,file_name,strlen(file_name));
            //sprintf(message,"%s:%s\n",name,buffer);
            write(sockfd,buffer,strlen(buffer));
            sleep(1);
            bzero(buffer,Buffer_Size);
            FILE *fp = fopen(file_name, "rb");
            if (fp == NULL) {
                perror("Can't open file");
                //exit(1);
                return;
            }

            sendfile(fp, sockfd);
            //printf("Send Success, NumBytes = %ld\n", total);
              printf("Send Succes\n");
//            fclose(fp);

        } else {
            sprintf(message,"%s:%s",name,buffer);
            //send(sockfd,message,strlen(message),0);
            write(sockfd,message,strlen(message));
        }

        bzero(buffer,Buffer_Size);
        bzero(message,Buffer_Size+Name_Len);

    }

    catch_message_return(2);

}


//Main method

int main(int argc,char **argv) {

    if(argc!=3) {
        printf("%s port is in use\n",argv[0]);
        return EXIT_FAILURE;
    }

    int port=atoi(argv[2]);


    signal(SIGINT,catch_message_return);

    printf("Enter the client's name: ");

    fgets(name,Name_Len,stdin);

    str_trim_lf(name,strlen(name));

    if(strlen(name)>32-1||strlen(name)<2) {

        printf("Enter name correctly\n");

        return EXIT_FAILURE;
    }


    struct sockaddr_in server_addr;


    //socket creating

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(argv[1]);
    server_addr.sin_port=htons(port);

    //printf("before connecting\n");

    //Connect to the server

    int value=connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));

    //printf("%d values",value);
    if(value<0) {
        printf("ERROR in connecting\n");

        return EXIT_FAILURE;
    }


   // printf("%s",name);

    //Send Client Name

    send(sockfd,name,Name_Len,0);

    printf("\n***********WHITEBOARD**********\n\n");

    pthread_t message_sending_thread;

    //while(1){
    if(pthread_create(&message_sending_thread,NULL,(void*)sending_message_handler,NULL)!=0) {

        printf("ERROR:in creating pthread\n");

        return EXIT_FAILURE;
    }

    //printf("after connecting\n");


    pthread_t message_receiving_thread;

    if(pthread_create(&message_receiving_thread,NULL,(void*)receiving_message_handler,NULL)!=0) {

        printf("ERROR:in creating pthread\n");

        return EXIT_FAILURE;
    }


    while(1) {
        if(flag) {

            printf("\nExit\n");

            break;

        }

        //sending_message_handler();
        //sreceiving_message_handler();
    }


    close(sockfd);

    return EXIT_SUCCESS;
}

