#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 2

void * handle_clnt(void *arg);
void send_msg(char *msg, int len, int to);
void error_handling(char *msg);

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
char *connection_msg = "Matching!!!\n";
char wait_msg[17] = "Matching Wait...\n";
char error_msg[19] = "Full Connection!!!\n";
char *my_msg = "Your_turn!!!\n";
char *op_msg = "Opponent's_turn!!!\n";
int turn;

int main(int argc, char * argv[]) {
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	int i;
	char welcome_msg1[BUF_SIZE];
	char welcome_msg2[BUF_SIZE];

	if(argc!=2){
		printf("usage:%s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 2)==-1)
		error_handling("listen() error");

	while(1){
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++]=clnt_sock;
		if(clnt_cnt == 2){
			turn = 0;
			sprintf(welcome_msg1, "%s%s", connection_msg, my_msg);
			write(clnt_socks[0], welcome_msg1, sizeof(welcome_msg1));
			sprintf(welcome_msg2, "%s%s", connection_msg, op_msg);
			write(clnt_socks[1], welcome_msg2, sizeof(welcome_msg2));
		}
		else if(clnt_cnt < 2){
			for(i = 0; i < clnt_cnt; i++){
				write(clnt_socks[i], wait_msg, sizeof(wait_msg));
			}
		}
		else{
			for(i = 2; i < clnt_cnt; i++){
				write(clnt_socks[i], error_msg, sizeof(error_msg));
			}
		}
		pthread_mutex_unlock(&mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf("connected client IP: %s\n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}

void * handle_clnt(void *arg){
	int clnt_sock=*((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];
	char number_msg[BUF_SIZE];
	char win_msg[11] = "You_Win!!!\n";
	char lose_msg[12] = "You_Lose!!!\n";
	char not_turn_msg[17] = "Not your turn!!!\n";
	
	while((str_len = read(clnt_sock, msg, sizeof(msg))) != 0){
		if(clnt_sock == 4){
			if(turn == 1)
				send_msg(not_turn_msg, strlen(not_turn_msg), 1);
			if(!strcmp(msg, "Complete")){
				send_msg(win_msg, strlen(win_msg), 1);
				send_msg(lose_msg, strlen(lose_msg), 2);
				exit(1);
			}
			else{
				sprintf(number_msg, "%s %s", msg, op_msg);
				send_msg(number_msg, strlen(number_msg), 1);
				sprintf(number_msg, "%s %s", msg, my_msg);
				send_msg(number_msg, strlen(number_msg), 2);
				for(i = 0; i < BUF_SIZE; i++){
					msg[i] = 0;
				}
				for(i = 0; i < BUF_SIZE; i++){
					number_msg[i] = 0;
				}
				turn = 1;
			}
		}
		else if(clnt_sock == 5){
			if(turn == 0)
				send_msg(not_turn_msg, strlen(not_turn_msg), 2);
			if(!strcmp(msg, "Complete")){
				send_msg(lose_msg, strlen(lose_msg), 1);
				send_msg(win_msg, strlen(win_msg), 2);
				exit(1);
			}
			else{
				sprintf(number_msg, "%s %s", msg, my_msg);
				send_msg(number_msg, strlen(number_msg), 1);
				sprintf(number_msg, "%s %s", msg, op_msg);
				send_msg(number_msg, strlen(number_msg), 2);
				for(i = 0; i < BUF_SIZE; i++){
					msg[i] = 0;
				}
				for(i = 0; i < BUF_SIZE; i++){
					number_msg[i] = 0;
				}
				turn = 0;
			}
		}
		
	}
	pthread_mutex_lock(&mutx);
	for(i = 0; i < clnt_cnt; i++){
		if(clnt_sock == clnt_socks[i]){
			while(i++ < clnt_cnt - 1)
				clnt_socks[i] = clnt_socks[i + 1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}

void send_msg(char *msg, int len, int to){
	int i;
	pthread_mutex_lock(&mutx);
	if(to == 0){
		for(i = 0; i < clnt_cnt; i++){
			write(clnt_socks[i], msg, len);
		}
	}
	else if(to == 1){
		write(clnt_socks[0], msg, len);
	}
	else if(to == 2){
		write(clnt_socks[1], msg, len);
	}
	pthread_mutex_unlock(&mutx);
}

void error_handling(char * msg){
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}