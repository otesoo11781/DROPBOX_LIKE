#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define MAX_BUF_SIZE 1024
#define MAX_FILE 10
int main(int argc , char **argv){
      int sockfd ;
      struct sockaddr_in servaddr ;
      char buf[MAX_BUF_SIZE] ;
      fd_set rset ;
      int maxfdp1 ;
      int n ;
      char e_file[MAX_FILE][256] ;
      int k ;
      for(k = 0 ; k < MAX_FILE ; k++){
         e_file[k][0] = '\0' ;
      } 
      sockfd = socket(AF_INET , SOCK_STREAM , 0) ;

      bzero(&servaddr , sizeof(servaddr)) ;
      servaddr.sin_family = AF_INET ;
      servaddr.sin_port = htons(atoi(argv[2])) ;
      inet_pton(AF_INET , argv[1] , &servaddr.sin_addr) ;
      
      connect(sockfd , (struct sockaddr *)&servaddr , sizeof(servaddr)) ;
   
      sprintf(buf , "CMD" ) ;
      sprintf(buf+5 , "%s" , argv[3]);
      buf[261] = '\0' ;

      write(sockfd , buf , sizeof(buf)) ;
      
      n =read(sockfd , buf , sizeof(buf)) ;
      printf("%s\n" , buf) ;
     
      FD_ZERO(&rset) ;
      for(;;){
          FD_SET(0 , &rset) ;
          FD_SET(sockfd , &rset) ;
          maxfdp1 = sockfd + 1 ;
          select(maxfdp1 , &rset ,NULL , NULL , NULL );
          if(FD_ISSET(0 , &rset)){
                n = read(0 ,buf , sizeof(buf) )  ;
                if(n == 0) return 0;
                buf[n] = '\0' ;
                char *ptr = strtok(buf , " ") ;
                if(strcmp(buf , "/put") == 0){
                    ptr = strtok(NULL , "\n") ;
                    char filename[256] ;
                    strcpy(filename , ptr) ;
                    FILE *fp = fopen(filename , "rb") ;
                    int file_con = socket(AF_INET, SOCK_STREAM ,0) ;
                      
                    connect(file_con , (struct sockaddr *)&servaddr , sizeof(servaddr)) ;
                    sprintf(buf , "UP") ;
                    sprintf(buf+5 , "%s" , argv[3]) ;
                    sprintf(buf+261 , "%s" , filename) ;
                    write(file_con , buf , sizeof(buf)) ;
                    
                    printf("Uploading file : %s\n" , filename) ;
                    printf("Progress : [") ;
                    int n  ;
                    while( (n = fread(buf , 1 , sizeof(buf) , fp)) > 0 ){
                         write(file_con , buf , n) ;
                         printf("#") ;                         
                    }
                    printf("]\n") ;
                    printf("Upload %s complete!\n" , filename) ;
                    fclose(fp) ;
                    close(file_con) ;
                    for(k = 0 ; k < MAX_FILE ;k++){
                        if(strlen(e_file[k]) == 0 ){
                            strcpy(e_file[k] , filename) ;
                            break ;
                        } 
                    }
                }
                else if(strcmp(buf , "/exit\n") == 0){
                     return 0 ;
                }
                else if(strcmp(buf , "/sleep") == 0){
                   ptr = strtok(NULL , "\n") ;
                   int sec = atoi(ptr) , i ;
                   printf("Client starts to sleep\n") ;
              
                   for(i = 1 ; i <= sec ; i++){
                      sleep(1) ;
                      printf("Sleep %d\n" , i) ;
                 
                   }
                   printf("Client wakes up\n") ;
                }
           }
          if(FD_ISSET(sockfd , &rset)){
               //read downloading filename
               n = read(sockfd , buf , sizeof(buf)) ;
               if(n == 0){
                   printf("Error: server closed prematurely\n") ;
                   exit(-1) ; 
               }
              
               char filename[256] ;
               strcpy(filename , buf) ;
               //examine file wheather exists , if yes ,don't download;
               for(k = 0 ; k < MAX_FILE ; k++){
                   if(strcmp(e_file[k] , filename) == 0) break ;
               }
               if(k != MAX_FILE) continue ;

               FILE *fp = fopen(filename , "wb") ;

               int file_con = socket(AF_INET, SOCK_STREAM ,0) ;
               connect(file_con , (struct sockaddr *)&servaddr , sizeof(servaddr)) ;
               sprintf(buf , "DOWN") ;
               sprintf(buf+5 , "%s" ,argv[3]);
               sprintf(buf+261 ,"%s" ,filename) ;
               write(file_con , buf , sizeof(buf)) ;
             
               printf("Downloading file : %s\n" , filename) ;
               printf("Progress : [") ;
               while( (n = read(file_con , buf , sizeof(buf))) > 0 ){
                    fwrite(buf , 1 , n ,fp) ;
                    printf("#") ;
               }
              printf("]\n") ;
              printf("Download %s complete!\n" , filename) ;
              fclose(fp) ;
              close(file_con) ;
          }
      }
      
      
}
