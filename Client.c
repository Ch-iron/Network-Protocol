#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define LINE_SIZE 5
#define BINGO_SIZE LINE_SIZE * LINE_SIZE

void initialize();
void set_rand(int *arr);
void swap(int *x, int *y);
void print_bingo(int arr[LINE_SIZE][LINE_SIZE]);
void get_number(int sock);
void erase_bingo(int arr[LINE_SIZE][LINE_SIZE], int number);
int check_bingo(int arr[LINE_SIZE][LINE_SIZE]);

void * initializer(void * arg);
void * gaming(void * arg);
void * decide_winner(void *arg);
void error_handling(char *msg);

int bingo[LINE_SIZE][LINE_SIZE];
char msg[BUF_SIZE];
int is_matching = 0;
int is_first_player = 0;
int checked[25];
int count = 0;
int is_my_turn = 0;
int winner_num = 0;
int is_winner = 0;
int is_end = 0;
int sock;

void main(int argc, char *argv[]){
    int i, j;
    struct sockaddr_in serv_addr;
    pthread_t init_thread, gaming_thread, decide_thread;
    void *thread_return;

    if(argc != 3){
        printf("usage: %s <IP> <Port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)	
	error_handling("connect() error");

    pthread_create(&gaming_thread, NULL, gaming, (void*)&sock);
    pthread_create(&decide_thread, NULL, decide_winner, (void*)&sock);
    pthread_create(&init_thread, NULL, initializer, (void*)&sock);

    pthread_join(init_thread, &thread_return);
    pthread_join(decide_thread, &thread_return);
    pthread_join(gaming_thread, &thread_return);
    close(sock);
}

void initialize(){
    srand((unsigned int)time(NULL) + is_first_player);
    set_rand((int *)bingo);
}

void set_rand(int *arr){
    int i;
    for(i = 0; i < BINGO_SIZE; i++){
        arr[i] = i + 1;
    }
    for(i = 0; i < BINGO_SIZE; i++){
        swap(&arr[i], &arr[rand() % 25]);
    }
}

void swap(int *a, int *b){
    int tmp;

    tmp = *a;
    *a = *b;
    *b = tmp;
}

void print_bingo(int arr[LINE_SIZE][LINE_SIZE]){
    int i, j;

    printf("--<Your bingopan>--\n");
    for(i = 0; i < LINE_SIZE; i++){
        for(j = 0; j < LINE_SIZE; j++){
            printf("%d  ", arr[i][j]);
        }
        printf("\n\n");
    }
}

void get_number(int sock){
    int number;
    int i, is_retry;
    char msg[BUF_SIZE];

    do{
        is_retry = 0;
	printf("> Input 1~25: ");
	scanf("%d", &number);
	if(number < 1 || number > 25){
	    is_retry = 1;
	}
	if(is_retry == 0){
	    for(i = 0; i < BINGO_SIZE; i++){
	        if(checked[i] == number){
	            is_retry = 1;
	            break;
	        }
	    }
	}
    } while(is_retry == 1);

    checked[count++] = number;
    sprintf(msg, "%d", number);
    write(sock, msg, strlen(msg));
}

void erase_bingo(int arr[LINE_SIZE][LINE_SIZE], int number){
    int i, j;
    for(i = 0; i < LINE_SIZE; i++){
        for(j = 0; j < LINE_SIZE; j++){
            if(arr[i][j] == number){
                arr[i][j] = 0;
            }
        }
    }
}

int check_bingo(int arr[5][5]){
    int i, j, sum;
    int bingo_check[12] = {0};
    int bingo_cnt = 0;

    for(i = 0; i < LINE_SIZE; i++)
        for(j = 0; j < LINE_SIZE; j++)
            if(arr[i][j] == 0)
                bingo_check[i]++;

    for(i = 0; i < LINE_SIZE; i++)
        for(j = 0; j < LINE_SIZE; j++)
            if(arr[j][i] == 0)
                bingo_check[i + 5]++;

    for(i = 0; i < LINE_SIZE; i++)
        if(arr[i][i] == 0)
            bingo_check[10]++;


    for(i = 0; i < LINE_SIZE; i++)
        if(arr[i][LINE_SIZE - i - 1] == 0)
            bingo_check[11]++;

    for(i = 0; i < 12; i++)
        if(bingo_check[i] == 5)
            bingo_cnt++;

    return bingo_cnt;
}

void * decide_winner(void *arg){
    int sock = *((int*)arg);
    char msg[8] = "Complete";

    while(1){
	if(is_winner == 1){
		write(sock, msg, strlen(msg));
		break;
	}
    }
    return NULL;
}

void * initializer(void *arg){
    int sock=*((int*) arg);
    while(1){
        if(is_matching == 1){
            initialize();
            break;
        }
    }
    print_bingo(bingo);
    if(is_my_turn == 1){
    	get_number(sock);
    }

    return NULL;
}

void * gaming(void * arg){
	int sock=*((int*) arg);
	char info_msg[BUF_SIZE];
	int str_len;

	while(1){
		str_len = read(sock, info_msg, BUF_SIZE - 1);
		if(str_len == -1)
			return (void*)-1;
		info_msg[str_len] = 0;
		if(!strcmp(info_msg, "Matching Wait...\n")){
		    is_first_player = 1;
		    fputs(info_msg, stdout);
		}
		else if(strstr(info_msg, "Matching") != NULL){
		    if(strstr(info_msg, "Your") != NULL){
		    	is_matching = 1;
		    	is_my_turn = 1;
		    	fputs(info_msg, stdout);
		    }
		    else if(strstr(info_msg, "Opponent") != NULL){
			is_matching = 1;
			fputs(info_msg, stdout);
		    }
		}
		else if(!strcmp(info_msg, "Full Connection!!!\n")){
		    fputs(info_msg, stdout);
		    exit(1);
		}
		else if(strstr(info_msg, "Not") != NULL){
		    fputs(info_msg, stdout);
		}
		else if(strstr(info_msg, "Your") != NULL){
		    char *number = strtok(info_msg, " ");
		    erase_bingo(bingo, atoi(number));
		    winner_num = check_bingo(bingo);
		    number = strtok(NULL, " ");
		    if(strstr(number, "Win") != NULL || strstr(number, "Lose") != NULL){
			char *win_msg = strtok(number, "\n");
			printf("%s\n", win_msg);
			print_bingo(bingo);
	    		printf("bingo: %d\n", winner_num);
			win_msg = strtok(NULL, "\n");
			printf("%s\n", win_msg);
			exit(1);
		    }
		    else{
		        printf("\n%s", number);
		        print_bingo(bingo);
		        printf("bingo: %d\n", winner_num);
		    }
		    if(winner_num == 3)
			is_winner = 1;
		    else if(winner_num < 3)
		    	get_number(sock);
		}
		else if(strstr(info_msg, "Opponent") != NULL){
		    char *number = strtok(info_msg, " ");
		    erase_bingo(bingo, atoi(number));
		    winner_num = check_bingo(bingo);
		    number = strtok(NULL, " ");
		    printf("\n%s", number);
		    print_bingo(bingo);
		    printf("bingo: %d\n", winner_num);
		    if(winner_num == 3)
			is_winner = 1;
		}
		else if(strstr(info_msg, "Win") != NULL || strstr(info_msg, "Lose") != NULL){
			fputs(info_msg, stdout);
			exit(1);
		}
	}
	return NULL;
}

void error_handling(char * msg){
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}