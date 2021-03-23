#include "getdata.h"

void read_proc_uptime(struct proc_data *proc){

	int fd;
	char buf[BUF_SIZE];

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open("/proc/uptime", O_RDONLY);
	read(fd, buf, BUF_SIZE-1);

	sscanf(buf, "%lf", &proc->uptime);

	close(fd);

}

void read_proc_loadavg(struct proc_data *proc){

	int fd;
	char buf[BUF_SIZE];

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open("/proc/loadavg", O_RDONLY);
	read(fd, buf, BUF_SIZE-1);

	sscanf(buf, "%lf %lf %lf", &proc->cpu_avg_1, &proc->cpu_avg_5, &proc->cpu_avg_15);

	close(fd);

}

void read_proc_stat(struct proc_data *proc){

	FILE *fp;
	int fd;
	int us, sy, ni, id, wa, hi, si, st;
	double sum = 0.;
	char buf[BUF_SIZE*2];
	char *line, key[15];

	memset(buf, 0, sizeof(char) *(BUF_SIZE * 2));

	fd = open("/proc/stat", O_RDONLY);
	read(fd, buf, BUF_SIZE*2-1);

	line = strtok(buf, "\n");
	sscanf(line, "%s %d %d %d %d %d %d %d %d", key, &us, &sy, &ni, &id, &wa, &hi, &si, &st);

	sum = sum + us + sy + ni + id + wa + hi + si + st;
	proc->us = us / sum * 100;
	proc->sy = sy / sum * 100;
	proc->ni = ni / sum * 100;
	proc->id = id / sum * 100;
	proc->wa = wa / sum * 100;
	proc->hi = hi / sum * 100;
	proc->si = si / sum * 100;
	proc->st = st / sum * 100;

	memset(buf, 0, sizeof(char) * (BUF_SIZE * 2));
	close(fd);

	fp = fopen("/proc/stat", "r");
	while(fgets(buf, BUF_SIZE*2-1, fp)){
		if(strstr(buf, "btime") != NULL){
			sscanf(buf, "%s %lld", key, &proc->btime);
			break;
		}
		memset(buf, 0, sizeof(char) * (BUF_SIZE * 2));
	}
	fclose(fp);

	/*
	while(1){
		line = strtok(NULL, "\n");
		if(strstr(line, "btime") != NULL){
			sscanf(line, "%s %lld", key, &proc->btime); 
			break;
		}
	}

	close(fd);
	*/

}

void read_proc_meminfo(struct proc_data *proc){

	int fd;
	char buf[BUF_SIZE];
	char *line, key[20];

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open("/proc/meminfo", O_RDONLY);
	read(fd, buf, BUF_SIZE-1);

	line = strtok(buf, "\n");
	sscanf(line, "%s %lld", key, &proc->memtotal);

	while(1){
		line = strtok(NULL, "\n");
		if(strstr(line, "MemFree") != NULL) sscanf(line, "%s %lld", key, &proc->memfree);
		if(strstr(line, "MemAvailable") != NULL) sscanf(line, "%s %lld", key, &proc->memavailable);
		if(strstr(line, "Buffers") != NULL) sscanf(line, "%s %lld", key, &proc->buffers);
		if(strstr(line, "Cached") != NULL && strstr(line, "Swap") == NULL) sscanf(line, "%s %lld", key, &proc->cached);
		if(strstr(line, "SwapCached") != NULL) sscanf(line, "%s %lld", key, &proc->swapcached);
		if(strstr(line, "SwapTotal") != NULL) sscanf(line, "%s %lld", key, &proc->swaptotal);
		if(strstr(line, "SwapFree") != NULL) sscanf(line, "%s %lld", key, &proc->swapfree);
		if(strstr(line, "SReclaimable") != NULL){
			sscanf(line, "%s %lld", key, &proc->reclaimable);
			break;
		}
	}

	if(proc->memtotal < proc->memfree + proc->buffers + proc->cached + proc->reclaimable) proc->memused = proc->memtotal - proc->memfree;
	else proc->memused = proc->memtotal - proc->memfree - proc->buffers - proc->cached - proc->reclaimable;

	//proc->memused = proc->memtotal - proc->memfree - proc->buffers - proc->cached;
	proc->swapused = proc->swaptotal - proc->swapfree;
	proc->buffcache = proc->buffers + proc->cached + proc->reclaimable;

	close(fd);

}

void read_pid_stat(char *path, struct pid_data *pid){

	int fd;
	char buf[BUF_SIZE], *stat_path, *comm;
	unsigned int flags;
	unsigned long int minflt, cminflt, majflt, cmajflt;
	long priority, iteralvalue;

	stat_path = (char *)malloc(sizeof(char) * (strlen(path) + 6));
	memset(stat_path, 0, sizeof(char) * (strlen(path) + 6));
	sprintf(stat_path, "%s/stat", path);

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open(stat_path, O_RDONLY);
	read(fd, buf, BUF_SIZE);

	comm = (char *)malloc(sizeof(char) * 300);
	memset(comm, 0, 300);

	sscanf(buf, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %lu",
			&pid->pid, comm, &pid->state, &pid->ppid, &pid->pgrp, &pid->session, 
			&pid->tty_nr, &pid->tpgid, &flags, &minflt, &cminflt, &majflt, &cmajflt, 
			&pid->utime, &pid->stime, &pid->cutime, &pid->cstime, &priority, &pid->nice, 
			&pid->num_threads, &iteralvalue, &pid->starttime, &pid->vsize, &pid->rss);


	pid->priority = (char *)malloc(sizeof(char) * 3);
	memset(pid->priority, 0, sizeof(char) * 3);
	if(priority == -100){
		strncpy(pid->priority, "rt", 2);
	}
	else{
		sprintf(pid->priority, "%ld", priority);
	}

	pid->vsize = pid->vsize / 1024;
	pid->rss = pid->rss * 4;

	free(stat_path);
	free(comm);
	stat_path = NULL;
	comm = NULL;

	close(fd);

}

void read_pid_statm(char *path, struct pid_data *pid){

	int fd;
	char buf[BUF_SIZE], *statm_path;
	int size, resident;

	statm_path = (char *)malloc(sizeof(char) * (strlen(path) + 7));
	memset(statm_path, 0, sizeof(char) * (strlen(path) + 7));
	sprintf(statm_path, "%s/statm", path);

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open(statm_path, O_RDONLY);
	read(fd, buf, BUF_SIZE);

	sscanf(buf, "%d %d %ld", &size, &resident, &pid->shr);
	pid->shr = pid->shr * 4;

	free(statm_path);
	statm_path = NULL;

	close(fd);

}

void read_pid_status(char *path, struct pid_data *pid){

	int fd, flag = 0;
	char buf[BUF_SIZE], *status_path, *line, key[20];
	struct passwd *pw;

	status_path = (char *)malloc(sizeof(char) * (strlen(path) + 8));
	memset(status_path, 0, sizeof(char) * (strlen(path) + 8));
	sprintf(status_path, "%s/status", path);

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open(status_path, O_RDONLY);
	read(fd, buf, BUF_SIZE);

	if(strstr(buf, "VmLck") != NULL) flag = 1;
	else pid->vmlck = 0;

	line = strtok(buf, "\n");
	while(1){
		line = strtok(NULL, "\n");
		if(strstr(line, "Uid") != NULL){
			sscanf(line, "%s %d", key, &pid->uid);
			if(flag == 0) break;
		}
		if(strstr(line, "VmLck") != NULL){
			sscanf(line, "%s %ld", key, &pid->vmlck);
			if(flag == 1) break;
		}
	}

	pid->user = (char *)malloc(sizeof(char) * 9);
	memset(pid->user, 0, sizeof(char) * 9);

	pw = getpwuid(pid->uid);
	strncpy(pid->user, pw->pw_name, 8);
	if(strlen(pw->pw_name) > 8) pid->user[7] = '+';

	free(status_path);
	status_path = NULL;

	close(fd);

}

void read_pid_comm(char *path, struct pid_data *pid){

	int fd;
	char buf[BUF_SIZE], *comm_path, *line;

	comm_path = (char *)malloc(sizeof(char) * (strlen(path) + 6));
	memset(comm_path, 0, sizeof(char) * (strlen(path) + 6));
	sprintf(comm_path, "%s/comm", path);

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open(comm_path, O_RDONLY);
	read(fd, buf, BUF_SIZE-1);

	line = strtok(buf, "\n");

	pid->comm = (char *)malloc(sizeof(char) * (strlen(line) + 1));
	memset(pid->comm, 0, sizeof(char) * (strlen(line) + 1));

	strcpy(pid->comm, buf);

	free(comm_path);
	comm_path = NULL;

	close(fd);

}

void read_pid_cmdline(char *path, struct pid_data *pid){

	int i, fd;
	char buf[BUF_SIZE], *cmdline_path;

	cmdline_path = (char *)malloc(sizeof(char) *(strlen(path) + 9));
	memset(cmdline_path, 0, sizeof(char) * (strlen(path) + 9));
	sprintf(cmdline_path, "%s/cmdline", path);

	memset(buf, 0, sizeof(char) * BUF_SIZE);

	fd = open(cmdline_path, O_RDONLY);
	read(fd, buf, BUF_SIZE-1);

	if(strlen(buf) == 0){
		pid->cmdline = (char *)malloc(sizeof(char) * 18);
		memset(pid->cmdline, 0, sizeof(char) * 18);
		strncpy(buf, pid->comm, 15);
		sprintf(pid->cmdline, "[%s]", buf);
	}
	else{
		// have to check...!
		for(i=0;i<BUF_SIZE-1;i++){
			if(buf[i] == '\0' && buf[i+1] != '\0'){
				buf[i] = ' ';
			}
			if(buf[i] == '\0' && buf[i+1] == '\0') break;
		}
		pid->cmdline = (char *)malloc(sizeof(char) * (strlen(buf) + 1));
		memset(pid->cmdline, 0, sizeof(char) * (strlen(buf) + 1));
		strcpy(pid->cmdline, buf);
	}

	free(cmdline_path);
	cmdline_path = NULL;

	close(fd);

}

void make_tty(struct pid_data *pid){

	int major, minor;

	major = ((unsigned)(pid->tty_nr)>>8u) & 0xfffu;
        minor = ((unsigned)(pid->tty_nr)&0xffu)|(((unsigned)(pid->tty_nr)&0xfff00000u)>>12u);

	pid->tty = (char *)malloc(sizeof(char) * 10);
	memset(pid->tty, 0, sizeof(char) * 10);

	if(major == 0) sprintf(pid->tty, "?");
        else if(major == 4){
                if(minor < 64) sprintf(pid->tty, "tty%d", minor);
                else sprintf(pid->tty, "ttyS%d", minor-64);
        }
        else if(major == 5){
                if(minor == 0) sprintf(pid->tty, "tty");
                else if(minor == 2) sprintf(pid->tty, "pts/ptmx");
                else if(minor == 3) sprintf(pid->tty, "ttyprintk");
        }
        else if(major == 136) sprintf(pid->tty, "pts/%d", minor);

}

void make_specific_state(struct pid_data *pid){

	int idx = 0;

	pid->specific_state = (char *)malloc(sizeof(char) * 7);
	memset(pid->specific_state, 0, sizeof(char) * 7);

	pid->specific_state[idx++] = pid->state;

	if(pid->nice < 0) pid->specific_state[idx++] = '<';
        if(pid->nice > 0) pid->specific_state[idx++] = 'N';
        if(pid->vmlck) pid->specific_state[idx++] = 'L';
        if(pid->session == pid->pid) pid->specific_state[idx++] = 's';
        if(pid->num_threads > 1) pid->specific_state[idx++] = 'l';
        if(pid->pgrp == pid->tpgid) pid->specific_state[idx++] = '+';

}

void make_start(struct proc_data *proc, struct pid_data *pid){

	int Hertz;
	char temp[25], *data;
	time_t time;

	memset(temp, 0, sizeof(char) * 25);

	pid->start = (char *)malloc(sizeof(char) * 6);
	memset(pid->start, 0, sizeof(char) * 6);

	Hertz = sysconf(_SC_CLK_TCK);
	time = proc->btime + pid->starttime / Hertz;

	snprintf(temp, 25, "%24.24s", ctime(&time));

	data = strtok(temp, " ");
	data = strtok(NULL, " ");
	data = strtok(NULL, " ");
	data = strtok(NULL, " ");

	strncpy(pid->start, data, 5);

}

void make_time(struct pid_data *pid){

	int Hertz;
	unsigned int total_time, hour, minute, second, hundred_second;

	pid->time = (char *)malloc(sizeof(char) * 9);
	memset(pid->time, 0, sizeof(char) * 9);

	pid->time_plus = (char *)malloc(sizeof(char) * 8);
	memset(pid->time_plus, 0, sizeof(char) * 8);

	Hertz = sysconf(_SC_CLK_TCK);

        total_time = pid->utime + pid->stime;
	hundred_second = total_time % Hertz;
	total_time = total_time / Hertz;
        second = total_time % 60;
        total_time = total_time / 60;
        minute = total_time % 60;
        total_time = total_time / 60;
        hour = total_time % 24;

        sprintf(pid->time, "%02u:%02u:%02u", hour, minute, second);
	sprintf(pid->time_plus, "%1u:%02u.%02u", minute, second, hundred_second); 

}

void make_pcpu(struct proc_data *proc, struct pid_data *pid){

	int Hertz;
	unsigned long long total_time;
	double	seconds = 0.;

	Hertz = sysconf(_SC_CLK_TCK);
        total_time = pid->utime + pid->stime;

	if((unsigned long long)proc->uptime > pid->starttime / Hertz)
		seconds = ((unsigned long long)proc->uptime - pid->starttime + 0.) / Hertz;
	if(seconds != 0.) pid->pcpu = (total_time * 100ULL / Hertz) / seconds;
	else pid->pcpu = 0.;

}

void make_pmem(struct proc_data *proc, struct pid_data *pid){

	pid->pmem = pid->rss * 100. / proc->memtotal;

}

void read_proc(struct proc_data *proc){

	read_proc_uptime(proc);
	read_proc_loadavg(proc);
	read_proc_stat(proc);
	read_proc_meminfo(proc);

}

void read_pid(char *path, struct pid_data *pid){

	read_pid_stat(path, pid);
	read_pid_statm(path, pid);
	read_pid_status(path, pid);
	read_pid_comm(path, pid);
	read_pid_cmdline(path, pid);

}

void make_additional_data(struct proc_data *proc, struct pid_data *pid){

	make_tty(pid);
	make_specific_state(pid);
	make_start(proc, pid);
	make_time(pid);
	make_pcpu(proc, pid);
	make_pmem(proc, pid);

}

void free_pid_memory(struct pid_data *pid, int flag){

	free(pid->user);
	free(pid->priority);
	free(pid->comm);
	free(pid->cmdline);
	pid->user = NULL;
	pid->priority = NULL;
	pid->comm = NULL;
	pid->cmdline = NULL;

	if(flag == 1){
		free(pid->tty);
		free(pid->specific_state);
		free(pid->start);
		free(pid->time);
		free(pid->time_plus);
		pid->tty = NULL;
		pid->specific_state = NULL;
		pid->start = NULL;
		pid->time = NULL;
		pid->time_plus = NULL;
	}

}

void print_row_adjust_to_size(struct winsize *max, char *row){

	if(strlen(row) <= max->ws_col) printf("%s\n", row);
	else{
		row[max->ws_col] = '\0';
		printf("%s\n", row);
	}

}
