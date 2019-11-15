#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>

#define STATCODE_404 "404 Bad Request\n"
#define STATCODE_200 "200 OK\n"
#define STATCODE_204 "No Content\n"
#define MAX_THREAD_NUM 20
#define BUFSIZE 8096
#define BLOCK_SIZE 4096

int close_file = 0;

void* connection_handler(void* socket_fd) {

  char read_buf[4096], write_buf[BUFSIZE], buffer[BUFSIZE];
  int socketfd = *(int*)socket_fd; 
  int fd, file_fd, ret;
  long len;
  FILE* fp;
  
  read(socketfd, read_buf, 4096);
  printf("%s", read_buf);
      
    // Handle POST request.
    if (strncmp(read_buf, "POST", 4)==0) {
      char received_file_name[1024];
    // Cut the image file name.
    char *start_pos1 = strstr(read_buf, "filename=") + 10;
    char *ptr1 = start_pos1;
    int j = 0;
    while (*ptr1!='"') {
      received_file_name[j] = *ptr1;
      j++;
      ptr1++;
    }
    received_file_name[j] = '\0';
    // get the content length
    char tmp_num[100];
    start_pos1 = strstr(read_buf, "Content-Length: ") + 16;
    ptr1 = start_pos1;
    j = 0;
    while (*ptr1==' ')  ptr1++;
    while ( isdigit(*ptr1) ) {
      tmp_num[j] = *ptr1;
      j++;
      ptr1++;
    }
    tmp_num[j] = '\0';
    int read_bytes = atoi(tmp_num); // For reading the file.
     printf("bytes need to read : %d\n", read_bytes);

    // copy the file from html response.
      // Find the first blank line.
    start_pos1 = strstr(read_buf, "\n\n") + 2;
    char block[4096] = { 0 };
    j = 0;
     
    // Start copying, read in block
    FILE* fp2 = fopen(received_file_name, "a");
    if ( fp2 == NULL)
      printf("Create file error.\n");
       
    char* tmp_ptr = start_pos1;
    while( read_bytes > 0) {
      if ( j >= BLOCK_SIZE - 3 ) { //flush this buffer.
        fputs(block, fp2);    
        j = 0;
        memset(block, 0, BLOCK_SIZE);
      }
      block[j++] = *tmp_ptr;
      tmp_ptr++;
      read_bytes--;
    }
    fputs(block, fp2);    
    printf("block content: !!!!!!!!!!!!!!!\n%sblock content end!!!!!!!!!", block);
   fclose(fp2); 
    return 0;
    // Handle image request.
    } else if(strncmp(read_buf, "GET", 3)==0 && strstr(read_buf, "image")!=NULL) {    
    // Cut the image file name.
    char *start_pos = strstr(read_buf, "/") + 1;
    char *ptr = start_pos;
        char image_file[1024];
        int i = 0;
        while (*ptr!=' ') {
          image_file[i] = *ptr;
          i++;
          ptr++;
        }
        image_file[i] = '\0';
        
        sprintf(write_buf+strlen(write_buf), "Access-Control-Allow-Origin: *\n");
        sprintf(write_buf + strlen(write_buf), "Content-Type: image/jpg\n");
        sprintf(write_buf + strlen(write_buf), "Server: MyWebServer\n\n");

        // Open image file
	if(( file_fd = open(image_file,O_RDONLY)) == -1) {  /* open the file for reading */
          printf("open file error.\n");
	}
	len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
	      lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
          sprintf(buffer,"HTTP/1.1 200 OK\nContent-Type: image/jpg\nContent-Length: %ld\nConnection: Close\n\n", len); /* Header + a blank line */
	write(socketfd,buffer,strlen(buffer));
	/* send file in 8KB block - last block may be smaller */
	while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
          write(socketfd,buffer,ret);
               
      // For text request.
        }
        close_file = 1;
        return 0;
      } else {
        sprintf(write_buf, "HTTP/1.1 ");
        sprintf(write_buf + strlen(write_buf), STATCODE_200);
        sprintf(write_buf+strlen(write_buf), "Access-Control-Allow-Origin: *\n");
        sprintf(write_buf + strlen(write_buf), "Content-Type: text/html; charset=utf-8\n");
        sprintf(write_buf + strlen(write_buf), "Server: MyWebServer\n\n");
        //printf("file ready to open.\n");
        if ( (fd = open( "index.html", O_RDONLY))==-1) {
           memset(write_buf, 0, sizeof(write_buf)); 
           sprintf(write_buf, STATCODE_204);               
         } else {
           printf("Resource exist.\n");
           while (read(fd, write_buf + strlen(write_buf), 1024)>0) continue;
        }
      //printf("%s",write_buf);
      send(socketfd, write_buf, strlen(write_buf), 0);
      memset(read_buf, 0, sizeof(read_buf));
      memset(write_buf, 0, sizeof(write_buf));
      //close(socketfd);
    }
    // Bad request.
    memset(write_buf, 0, sizeof(read_buf));
    printf("404 Bad Request.\n");
    printf("\n\n%s", read_buf);
    sprintf(write_buf , "HTTP/1.1 ");
    sprintf(write_buf + strlen(write_buf), STATCODE_404);
    //sprintf(write_buf + strlen(write_buf), "Connection: close\n");
    write(socketfd, write_buf, strlen(write_buf));
   
  return 0;      
}

int main() {

  int listenfd, socketfd, pid;
  int fd;
  static struct sockaddr_in serv_addr;
  static struct sockaddr_in client_addr;
  char *seg_ptr;
  FILE *fp;
  
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(8080);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  //inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
  bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));
  if (listen(listenfd, 4)==-1) {
    printf("listen failed.");
    exit(0);
  }
  int len;

    printf("listening.\n");
    //socketfd = accept(listenfd, (struct sockaddr*)&client_addr, &len);
    //printf("A socket connected.\n");

  pthread_t thread_id;
  while (socketfd = accept(listenfd, (struct sockaddr*)&client_addr, &len)) {
    printf("Connection accepted.\n");
    if ( pthread_create(&thread_id, NULL, connection_handler, (void*)&socketfd) < 0 )
      printf("Thread creation error.\n");
    //if (close_file==1) {
    //  close(socketfd);
    //  return 0;
    //}
  }
  return 0;
}
