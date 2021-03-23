#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "getdata.h"

void pps(char *options){

	DIR *dp;
	struct dirent *entry;
	struct winsize max;
	struct proc_data proc;
	struct pid_data pid;
	char row[ROW_SIZE];
	char *curr_pid, *path;
	int curr_tty_nr, curr_uid;

	// get terminal size
	ioctl(0, TIOCGWINSZ, &max);

	// get info of current process
	curr_pid = (char *)malloc(sizeof(char) * 6);
	memset(curr_pid, 0, sizeof(char) * 6);
	sprintf(curr_pid, "%d", getpid());
	path = (char *)malloc(sizeof(char) * (strlen(curr_pid) + 7));
	memset(path, 0, sizeof(char) * (strlen(curr_pid) + 7));
	sprintf(path, "/proc/%s", curr_pid);
	read_pid(path, &pid);
	curr_tty_nr = pid.tty_nr;
	curr_uid = pid.uid;
	
	// free memory
	free_pid_memory(&pid, 0);
	free(curr_pid);
	free(path);
	curr_pid = NULL;	
	path = NULL;

	// print columns
	if(strcmp(options, "") == 0){
		memset(row, 0, sizeof(char) * ROW_SIZE);
		sprintf(row, "%5s %-8s %8s %s", "PID", "TTY", "TIME", "CMD");
		print_row_adjust_to_size(&max, row);
	}
	else{
		if(strstr(options, "u") != NULL){
			memset(row, 0, sizeof(char) * ROW_SIZE);
			sprintf(row, "%-8s %5s %4s %4s %6s %5s %-8s %-4s %5s   %4s %s", 
					"USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
			print_row_adjust_to_size(&max, row);
		}
		else{
			memset(row, 0, sizeof(char) * ROW_SIZE);
			sprintf(row, "%5s %-8s %-6s %4s %s", "PID", "TTY", "STAT", "TIME", "COMMAND");
			print_row_adjust_to_size(&max, row);
		}
	}

	// read /proc
	read_proc(&proc);

	// read /proc/#
	if((dp = opendir("/proc")) == NULL) exit(0);
	while((entry = readdir(dp)) != NULL){
		if(isdigit(entry->d_name[0])){

			// make path
			path = (char *)malloc(sizeof(char) * (strlen(entry->d_name) + 7));
			memset(path, 0, sizeof(char) * (strlen(entry->d_name) + 7));
			sprintf(path, "/proc/%s", entry->d_name);

			// read /proc/#
			read_pid(path, &pid);
			make_additional_data(&proc, &pid);

			// check options
			if(strcmp(options, "") == 0){
				if(pid.tty_nr == curr_tty_nr){
					memset(row, 0, sizeof(char) * ROW_SIZE);
					if(strlen(pid.comm) > ROW_SIZE - 25) pid.comm[ROW_SIZE-25] = '\0';
					sprintf(row, "%5d %-8s %8s %s", pid.pid, pid.tty, pid.time, pid.comm);
					print_row_adjust_to_size(&max, row);
				}
			}
			else if(strcmp(options, "a") == 0){
				if(pid.tty_nr != 0){
					memset(row, 0, sizeof(char) * ROW_SIZE);
					if(strlen(pid.cmdline) > ROW_SIZE - 28) pid.cmdline[ROW_SIZE-28] = '\0';
					sprintf(row, "%5d %-8s %-6s %4s %s", 
							pid.pid, pid.tty, pid.specific_state, pid.time + 4, pid.cmdline);
					print_row_adjust_to_size(&max, row);
				}
			}
			else if(strcmp(options, "u") == 0){
				if(pid.tty_nr != 0 && pid.uid == curr_uid){
					memset(row, 0, sizeof(char) * ROW_SIZE);
					if(strlen(pid.cmdline) > ROW_SIZE - 66) pid.cmdline[ROW_SIZE-66] = '\0';
					sprintf(row, "%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s   %4s %s",
							pid.user, pid.pid, pid.pcpu, pid.pmem, pid.vsize, pid.rss, pid.tty, pid.specific_state, pid.start, pid.time + 4, pid.cmdline);
					print_row_adjust_to_size(&max, row);
				}
			}
			else if(strcmp(options, "x") == 0){
				if(pid.uid == curr_uid){
					memset(row, 0, sizeof(char) * ROW_SIZE);
					if(strlen(pid.cmdline) > ROW_SIZE - 28) pid.cmdline[ROW_SIZE-28] = '\0';
                                        sprintf(row, "%5d %-8s %-6s %4s %s",
                                                        pid.pid, pid.tty, pid.specific_state, pid.time + 4, pid.cmdline);
                                        print_row_adjust_to_size(&max, row);
				}

			}
			else if(strcmp(options, "au") == 0 || strcmp(options, "ua") == 0){
				if(pid.tty_nr != 0){
					memset(row, 0, sizeof(char) * ROW_SIZE);
					if(strlen(pid.cmdline) > ROW_SIZE - 66) pid.cmdline[ROW_SIZE-66] = '\0';
					sprintf(row, "%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s   %4s %s",
                                                pid.user, pid.pid, pid.pcpu, pid.pmem, pid.vsize, pid.rss, pid.tty, pid.specific_state, pid.start, pid.time + 4, pid.cmdline);
					print_row_adjust_to_size(&max, row);
				}
			}
			else if(strcmp(options, "ax") == 0 || strcmp(options, "xa") == 0){
				memset(row, 0, sizeof(char) * ROW_SIZE);
				if(strlen(pid.cmdline) > ROW_SIZE - 28) pid.cmdline[ROW_SIZE-28] = '\0';
				sprintf(row, "%5d %-8s %-6s %4s %s", pid.pid, pid.tty, pid.specific_state, pid.time + 4, pid.cmdline);
				print_row_adjust_to_size(&max, row);
			}
			else if(strcmp(options, "ux") == 0 || strcmp(options, "xu") == 0){
				if(pid.uid == curr_uid){
					memset(row, 0, sizeof(char) * ROW_SIZE);
					if(strlen(pid.cmdline) > ROW_SIZE - 66) pid.cmdline[ROW_SIZE-66] = '\0';
					sprintf(row, "%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s   %4s %s",
                                                pid.user, pid.pid, pid.pcpu, pid.pmem, pid.vsize, pid.rss, pid.tty, pid.specific_state, pid.start, pid.time + 4, pid.cmdline);
                                        print_row_adjust_to_size(&max, row);
				}
			}
			else if(strstr(options, "a") != NULL && strstr(options, "u") != NULL && strstr(options, "x") != NULL){
				memset(row, 0, sizeof(char) * ROW_SIZE);
				if(strlen(pid.cmdline) > ROW_SIZE - 66) pid.cmdline[ROW_SIZE-66] = '\0';
				sprintf(row, "%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s   %4s %s", 
						pid.user, pid.pid, pid.pcpu, pid.pmem, pid.vsize, pid.rss, pid.tty, pid.specific_state, pid.start, pid.time + 4, pid.cmdline);
				print_row_adjust_to_size(&max, row);
			}

			// free memory
			free_pid_memory(&pid, 0);
			free(path);
			path = NULL;

		}
	}

}

int main(int argc, char **argv){

	if(argc == 1) pps("");
	else{
		if(strstr(argv[1], "a") != NULL || strstr(argv[1], "u") != NULL || strstr(argv[1], "x") != NULL) pps(argv[1]);
		else printf("SSUShell : Incorrect command\n");
	}

}
