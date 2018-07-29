#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_USER 5
#define MAX_CLI 5
#define MAX_FILE 10
#define MAX_BUF_SIZE 1024

struct cli_info{
    int connfd ;
    int file_state[MAX_FILE]  ;
    char sendbuf[MAX_BUF_SIZE] ;
    char *begin ;
};
struct up_info{ 
    int connfd  ; 
    FILE* fd  ;
    char file[256] ;
};
struct down_info{
   int connfd ;
   FILE* fd ;
   char data[MAX_BUF_SIZE];
   char *begin , *end ;
} ;
struct user_info{
    char username[256] ;
    char filename[MAX_FILE][256] ;
    struct cli_info client[MAX_CLI] ;
    struct up_info up[MAX_CLI] ;
    struct down_info down[MAX_CLI * MAX_FILE] ; 
};
void initial_user(struct user_info *user) ;
void initial_cli(struct cli_info *client) ;
void initial_up(struct up_info *up) ;
void initial_down(struct down_info *down) ;

int main(int argc , char **argv){
     int listenfd , connfd ;
     char buf[MAX_BUF_SIZE] ;
     struct sockaddr_in servaddr,cliaddr ;
     int clilen ;
     struct user_info user[MAX_USER] ; 
     fd_set rset , allrset , allwset , wset ;
     int maxfdp1 , i ;

     listenfd = socket(AF_INET,  SOCK_STREAM , 0) ;

     bzero(&servaddr , sizeof(servaddr)) ;
     servaddr.sin_family = AF_INET ; 
     servaddr.sin_port = htons(atoi(argv[1])) ;
     servaddr.sin_addr.s_addr = htonl(INADDR_ANY) ;

     bind(listenfd , (struct sockaddr *)&servaddr , sizeof(servaddr)) ;
      
     listen(listenfd , 100) ;
     
     //initialize 
     FD_ZERO(&allrset) ;
     FD_ZERO(&allwset) ;
     FD_SET(listenfd , &allrset) ;

     for(i = 0 ; i < MAX_USER  ; i++) initial_user( &user[i] ) ;
     maxfdp1 = listenfd + 1 ;
      
     for(;;){
          rset = allrset ; 
          wset = allwset ;
          select(maxfdp1 , &rset , &wset , NULL , NULL ) ;
          if(FD_ISSET(listenfd , &rset )){
              int connfd = accept(listenfd , NULL , NULL) ;
              maxfdp1 = (maxfdp1 > connfd + 1) ? maxfdp1 : connfd + 1 ;
              // read connection type
              int n = read(connfd , buf , sizeof(buf)) ;
              char type[5]  ;
              char username[256] ;
              char filename[256] ;  //+5 + 256
              strcpy(type , buf) ;
              strcpy(username , buf+5);
              strcpy(filename ,buf+261) ;
              fprintf(stderr ,"%s %s %s\n" ,type ,username , buf+261) ;
              for(i = 0 ; i < MAX_USER ; i++){
                  if( (strcmp(user[i].username , username) != 0) && 
                       (strlen(user[i].username) != 0) ) continue ;         
                  //find corresponding user or first user
                  //set user info
                  if(strcmp(type , "CMD") == 0 ){
                       if(strlen(user[i].username) == 0 ) strcpy(user[i].username  , username) ;
                       int j ; 
                       for(j = 0 ; j < MAX_CLI ; j++){
                             if(user[i].client[j].connfd == -1){
                                  user[i].client[j].connfd = connfd ; 
      
                                  int k ;
                                  for(k = 0 ; k < MAX_FILE ; k++){
         				if(strlen(user[i].filename[k]) == 0 ) continue ;
                                        user[i].client[j].file_state[k] = 0 ;
                                  }
                                  FD_SET(connfd ,&allrset) ;
                                  FD_SET(connfd ,&allwset) ;
                                  break;                              
                             }
                       }
                       if(j == MAX_CLI){
                           fprintf(stderr , "Too many client in same iuser\n") ;
                           exit(-1) ;
                        }
                       
                       sprintf(buf , "Welcome to the dropbox-like server! : %s" , username) ;
                       write(connfd , buf , sizeof(buf)) ;
                       int flag = fcntl(connfd , F_GETFL ,0) ;
                       fcntl(connfd , F_SETFL ,flag | O_NONBLOCK) ;
                  }
                  else if(strcmp(type , "UP") == 0 ){
                      int j ;
                      for(j = 0 ; j < MAX_CLI  ; j++){
                          if(user[i].up[j].connfd == -1 ){
                                char save_name[512] ;
                                sprintf(save_name , "%s_%s" , username , filename) ;
                                user[i].up[j].connfd = connfd ;
                                user[i].up[j].fd = fopen(save_name , "wb") ;
                                strcpy(user[i].up[j].file ,filename ) ;
                                FD_SET(connfd ,&allrset) ;
                                break;
                          }
                      }
                      if(j == MAX_CLI ){
                             fprintf(stderr , "Too many uploading connection\n") ;
                             exit(-1) ;
                       }    
                
                  }
                  else if(strcmp(type , "DOWN") == 0){
                      int j ;
                      for(j = 0 ; j < MAX_CLI * MAX_FILE  ; j++){
                             if(user[i].down[j].connfd == -1){
                                  char save_name[512] ;
                                  sprintf(save_name , "%s_%s" ,username ,filename) ;
                                  user[i].down[j].connfd = connfd ; 
                                  user[i].down[j].fd = fopen(save_name , "rb") ;
  
                                  int flag = fcntl(connfd , F_GETFL ,0 ) ;
                                  fcntl(connfd , F_SETFL , flag | O_NONBLOCK) ;

                                  FD_SET(connfd , &allwset) ;
                                  break;
                             }
                      }
                     if(j == MAX_CLI *MAX_FILE){
                        fprintf(stderr , "Too many downloading connection\n") ;
                        exit(-1) ;
                        }
                  }
                  else {
                       fprintf(stderr , "Error header\n")  ;
                       exit(-1) ;
                  }
                  break ;
              }
             if(i == MAX_USER){//too many user or cannot find the user
                   fprintf(stderr , "Too many user or cannot fine the corresponding user\n") ;
                     exit(-1) ;
             }
          }
          for(i = 0 ; i < MAX_USER ; i ++){
                int j ;
                //for cmd connection
                for(j = 0 ; j < MAX_CLI ; j++){
                    if(user[i].client[j].connfd == -1) continue ; 
                    if( FD_ISSET(user[i].client[j].connfd , &rset) ){
                         //read wheather client close the connect
                         int n =  read(user[i].client[j].connfd , buf , sizeof(buf)) ;
                         if(n == 0){
                               FD_CLR(user[i].client[j].connfd , &allrset) ;
                               FD_CLR(user[i].client[j].connfd , &allwset) ;
                               FD_CLR(user[i].client[j].connfd , &rset) ;
                               FD_CLR(user[i].client[j].connfd , &wset) ;

                               close(user[i].client[j].connfd) ;
                               initial_cli( & (user[i].client[j]) );
                          }
                     }
                  if( FD_ISSET(user[i].client[j].connfd , &wset)){
                          int k ;
                          for(k = 0 ; k < MAX_FILE ;k++){
                               if(user[i].client[j].file_state[k] == 0) {
                                    if(user[i].client[j].begin == user[i].client[j].sendbuf){
                                        sprintf(user[i].client[j].sendbuf , "%s"
                                                , user[i].filename[k] ) ;
                                     }
                                    int n = write(user[i].client[j].connfd
                                              , user[i].client[j].begin
                                              , user[i].client[j].sendbuf + MAX_BUF_SIZE -
                                                user[i].client[j].begin ) ;
                                    if(n < 0){
                                        if(errno == EWOULDBLOCK) {printf("Be blocked\n");break ;}
                                        else {printf("Write Error\n") ;exit(-1);}
                                     }
                                    user[i].client[j].begin += n ;
                                    if(user[i].client[j].begin != 
                                       user[i].client[j].sendbuf + MAX_BUF_SIZE ){
                                              printf("Still remain data\n")  ;                  
                                              break;
                                           }
                                    //write all data ,get here
                                    user[i].client[j].begin = user[i].client[j].sendbuf ;
                                    user[i].client[j].file_state[k] = 1 ;
                               }
                          }
                     }
                }
                //for up connection
                for(j = 0 ; j < MAX_CLI ; j++){
                     if(user[i].up[j].connfd == -1) continue ;
                     if(FD_ISSET(user[i].up[j].connfd , &rset)){
                          int n ;
                          while( (n = read(user[i].up[j].connfd , buf ,sizeof(buf))) != 0 ){
                               fwrite(buf , sizeof(char) ,n ,user[i].up[j].fd) ; 
                          }
                          //file uploading complete
                          printf("File : %s uploading complete\n" , user[i].up[j].file) ;
                          //save filename into user info and updata every client's file_state
                          int k , index = -1;
                          for(k = 0 ; k < MAX_FILE ;k++){
                              if(strlen(user[i].filename[k]) > 0) continue ;
                              strcpy(user[i].filename[k] , user[i].up[j].file);
                              index = k ;
                              break ;  
                          }
                          if(k == MAX_FILE) printf("Error : Too many files with same user\n") ;
                          for(k = 0 ; k < MAX_CLI ;k++){
                                if(index == -1) break ;
                                if(user[i].client[k].connfd == -1) continue ;
                                user[i].client[k].file_state[index] = 0 ;
                          }
                          
                          fclose(user[i].up[j].fd) ;
                          close(user[i].up[j].connfd) ;
                          
                          FD_CLR(user[i].up[j].connfd , &allrset) ;
                          FD_CLR(user[i].up[j].connfd , &rset) ;

                          initial_up( &(user[i].up[j]) ) ;
                               
                     }
                 }
                //for down connection
                for(j = 0 ; j < MAX_CLI * MAX_FILE  ; j++){
                      if(user[i].down[j].connfd == -1) continue ;
                      if( FD_ISSET(user[i].down[j].connfd , &wset) ){
                             while(1){
                                 int n ;
                                 //check wheather data is empty ,if yes, read file data into it
                                 //if file is eof ,download ok and break 
                                 if(user[i].down[j].end == user[i].down[j].data){
                                    n = fread(user[i].down[j].data , 1
				           , sizeof(user[i].down[j].data) ,user[i].down[j].fd) ;
                                    user[i].down[j].end += n ;
                                    if(n == 0){
                                         printf("File download complete\n") ;
                                         FD_CLR(user[i].down[j].connfd , &wset) ;
                                         FD_CLR(user[i].down[j].connfd , &allwset) ;
                                         close(user[i].down[j].connfd) ;
                                         initial_down( &(user[i].down[j]) ) ;
                                         break; 
                                     }
                                 }
                                 //write file to client
                                 n = write(user[i].down[j].connfd ,user[i].down[j].begin
						 , user[i].down[j].end - user[i].down[j].begin ) ;
                                 if(n < 0){
                                       if(errno == EWOULDBLOCK) break ;
                                       else {
                                          printf("Error:Download file fails\n") ;exit(-1) ;
                                       }
                                  }
                                  user[i].down[j].begin += n ;
                                  if(user[i].down[j].begin == user[i].down[j].end){
                                       user[i].down[j].begin = user[i].down[j].data ;
                                       user[i].down[j].end = user[i].down[j].data ;
                                   }
                              }
                      }
                }
          }
             
     }
    
}
void initial_user(struct user_info *user){
    user -> username[0] = '\0' ;
    int i ;
    for(i = 0 ; i < MAX_FILE ; i ++) user -> filename[i][0] = '\0' ;
    for(i = 0 ; i < MAX_CLI ;i++)  initial_cli(  &(user -> client[i]) ) ;
    for(i = 0 ; i < MAX_CLI ; i++) initial_up(  &(user -> up[i]) ) ;
    for(i = 0 ; i < MAX_CLI * MAX_FILE ; i ++) initial_down( &(user -> down[i]) ) ;
}
void initial_cli(struct cli_info *client) { 
    client -> connfd = -1 ; 
    int i ;
    for( i = 0 ; i < MAX_FILE ; i++){
        client -> file_state[i] = -1 ;
    }
    client -> begin = client -> sendbuf ;
   
}
void initial_up(struct up_info *up){
    up ->connfd = -1 ;
    up -> file[0] = '\0' ;
}
void initial_down(struct down_info *down){
     down -> connfd = -1 ;
     down -> begin = down -> data ;
     down -> end = down -> data ;
}
