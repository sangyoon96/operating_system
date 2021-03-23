#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

char curr_path[MAX_INPUT_SIZE];
char pps_path[MAX_INPUT_SIZE];
char ttop_path[MAX_INPUT_SIZE];

char **tokenize(char *line){

	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo=0;

	for(i =0; i < strlen(line); i++){
		char readChar = line[i];
		if(readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
			if(tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
				memset(tokens[tokenNo], 0, MAX_TOKEN_SIZE);
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
			}
		} 
		else {
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL ;

	return tokens;

}

int get_pipe_cnt(char *str){

	int i, pipe_cnt=0;

	for(i=0;i<strlen(str);i++){
		if(str[i] == '|') pipe_cnt++;
	}

	return pipe_cnt;

}

void implement_pipeline(char ***commands){

	int fd[2], backup = 0;
	pid_t pid;

	// have to modify
	while(*commands != NULL){
		pipe(fd);
		pid = fork();
		if(pid < 0){
			fprintf(stderr, "Fork Failed\n");
			exit(1);
		}
		else if(pid == 0){
			dup2(backup, 0);
			if (*(commands + 1) != NULL) {
				dup2(fd[1], 1);
			}
			close(fd[0]);
			if(strcmp((*commands)[0], "ttop") == 0){
				if(execv(ttop_path, *commands) < 0){
					printf("SSUShell : Incorrect command\n");
				}
			}
			else if(strcmp((*commands)[0], "pps") == 0){
				if(execv(pps_path, *commands) < 0){
					printf("SSUShell : Incorrect command\n");
				}
			}
			else{
				if(execvp((*commands)[0], *commands) < 0){
					printf("SSUShell : Incorrect command\n");
				}
			}
			exit(1);
		}
		else {
			wait(NULL);
			close(fd[1]);
			backup = fd[0];
			commands++;
		}
	}

	// have to free memory

}

int main(int argc, char **argv){

	char ch;
	char line[MAX_INPUT_SIZE];
	char **tokens, ***sets;
	int i, j, pipe_cnt;
	int command_cnt, token_idx;
	pid_t pid;

	memset(curr_path, 0, sizeof(char) * MAX_INPUT_SIZE);
	getcwd(curr_path, MAX_INPUT_SIZE);

	memset(pps_path, 0, sizeof(char) * MAX_INPUT_SIZE);
	sprintf(pps_path, "%s/pps", curr_path);

	memset(ttop_path, 0, sizeof(char) * MAX_INPUT_SIZE);
	sprintf(ttop_path, "%s/ttop", curr_path);

	FILE* fp;
	if(argc == 2) {
		fp = fopen(argv[1],"r");
		if(fp < 0) {
			printf("File doesn't exists.");
			return -1;
		}
	}

	while(1) {
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		if(argc == 2) { // batch mode
			if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
				break;
			}
			line[strlen(line) - 1] = '\0';
		} else { // interactive mode
			printf("$ ");
			scanf("%[^\n]", line);
			getchar();
		}
		/* END: TAKING INPUT */
		line[strlen(line)] = '\n';
		tokens = tokenize(line);

		// case pipe...!
		if(strstr(line, "|") != NULL){
			pipe_cnt = get_pipe_cnt(line);

			sets = (char ***)malloc(sizeof(char **) * (pipe_cnt + 2));
			memset(sets, 0, sizeof(char **) * (pipe_cnt+2));

			command_cnt=0;
			token_idx=0;

			for(i=0;tokens[i]!=NULL;i++){
				if(strstr(tokens[i], "|") != NULL){
					sets[command_cnt] = (char **)malloc(sizeof(char *) * (i - token_idx + 1));
					memset(sets[command_cnt], 0, sizeof(char *) * (i - token_idx + 1));
					for(j=token_idx;j<=i;j++){
						sets[command_cnt][j-token_idx]  = (char *)malloc(sizeof(char) * 100);
						if(j != i){
							memset(sets[command_cnt][j-token_idx], 0, sizeof(char) * 100);
							strcpy(sets[command_cnt][j-token_idx], tokens[j]);
						}
						else sets[command_cnt][j-token_idx] = NULL;
					}
					token_idx = i + 1;
					command_cnt++;
				}
			}

			sets[command_cnt] = (char **)malloc(sizeof(char *) * (i - token_idx + 1));
			memset(sets[command_cnt], 0, sizeof(char *) * (i - token_idx + 1));
			for(j=token_idx;j<=i;j++){
				sets[command_cnt][j-token_idx]  = (char *)malloc(sizeof(char) * 100);
				if(j != i){
					memset(sets[command_cnt][j-token_idx], 0, sizeof(char) * 100);
					strcpy(sets[command_cnt][j-token_idx], tokens[j]);
				}
				else sets[command_cnt][j-token_idx] = NULL;
			}
			command_cnt++;

			sets[command_cnt] = (char **)malloc(sizeof(char *) * MAX_NUM_TOKENS);
			sets[command_cnt] = NULL;

			/*
			printf("debug\n");
			for(i=0;i<=pipe_cnt;i++){
				printf("%s %s %s\n", sets[i][0], sets[i][1], sets[i][2]);
			}
			*/

			implement_pipeline(sets);

			// free memory
			/*
			   for(i=0;i<=pipe_cnt;i++){
			   for(j=0;sets[i]!=NULL;j++){
			   free(*(*(sets+i) + j));
			   (*(*(sets+i) + j)) = NULL;

			   }
			   }
			   */

		}
		else{
			pid = fork();
			if(pid < 0){
				fprintf(stderr, "Fork Failed\n");
				return 1;
			}
			else if(pid == 0){
				if(strcmp("ttop", tokens[0]) == 0){
					if(execv(ttop_path, tokens) < 0){
						printf("SSUShell : Incorrect command\n");
					}
				}
				else if(strcmp("pps", tokens[0]) == 0){
					if(execv(pps_path, tokens) < 0){
						printf("SSUShell : Incorrect command\n");
					}
				}
				else{
					if(execvp(tokens[0], tokens) < 0){
						printf("SSUShell : Incorrect command\n");
					}
				}
			}
			else{
				wait(NULL);
			}

			// Freeing the allocated memory
			for(i=0;tokens[i]!=NULL;i++){
				free(tokens[i]);
			}
			free(tokens);
		}
	}

	return 0;

}



