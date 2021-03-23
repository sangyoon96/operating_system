//#ifndef MAKEDATA_H
//#define MAKEDATA_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define BUF_SIZE 1024 
#define ROW_SIZE 512 

struct proc_data{

	// from /proc/uptime
	double uptime;

	// from /proc/loadavg
	double cpu_avg_1;
	double cpu_avg_5;
	double cpu_avg_15;

	// from /proc/stat
	double us;
        double sy;
        double ni;
        double id;
        double wa;
        double hi;
        double si;
        double st;

	// type checking!!!
	unsigned long long btime;

	// from /proc/meminfo
	unsigned long long memtotal;
        unsigned long long memfree;
        unsigned long long memavailable;
        unsigned long long buffers;
        unsigned long long cached;
        unsigned long long swapcached;
        unsigned long long swaptotal;
        unsigned long long swapfree;
	unsigned long long reclaimable;

	// have to calculate
	unsigned long long memused;
	unsigned long long swapused;
	unsigned long long buffcache;

};

struct pid_data{

	// how to get this value?
	char *user;

	// from /proc/#/stat
	int pid;
        char state;
        int ppid;
        int pgrp;
        int session;
        int tty_nr;
        int tpgid;
        unsigned long int utime;
        unsigned long int stime;
        long cutime;
        long cstime;
	char *priority;
        long nice;
        long num_threads;
        unsigned long long starttime;
        unsigned long vsize; // have to convert!!!
        unsigned long rss; // have to convert!!!

	// from /proc/#/statm
	unsigned long shr;

	// from /proc/#/status
	int uid;
	unsigned long vmlck;

	// from /proc/#/comm
	char *comm;

	//from /proc/#/cmdline
	char *cmdline;

	// have to convert to get these value
	char *tty;
	char *specific_state;
	char *start;
	char *time;
	char *time_plus;
	double pcpu;
	double pmem;

};

void read_proc_uptime(struct proc_data *proc);
void read_proc_loadavg(struct proc_data *proc);
void read_proc_stat(struct proc_data *proc);
void read_proc_meminfo(struct proc_data *proc);

void read_pid_stat(char *path, struct pid_data *pid);
void read_pid_statm(char *path, struct pid_data *pid);
void read_pid_status(char *path, struct pid_data *pid);
void read_pid_comm(char *path, struct pid_data *pid);
void read_pid_cmdline(char *path, struct pid_data *pid);

void make_tty(struct pid_data *pid);
void make_specific_state(struct pid_data *pid);
void make_start(struct proc_data *proc, struct pid_data *pid);
void make_time(struct pid_data *pid);
void make_pcpu(struct proc_data *proc, struct pid_data *pid);
void make_pmem(struct proc_data *proc, struct pid_data *pid);

void read_proc(struct proc_data *proc);
void read_pid(char *path, struct pid_data *pid);
void make_additional_data(struct proc_data *proc, struct pid_data *pid);
void free_pid_memory(struct pid_data *pid, int flag);

void print_row_adjust_to_size(struct winsize *max, char *row);
