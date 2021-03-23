#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <utmp.h>
#include <pthread.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "getdata.h"

long target_seconds;
int up=0, down=0;

void get_now_time(char *now_time){

	time_t t;
	struct tm tm;

	t = time(NULL);
	tm = *localtime(&t);

	memset(now_time, 0, sizeof(char) * 9);
	sprintf(now_time, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);

}

void get_time_since_boot(struct proc_data *proc, char *time_since_boot){

	unsigned int seconds, minute, hour;

	seconds = (unsigned int)proc->uptime;
	seconds = seconds / 60;
	minute = seconds % 60;
	seconds = seconds / 60;
	hour = seconds % 24;

	memset(time_since_boot, 0, sizeof(char) * 6);
	sprintf(time_since_boot, "%2u:%02u", hour, minute);

}

int get_num_of_users(){

	struct utmp *us;
	int num_of_users=0;

	setutent();
	while((us = getutent())){
		if((us->ut_type == USER_PROCESS) && (us->ut_name[0] != '\0')) num_of_users++;
	}
	endutent();

	return num_of_users;
}

int get_one_char(){

	int ch;
	struct termios old;
	struct termios current;

	tcgetattr(0, &old);
	current = old;
	current.c_lflag &= ~ICANON;
	current.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &current);
	ch = getchar();
	tcsetattr(0, TCSANOW, &old);

	return ch;
}

void display(){

	DIR *dp;
	struct dirent *entry;
	struct winsize max;
	struct proc_data proc;
	struct pid_data **pid, tmp;
	int i=0, cnt=0, prior_cnt=0, flag, cnt_from_start=0;
	int total_cnt, run_cnt, sleep_cnt, stop_cnt, zombie_cnt;
	int prior_list[10];
	char row[ROW_SIZE], path[12];
	char now_time[9], time_since_boot[6];
	time_t t;

	// get terminal size
	ioctl(0, TIOCGWINSZ, &max);

	// clear screen
	printf("\e[1;1H\e[2J");

	// read /proc
	read_proc(&proc);

	//initialize
	total_cnt = 0;
	run_cnt = 0;
	sleep_cnt = 0;
	stop_cnt = 0;
	zombie_cnt = 0;

	if((dp = opendir("/proc")) == NULL) exit(0);
	while((entry = readdir(dp)) != NULL){
		if(isdigit(entry->d_name[0])){
			// make path
			memset(path, 0, sizeof(char) * 12);
			sprintf(path, "/proc/%s", entry->d_name);
			// read /proc/#
			read_pid(path, &tmp);
			total_cnt++;
			switch(tmp.state){
				case 'R':
					run_cnt++;
					break;
				case 'S':
					sleep_cnt++;
					break;
				case 'T':
					stop_cnt++;
					break;
				case 'Z':
					zombie_cnt++;
					break;
				default:
					break;
			}
			//make_additional_data(&proc, &tmp);
			//if(tmp.pcpu >= 0.1) prior_list[prior_cnt++] = atoi(entry->d_name);
		}
	}

	get_now_time(now_time);
	get_time_since_boot(&proc, time_since_boot);

	// display data from /proc
	memset(row, 0, sizeof(char) * ROW_SIZE);
	sprintf(row, "%3s - %8s %2s %5s, %2d %5s,  %12s: %.2f, %.2f, %.2f",
			"top", now_time, "up", time_since_boot, get_num_of_users(), "users", "load average", proc.cpu_avg_1, proc.cpu_avg_5, proc.cpu_avg_15);
	print_row_adjust_to_size(&max, row);

	memset(row, 0, sizeof(char) * ROW_SIZE);
	sprintf(row, "%5s: %3d %s, %3d %s, %3d %s, %3d %s, %3d %s",
			"Tasks", total_cnt, "total", run_cnt, "running", sleep_cnt, "sleeping", stop_cnt, "stopped", zombie_cnt, "zombie");
	print_row_adjust_to_size(&max, row);

	memset(row, 0, sizeof(char) * ROW_SIZE);
	sprintf(row, "%8s %4.1f %3s %4.1f %3s %4.1f %3s %4.1f %3s %4.1f %3s %4.1f %3s %4.1f %3s %4.1f %3s",
			"%Cpu(s):", proc.us, "us,", proc.sy, "sy,", proc.ni, "ni,", proc.id, "id,", proc.wa, "wa,", proc.hi, "hi,", proc.si, "si,", proc.st, "st");
	print_row_adjust_to_size(&max, row);

	memset(row, 0, sizeof(char) * ROW_SIZE);
	sprintf(row, "%9s %8llu %6s %8llu %5s %8llu %5s %8llu %10s",
			"KiB Mem :", proc.memtotal, "total,", proc.memfree, "free,", proc.memused, "used,", proc.buffcache, "buff/cache");
	print_row_adjust_to_size(&max, row);

	memset(row, 0, sizeof(char) * ROW_SIZE);
	sprintf(row, "%9s %8llu %6s %8llu %5s %8llu %5s %8llu %9s",
			"KiB Swap:", proc.swaptotal, "total,", proc.swapfree, "free,", proc.swapused, "used,", proc.memavailable, "avail Mem");
	print_row_adjust_to_size(&max, row);
	printf("\n");

	// display column
	memset(row, 0, sizeof(char) * ROW_SIZE);
	sprintf(row, "%5s %-8s %3s %3s %7s %6s %6s %1s  %4s %4s   %7s %s",
			"PID", "USER", "PR", "NI", "VIRT", "RES", "SHR", "S", "%CPU", "%MEM", "TIME+", "COMMAND");
	print_row_adjust_to_size(&max, row);

	// make struct array
	if(max.ws_row <= 7) return;
	pid = (struct pid_data **)malloc(sizeof(struct pid_data *) * (max.ws_row - 8));
	for(i=0;i<max.ws_row-8;i++) pid[i] = (struct pid_data *)malloc(sizeof(struct pid_data));

	cnt=0;
	cnt_from_start=0;

	/*
	// print prior list
	for(i=0;i<prior_cnt;i++){
	if(cnt >= max.ws_row-7) break;
	memset(path, 0, 12);
	sprintf(path, "/proc/%s", entry->d_name);
	read_pid(path, pid[cnt]);
	make_additional_data(&proc, pid[cnt]);
	memset(row, 0, ROW_SIZE);
	sprintf(row, "%5d %-8s %3s %3ld %7ld %6ld %6ld %c  %4.1f %4.1f   %7s %s",
	pid[cnt]->pid, pid[cnt]->user, pid[cnt]->priority, pid[cnt]->nice, pid[cnt]->vsize, pid[cnt]->rss, pid[cnt]->shr, pid[cnt]->state, pid[cnt]->pcpu, pid[cnt]->pmem, "time+", pid[cnt]->comm);
	if(cnt == max.ws_row-8) print_row_adjust_to_size_(&max, row, 1);
	else print_row_adjust_to_size_(&max, row, 0);
	cnt++;
	}
	*/

	if((dp = opendir("/proc")) == NULL) exit(0);
	while((entry = readdir(dp)) != NULL){
		if(isdigit(entry->d_name[0])){

			// check if is in prior list
			/*
			   flag = 0;
			   for(i=0;i<prior_cnt;i++){
			   if(atoi(entry->d_name) == prior_list[i]){
			   flag = 1;
			   break;
			   }
			   }
			   if(flag == 1) continue;
			   */

			if(down - up <= cnt_from_start){

				// make path
				memset(path, 0, sizeof(char) * 12);
				sprintf(path, "/proc/%s", entry->d_name);

				// read /proc/#
				read_pid(path, pid[cnt]);
				make_additional_data(&proc, pid[cnt]);

				// display
				memset(row, 0, sizeof(char) * ROW_SIZE);
				if(strlen(pid[cnt]->comm) > ROW_SIZE - 69) pid[cnt]->comm[ROW_SIZE-69] = '\0';
				sprintf(row, "%5d %-8s %3s %3ld %7ld %6ld %6ld %c  %4.1f %4.1f   %7s %s",
						pid[cnt]->pid, pid[cnt]->user, pid[cnt]->priority, pid[cnt]->nice, pid[cnt]->vsize, pid[cnt]->rss, pid[cnt]->shr, pid[cnt]->state, pid[cnt]->pcpu, pid[cnt]->pmem, pid[cnt]->time_plus, pid[cnt]->comm);
				print_row_adjust_to_size(&max, row);
				if(cnt == max.ws_row - 9) break;

				cnt++;
			}
			cnt_from_start++;

		}
	}

	//free memory
	for(i=0;i<cnt;i++){
		free_pid_memory(pid[i], 1);
		free(pid[i]);
		pid[i] = NULL;
	}

	t = time(NULL);
	target_seconds = t + 3;

}

void *display_thread(void *arg){

	time_t t;

	while(1){
		display();
		// sleep
		while(1){
			t = time(NULL);
			if(t >= target_seconds) break;
			sleep(0.1);
		}
	}

}

void *key_event_thread(void *arg){

	char ch;

	while(1){
		ch = get_one_char();
		if(ch == 113) exit(0);
		if(ch == 27){
			ch = get_one_char();
			if(ch == 91){
				ch = get_one_char();
				switch(ch){
					case 65:
						up+=1;
						display();
						break;
					case 66:
						down+=1;
						display();
						break;
					default:
						break;
				}
			}
		}
		if(ch == 224){
			ch = get_one_char();
			switch(ch){
				case 72:
					up+=1;
					display();
					break;
				case 80:
					down+=1;
					display();
					break;
				default:
					break;
			}
		}
	}

}

int main(int argc, char *argv[]){

	if(argc == 1){

		pthread_t thread1, thread2;

		if(pthread_create(&thread1, NULL, display_thread, NULL) != 0) exit(1);
		if(pthread_create(&thread2, NULL, key_event_thread, NULL) != 0) exit(1);
		while(1);
	}
	else printf("SSUShell : Incorrect command\n");

}
