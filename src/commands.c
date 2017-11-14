#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <sys/termios.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <assert.h>
#include <sys/stat.h>

#define BUFF_SIZE 1024
#define PATH "tpf_unix_sock.socket"

#include "commands.h"
#include "built_in.h"
       

int status;
int bg_status;
int in_bg;
int bg;
char bg_command[1024];
int fg_check;
static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
	  return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
void *myfuc(void *arg) {
	struct single_command exe[512];
	struct single_command *b=(struct single_command *)arg;
	memcpy(&exe[0],b,sizeof(struct single_command));

	int server_socket;
	int client_socket;

	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;
	

	server_socket=socket(PF_LOCAL,SOCK_STREAM,0);
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sun_family=PF_UNIX;
	strcpy(server_addr.sun_path,PATH);

	unlink(PATH);
	while(1){
		if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1){
		printf("bind error");
		exit(0);
	}

		if(listen(server_socket,5)==-1){
			printf("fail to listen");
			exit(0);
		}
	
		int client_addr_size=sizeof(client_addr);
		client_socket=accept(server_socket,(struct sockaddr*)&client_addr,&client_addr_size);
		if(fork()==0)
		{
			close(0);
			dup2(client_socket,0);
			evaluate_command(1,&exe);
			exit(0);
		}
		wait(&status);
		close(client_socket);
		close(server_socket);
		pthread_exit(0);
		exit(0);
	}
}

int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  
	if(n_commands>1){
		int client_socket;
		struct sockaddr_un server_addr;
		char buff[BUFF_SIZE];

		struct single_command a[512];
		struct single_command b[512];
		struct single_command *c1=(*commands);
		struct single_command *c2=&(*commands)[1];
		
		memcpy(&a[0],c1,sizeof(struct single_command));
		memcpy(&b[0],c2,sizeof(struct single_command));

		pthread_t thread_id;
		pthread_create(&thread_id,NULL,myfuc,&b);

		if(fork()==0)
		{
			client_socket=socket(PF_LOCAL,SOCK_STREAM,0);
			memset(&server_addr,0,sizeof(server_addr));
			server_addr.sun_family=PF_UNIX;
			strcpy(server_addr.sun_path,PATH);
			while(connect(client_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1);
			if(fork()==0)
			{
				close(0);
				close(1);
				dup2(client_socket,1);
				evaluate_command(1,&a);
				exit(0);
			}
			wait(&status);
			pthread_join(thread_id,NULL);
			close(client_socket);
			exit(0);
		}
		wait(&status);
	
	}

	else if (n_commands ==1) {
 		struct single_command* com = (*commands);
    	assert(com->argc != 0);

    	int built_in_pos = is_built_in_command(com->argv[0]);
		
    	if (built_in_pos != -1) {
      		if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        		if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          			fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
       			}
      		}
		
	  		else 
	  		{
        		fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        		return -1;
      		}
		}
		else if (strcmp(com->argv[0], "") == 0) {
      		return 0;
    	}
		else if (strcmp(com->argv[0], "exit") == 0) {
      		return 1;
    	}

		else {
			if(built_in_pos==-1){
		
				int pid;
				pid=fork();
				
				if(pid==-1){
					printf("fail to fork");
				}
				else if(pid>0){
					if(strcmp(com->argv[(com->argc)-1],"&")!=0){
						waitpid(pid,&status,0);
					
						if(in_bg==1){
							int a=waitpid(bg,&bg_status,WNOHANG);
							
							if(fg_check==0){
									
								if(a==bg){
									printf("%d done     %s\n",bg,bg_command);									in_bg=0;
									
								}
							}
							
						}
			
					}

					//"&" delete
					else{
						com->argv[(com->argc)-1]=NULL;
						--(com->argc);

						printf("%d\n",pid);
						strcpy(bg_command,com->argv[0]);
						for(int i=1;i<com->argc;i++){
						strcat(bg_command," ");
						strcat(bg_command,com->argv[i]);
						//bg_command==command deleted "&"
						}
						bg=pid;//pid in background
						in_bg=1; //informing some process is in  background 
		

					}//if background
						
				}
				
				else if(pid==0){
					if(fg_check==1){//if input 'fg',fg_check==1
						waitpid(bg,&bg_status,0);
						execlp(com->argv[0],com->argv[0],com->argv[1],NULL);
						execv(com->argv[0],com->argv);
						printf("%d done    %s\n",bg,bg_command);
						fg_check=0;
					}
					else{	
					execlp(com->argv[0],com->argv[0],com->argv[1],NULL);
					execv(com->argv[0],com->argv);
					}
				}

		  }
		  else{
		  	fprintf(stderr, "%s: command not found\n", com->argv[0]    );
			return -1;

		  }
	}
	
  }

  return 0;

}


void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
