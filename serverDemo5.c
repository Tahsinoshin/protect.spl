#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<netinet/in.h>



void error(const char *errmsg)//function for showing error msg
{
    perror(errmsg);
    exit(1);
 }


 
 int main(int argc ,char *argv[])
 {
 
 
	//argument for providing port number
    if(argc<2)
    {
        fprintf(stderr,"Port Number not provided.PLease Provide A Port number.\nProgram Terminated\n");
        exit(1);
     }

     int sockfd,newSockfd,portNo,input;

     char buffer[256];
     char appendText[5000];
	bzero(appendText,5000);
     
     struct sockaddr_in server_addr,client_addr;

     socklen_t client;
     
     //creating socket(store value from socket system call)
     
     sockfd=socket(AF_INET,SOCK_STREAM,0);

     if(sockfd<0)
     {
        error("Error opening Socket!");
     }
          
          bzero((char *)&server_addr,sizeof(server_addr));

          portNo=atoi(argv[1]);
          
          
          
          server_addr.sin_family=AF_INET;

          server_addr.sin_addr.s_addr=INADDR_ANY;

          server_addr.sin_port=htons(portNo);
          
          
          //binding socket to the port
          
          if(bind(sockfd,(struct sockaddr*) &server_addr,sizeof(server_addr))<0)
          {
             error("Binding Failed!");
            
          }
          
          //listening to request of client 
           
           listen(sockfd,5);

           client=sizeof(client_addr);
           
           //accepting the connection of client(store value from accept function call)
           
           newSockfd=accept(sockfd,(struct sockaddr*)&client_addr,&client);

           if(newSockfd<0)
           {
                error("Error on Accept!\n");
           }
           
           //reading and writing function
            
            while(1)
            {
		

		
                bzero(buffer,256);

                input=read(newSockfd,buffer,256);
                

                if(input<0)
                {
                        error("Error on reading!\n");
                }
                 

		strcat(appendText,buffer);
		printf("client:%s\n",appendText);
                 bzero(buffer ,256);

                 fgets(buffer,256,stdin);
                 
		 printf("msg: %s",buffer);
		 strcat(appendText,buffer);
                 
        input=write(newSockfd,appendText,strlen(appendText));
                                                                                             
         if(input<0)
         {
             error("Error on Writing!\n");
                                                                                                        
                                                                                    
         }

           int i=strncmp ("Bye",buffer,3);

           if(i==0)
	   {
                break;
	   }

       }
       
       
       
          close(newSockfd);

          close(sockfd);

          return 0;
    }                                                                        
                                                                                    
                 
                   
           
              
 
 
