
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Xiao hong, 2016
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start. 
 * 
 *****************************************************************************/
char location[128] = "/";
char filepath[128] = "";
char users[2][128] = {"empty", "empty"};
char passwords[10][128];
char files[20][128];
char userfiles[20][128];
int filequeue[50];
int filecount = 0;
int usercount = 0;
int leiflag = 0;

PUBLIC int kernel_main()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	struct task * t;
	struct proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

	        if (i < NR_TASKS) {     /* TASK */
                        t	= task_table + i;
                        priv	= PRIVILEGE_TASK;
                        rpl     = RPL_TASK;
                        eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = 15;
                }
                else {                  /* USER PROC */
                        t	= user_proc_table + (i - NR_TASKS);
                        priv	= PRIVILEGE_USER;
                        rpl     = RPL_USER;
                        eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = 5;
                }

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->ticks = p->priority = prio;

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s' ", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);

	while (1) {
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0)
			break;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]");
			return;
		}
		printf("    %s (%d bytes) ", phdr->name, f_len);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	close(fd);

	printf(" done]\n");
}



/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	//printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");
			

	char * tty_list[] = {"/dev_tty0"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
		}
		else {	/* child process */
			close(fd_stdin);
			close(fd_stdout);
			
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	while(1)
	{
		//printf("%d", TESTA());
		//int n = TESTA();
		//milli_delay(200);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	for(;;);
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

/*****************************************************************************
 *                                xiaohong_shell
 *****************************************************************************/
/**
 * A very very powerful shell.
 * 
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shabby_shell(const char * tty_name)
{

	
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char cmd[128];
    	char arg1[128];
    	char arg2[128];
    	char buf[1024];
	int j = 0;

	colorful();
	clear();
	animation();
	welcome();
	printf("press any key to start:\n");
	int r = read(0, rdbuf, 70);
	initFs();

	while (1) {

		clearArr(rdbuf, 128);
        	clearArr(cmd, 128);
        	clearArr(arg1, 128);
        	clearArr(arg2, 128);
        	clearArr(buf, 1024);
		
		printf("%s $ ", location);
		int r = read(0, rdbuf, 70);
		rdbuf[r] = 0;
	

		int argc = 0;
		char * argv[PROC_ORIGIN_STACK];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		} while(ch);
		argv[argc] = 0;

		int fd = open(argv[0], O_RDWR);
		if (fd == -1) {
			if (rdbuf[0]) {
				int i = 0, j = 0;
				/* get command */
				while (rdbuf[i] != ' ' && rdbuf[i] != 0)
				{
					cmd[i] = rdbuf[i];
					i++;
				}
				i++;
				/* get arg1 */
				while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        			{
            				arg1[j] = rdbuf[i];
            				i++;
            				j++;
        			}
        			i++;
        			j = 0;
				/* get arg2 */
       				while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        			{
            				arg2[j] = rdbuf[i];
            				i++;
            				j++;
        			}
				/* play video */
				if(strcmp(cmd, "animation") == 0)
				{
					animation();
					welcome();
				}
				/* welcome */
				else if(strcmp(cmd, "welcome") == 0)
				{
					welcome();
				}
				/* clear screen */
				else if(strcmp(cmd, "clear") == 0)
				{
					clear();		
					welcome();
				}
				/* show process */
				else if(strcmp(cmd, "proc") == 0)
				{
					showProcess();
				}
				/* show help message */
				else if(strcmp(cmd, "help") == 0)
				{
					help();
				}
				/* create a file */
				else if(strcmp(cmd, "newfile") == 0)
				{
					createFilepath(arg1);
					createFile(filepath, arg2, 1);
					clearArr(filepath, 128);
				}
				/* read a file */
				else if(strcmp(cmd, "read") == 0)
				{
					createFilepath(arg1);
					readFile(filepath);
					clearArr(filepath, 128);
					//readFile(arg1);	
				}
				/* edit a file appand */
				else if(strcmp(cmd, "edit+") == 0)
				{	
					createFilepath(arg1);
					editAppand(filepath, arg2);
					clearArr(filepath, 128);
				}
				/* edit a file cover */
				else if(strcmp(cmd, "edit") == 0)
				{
					createFilepath(arg1);
					editCover(filepath, arg2);
					clearArr(filepath, 128);
				}
				/* delete a file */
				else if(strcmp(cmd, "delete") == 0)
				{
					createFilepath(arg1);
					deleteFile(filepath);
					clearArr(filepath, 128);
				}
				/* login */
				else if(strcmp(cmd, "login") == 0)
				{
					login(arg1, arg2);
				}
				/* login out */
				else if(strcmp(cmd, "loginout") == 0)
				{
					loginOut();
				}
				/* ls */
				else if(strcmp(cmd, "ls") == 0)
				{
					ls();
				}
				/* pwd */
				else if(strcmp(cmd, "user") == 0)
				{
					pwd();
				}
				/* add user */
				else if(strcmp(cmd, "add") == 0)
				{
					addUser(arg1, arg2);
				}
				/* move user */
				else if(strcmp(cmd, "move") == 0)
				{
					moveUser(arg1, arg2);
				}
				/* message */
				else if(strcmp(cmd, "duihua") == 0)
				{
					TESTA(arg1);
				}
			}
		}
		else {
			close(fd);
			int pid = fork();
			if (pid != 0) { /* parent */
				int s;
				wait(&s);
			}
			else {	/* child */
				execv(argv[0], argv);
			}
		}
	}

	close(1);
	close(0);
}

/* Tools */

	/* Init Arr */
void clearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

	/* Create Filepath */
void createFilepath(char * filename)
{
	int k = 0, j = 0;
		
	for (k = 0; k < strlen(location); k++)
	{
		filepath[k] = location[k];
	}
	filepath[k] = '_';
	k++;
	for(j = 0; j < strlen(filename); j++, k++)
	{	
		filepath[k] = filename[j];
	}
	filepath[k] = '\0';
}

	/* Update FileLogs */
void updateFileLogs()
{
	int i = 0, count = 0;
	editCover("fileLogs", "");
	while (count <= filecount - 1)
	{
		if (filequeue[i] == 0)
		{
			i++;
			continue;
		}
		char filename[128];
		int len = strlen(files[count]);
		strcpy(filename, files[count]);
		filename[len] = ' ';
		filename[len + 1] = '\0';
		printf("%s\n", filename);
		editAppand("fileLogs", filename);
		count++;
		i++;
	}
}

	/* Update myUsers */
void updateMyUsers()
{
	int i = 0, count = 0;
	editCover("myUsers", "");
	if (strcmp(users[0], "empty") != 0)
	{
		editAppand("myUsers", users[0]);
		editAppand("myUsers", " ");
	}
	else
	{
		editAppand("myUsers", "empty ");
	}
	if (strcmp(users[1], "empty") != 0)
	{
		editAppand("myUsers", users[1]);
		editAppand("myUsers", " ");
	}
	else
	{
		editAppand("myUsers", "empty ");
	}
}

	/* Update myUsersPassword */
void updateMyUsersPassword()
{
	int i = 0, count = 0;
	editCover("myUsersPassword", "");
	if (strcmp(passwords[0], "") != 0)
	{
		editAppand("myUsersPassword", passwords[0]);
		editAppand("myUsersPassword", " ");
	}
	else
	{
		editAppand("myUsersPassword", "empty ");
	}
	if (strcmp(passwords[1], "") != 0)
	{
		editAppand("myUsersPassword", passwords[1]);
		editAppand("myUsersPassword", " ");
	}
	else
	{
		editAppand("myUsersPassword", "empty ");
	}
}

	/* Add FIle Log */
void addLog(char * filepath)
{
	int pos = -1, i = 0;
	pos = getPos();
	filecount++;
	strcpy(files[pos], filepath);
	updateFileLogs();
	filequeue[pos] = 0;
	if (strcmp("/", location) != 0)
	{
		int fd = -1, k = 0, j = 0;
		char filename[128];
		while (k < strlen(filepath))
		{
			if (filepath[k] != '_')
				k++;
			else
				break;
		}
		k++;
		while (k < strlen(filepath))
		{
			filename[j] = filepath[k];
			k++;
			j++;
		}
		filename[j] = '\0';
		if (strcmp(location, users[0]) == 0)
		{
			editAppand("user1", filename);
			editAppand("user1", " ");
		}
		else if(strcmp(location, users[1]) == 0)
		{
			editAppand("user2", filename);
			editAppand("user2", " ");
		}
	}
}

	/* Delete File Log */
void deleteLog(char * filepath)
{
	int i = 0, fd = -1;
	for (i = 0; i < filecount; i++)
	{
		if (strcmp(filepath, files[i]) == 0)
		{
			strcpy(files[i], "empty");
			int len = strlen(files[i]);
			files[i][len] = '0' + i;
			files[i][len + 1] = '\0';
			fd = open(files[i], O_CREAT | O_RDWR);
			close(fd);
			filequeue[i] = 1;
			break;
		}
	}
	filecount--;
	updateFileLogs();
}

	/* Get File Pos */
int getPos()
{
	int i = 0;
	for (i = 0; i < 500; i++)
	{
		if (filequeue[i] == 1)
			return i;
	}
}

	/* For vertify */
int vertify()
{
	if (strcmp(location, "/") == 0)
	{
		printf("Permission deny!!\n");
		return 0;
	}
	else
		return 1;
}

/* Init FS */
void initFs()
{
	int fd = -1, n = 0, i = 0, count = 0, k = 0;
	char bufr[1024] = "";
	char bufp[1024] = "";
	char buff[1024] = "";

	for (i = 0; i < 500; i++)
		filequeue[i] = 1;

	fd = open("myUsers", O_CREAT | O_RDWR);
	close(fd);
	fd = open("myUsersPassword", O_CREAT | O_RDWR);
	close(fd);
	fd = open("fileLogs", O_CREAT | O_RDWR);
	close(fd);
	fd = open("user1", O_CREAT | O_RDWR);
	close(fd);
	fd = open("user2", O_CREAT | O_RDWR);
	close(fd);
	/* init users */
	fd = open("myUsers", O_RDWR);
	n = read(fd, bufr, 1024);
	bufr[strlen(bufr)] = '\0';
	for (i = 0; i < strlen(bufr); i++)
	{
		if (bufr[i] != ' ')
		{
			users[count][k] = bufr[i];
			k++;
		}
		else
		{
			while (bufr[i] == ' ')
			{
				i++;
				if (bufr[i] == '\0')
				{
					users[count][k] = '\0';
					if (strcmp(users[count], "empty") != 0)
						usercount++;
					count++;
					break;
				}
			}
			if (bufr[i] == '\0')
			{
				break;
			}
			i--;
			users[count][k] = '\0';
			if (strcmp(users[count], "empty") != 0)
						usercount++;
			k = 0;
			count++;
		}
	}
	close(fd);
	count = 0;
	k = 0;
	
	/* init password */
	fd = open("myUsersPassword", O_RDWR);
	n = read(fd, bufp, 1024);
	for (i = 0; i < strlen(bufp); i++)
	{
		if (bufp[i] != ' ')
		{
			passwords[count][k] = bufp[i];
			k++;
		}
		else
		{
			while (bufp[i] == ' ')
			{
				i++;
				if (bufp[i] == '\0')
				{
					count++;
					break;
				}
			}
			if (bufp[i] == '\0')
				break;
			i--;
			passwords[count][k] = '\0';
			k = 0;
			count++;
		}
	}
	close(fd);
	count = 0;
	k = 0;

	/* init files */
	fd = open("fileLogs", O_RDWR);
	n = read(fd, buff, 1024);
;	for (i = 0; i <= strlen(buff); i++)
	{
		if (buff[i] != ' ')
		{
			files[count][k] = buff[i];
			k++;
		}
		else
		{
			while (buff[i] == ' ')
			{
				i++;
				if (buff[i] == '\0')
				{
					break;
				}
			}
			if (buff[i] == '\0')
			{
				files[count][k] = '\0';
				count++;
				break;
			}
			i--;
			files[count][k] = '\0';
			k = 0;
			count++;
		}
	}
	close(fd);
	
	int empty = 0;
	for (i = 0; i < count; i++)
	{
		char flag[7];
		strcpy(flag, "empty");
		flag[5] = '0' + i;
		flag[6] = '\0';
		fd = open(files[i], O_CREAT | O_RDWR);
		close(fd);
	
		if (strcmp(files[i], flag) != 0)
			filequeue[i] = 0;
		else
			empty++;
	}
	filecount = count - empty;
}


/* Welcome */
PUBLIC void welcome()
{

	printf("              ******************************************************\n");
	printf("              *                                                    *\n");
	printf("              *        Welcome to Xiao Hong's Operating System     *\n");
	printf("              *                                                    *\n");
	printf("              ******************************************************\n");
	printf("              *                                                    *\n");
	printf("              *                                                    *\n");
	printf("              *                1452822 Hong Jiayong                *\n");
	printf("              *                1454093   Xia Chen                  *\n");
	printf("              *                1452690   Zhao Xu                   *\n");
	printf("              *                                                    *\n");
	printf("              *                                                    *\n");
	printf("              ******************************************************\n\n");
}

/* Clear */
PUBLIC void clear()
{
	int i = 0;
	for (i = 0; i < 20; i++)
		printf("\n");
}

/* Show Process */
void showProcess()
{	int i = 0;
	printf("********************************************************************************\n");
	printf("        name        |        priority        |        f_flags(0 is runable)        \n");
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	for (i = 0; i < NR_TASKS + NR_PROCS; i++)
	{
		printf("        %s                   %d                      %d\n", proc_table[i].name, proc_table[i].priority, proc_table[i].p_flags);
	}
	printf("********************************************************************************\n");
}

/* Show Help Message */
void help()
{
	printf("********************************************************************************\n");
	printf("        name               |                      function                      \n");
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("        welcome            |           Welcome the users\n");
	printf("        clear              |           Clean the screen\n");
	printf("        animation          |           Play the video\n");
	printf("        ls                 |           List all files in current file path\n");
	printf("        help               |           List all commands\n");
	printf("        proc               |           List all process's message\n");
	printf("        saolei             |           Start game saolei\n");
	printf("        2048               |           Start game 2048\n");
	printf("        caculator          |           Start a simply caculator\n");
	printf("        duihua [str]       |           Chat with Os\n");
	printf("        print  [str]       |           Print a string\n");
	printf("        newfile[file][str] |           Create a file\n");
	printf("        read   [file]      |           Read a file\n");
	printf("        delete [file]      |           Delete a file\n");
	printf("        edit   [file][str] |           Edit file, cover the content\n");
	printf("        edit+  [file][str] |           Edit file, appand after the content\n");
	printf("        add    [user][pass]|           Create a new user\n");
	printf("        move   [user][pass]|           Remove a user and delete his files\n");
	printf("        login  [user][pass]|           Login \n");
	printf("        loginout           |           Loginout\n");
	printf("********************************************************************************\n");
	
}

/* Create File */
void createFile(char * filepath, char * buf, int flag)
{
	if (vertify() == 0)
		return;

	int fd = -1, i = 0, pos;
	
	pos = getPos();
	char f[7];
	strcpy(f, "empty");
	f[5] = '0' + pos;
	f[6] = '\0';
	if (strcmp(files[pos], f) == 0 && flag == 1)
	{
		unlink(files[pos]);
	}

	fd = open(filepath, O_CREAT | O_RDWR);
	printf("file name: %s content: %s\n", filepath, buf);
	if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	if(fd == -2)
	{
		printf("Fail, file exsists!!\n");
		return;
	}
	printf("%s\n", buf);
	
	write(fd, buf, strlen(buf));
	close(fd);
	
	/* add log */
	if (flag == 1)
		addLog(filepath);
		
}


/* Read File */
void readFile(char * filepath)
{
	if (vertify() == 0)
		return;

	int fd = -1;
	int n;
	char bufr[1024] = "";
	fd = open(filepath, O_RDWR);
	if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	n = read(fd, bufr, 1024);
	bufr[n] = '\0';
	printf("%s(fd=%d) : %s\n", filepath, fd, bufr);
	close(fd);
}

/* Edit File Appand */
void editAppand(char * filepath, char * buf)
{
	if (vertify() == 0)
		return;

	int fd = -1;
	int n, i = 0;
	char bufr[1024] = "";
	char empty[1024];
	
	for (i = 0; i < 1024; i++)
		empty[i] = '\0';
	fd = open(filepath, O_RDWR);
	if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}

	n = read(fd, bufr, 1024);
	n = strlen(bufr);
	
	for (i = 0; i < strlen(buf); i++, n++)
	{	
		bufr[n] = buf[i];
		bufr[n + 1] = '\0';
	}
	write(fd, empty, 1024);
	fd = open(filepath, O_RDWR);
	write(fd, bufr, strlen(bufr));
	close(fd);
}

/* Edit File Cover */
void editCover(char * filepath, char * buf)
{
	
	if (vertify() == 0)
		return;

	int fd = -1;
	int n, i = 0;
	char bufr[1024] = "";
	char empty[1024];
	
	for (i = 0; i < 1024; i++)
		empty[i] = '\0';

	fd = open(filepath, O_RDWR);
	if (fd == -1)
		return;
	write(fd, empty, 1024);
	close(fd);
	fd = open(filepath, O_RDWR);
	write(fd, buf, strlen(buf));
	close(fd);
}

/* Delete File */
void deleteFile(char * filepath)
{
	if (vertify() == 0)
		return;
	if (usercount == 0)
	{
		printf("Fail!\n");
		return;
	}
	editCover(filepath, "");
	if(unlink(filepath) != 0)
	{
		printf("Edit fail, please try again!\n");
		return;
	}
	deleteLog(filepath);
	
	char username[128];
	if (strcmp(location, users[0]) == 0)
	{
		strcpy(username, "user1");
	}
	if (strcmp(location, users[1]) == 0)
	{
		strcpy(username, "user2");
	}

	char userfiles[20][128];
	char bufr[1024];
	char filename[128];
	char realname[128];
	int fd = -1, n = 0, i = 0, count = 0, k = 0;
	fd = open(username, O_RDWR);
	n = read(fd, bufr, 1024);
	close(fd);
	
	for (i = strlen(location) + 1; i < strlen(filepath); i++, k++)
	{
		realname[k] = filepath[i];
	}	
	realname[k] = '\0';
	k = 0;
	for (i = 0; i < strlen(bufr); i++)
	{
		if (bufr[i] != ' ')
		{
			filename[k] = bufr[i];
			k++;
		}
		else
		{
			filename[k] = '\0';
			if (strcmp(filename, realname) == 0)
			{
				k = 0;
				continue;
			}
			strcpy(userfiles[count], filename);
			count++;
			k = 0;
		}
	}
	
	i = 0, k = 0;
	for (k = 0; k < 2; k++)
	{
		printf("%s\n", userfiles[k]);
	}
	editCover(username, "");
	while (i < count)
	{
		if (strlen(userfiles[i]) < 1)
		{
			i++;
			continue;
		}
		char user[128];
		int len = strlen(userfiles[i]);
		strcpy(user, userfiles[i]);
		user[len] = ' ';
		user[len + 1] = '\0';
		editAppand(username, user);
		i++;
	}
}

/* Login */
void login(char * username, char * password)
{
	int i = 0;
	for (i = 0; i < usercount; i++)
	{
		if (strcmp(username, users[i]) == 0 && strcmp(password, passwords[i]) == 0)
		{
			strcpy(location, users[i]);
			printf("Welcome! %s!\n", users[i]);
			return;
		}
	}
	printf("Sorry! No such user!\n");
}

/* Login Out */
void loginOut()
{
	printf("Good bye! %s\n", location);
	strcpy(location, "/");
}

/* Ls */
void ls()
{
	int fd = -1, n;
	char bufr[1024];
	if (strcmp(location, users[0]) == 0)
	{
		fd = open("user1", O_RDWR);
		if (fd == -1)
		{
			printf("empty\n");
		}
		n = read(fd, bufr, 1024);
		printf("%s\n", bufr);
		close(fd);
	}
	else if(strcmp(location, users[1]) == 0)
	{
		fd = open("user2", O_RDWR);
		if (fd == -1)
		{
			printf("empty\n");
		}
		n = read(fd, bufr, 1024);
		printf("%s\n", bufr);
		close(fd);
	}
	else
		printf("Permission deny!\n");
}


/* Add User */
void addUser(char * username, char * password)
{
	printf("Please input admin password:");
	char buf[128];
	int r = read(0, buf, 128);
	int i = 0;
	strcpy(location, "admin");
	//printf("%s %s\n", users[0], users[1]);
	for (i = 0; i < 2; i++)
	{
		if (strcmp(users[i], username) == 0)
		{
			printf("User exists!\n");
			return;
		}
	}
	if (usercount == 2)
	{
		printf("No more users\n");
		loginOut();
		return;
	}
	if (strcmp(users[0], "empty") == 0)
	{
		strcpy(users[0], username);
		strcpy(passwords[0], password);
		usercount++;
		updateMyUsers();
		updateMyUsersPassword();
		loginOut();
		return;
	}
	if (strcmp(users[1], "empty") == 0)
	{
		strcpy(users[1], username);
		strcpy(passwords[1], password);
		usercount++;
		updateMyUsers();
		updateMyUsersPassword();
		loginOut();
		return;
	}

}

/* Move User */
void moveUser(char * username, char * password)
{
	printf("Please input admin password:");
	char buf[128];
	int r = read(0, buf, 128);
	int i = 0;
	for (i = 0; i < 2; i++)
	{
		if (strcmp(username, users[i]) == 0 && strcmp(password, passwords[i]) == 0)
		{
			strcpy(location, username);
			
			int fd = -1, n = 0, k = 0, count = 0;
			char bufr[1024], deletefile[128];
			if (i == 0)
			{
				fd = open("user1", O_RDWR);
			}
			if (i == 1)
			{
				fd = open("user2", O_RDWR);
			}
			n = read(fd, bufr, 1024);
			close(fd);
			for (k = 0; k < strlen(bufr); k++)
			{
				if (bufr[k] != ' ')
				{
					deletefile[count] = bufr[k];
					count++;
				}
				else
				{
					deletefile[count] = '\0';
					createFilepath(deletefile);
					deleteFile(filepath);
					count = 0;
				}
			}
		
			printf("Delete %s!\n", users[i]);
			strcpy(users[i], "empty");
			strcpy(passwords[i], "");
			updateMyUsers();
			updateMyUsersPassword();
			usercount--;
			strcpy(location, "/");
			return;
		}
	}
	printf("Sorry! No such user!\n");
}

/* Colorful */
void colorful()
{	
	int j = 0;
	for (j = 0; j < 3200; j++){disp_color_str("S", BLACK);}
	for (j = 0; j < 3; j++)
		disp_color_str("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS", GREEN);
	/* first line */
	disp_color_str("SSSSSSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSSSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSSSSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSSSSSS", BLUE);
	/* second line */
	disp_color_str("SSSSSSSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSSS", BLUE);
	/* third line */
	disp_color_str("SSSSSSSSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("S", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSSSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSSSSSSSSS", BLUE);
	/* forth line */
	disp_color_str("SSSSSSSSSSSS", BLUE);
	disp_color_str("AAAAA", RED);
	disp_color_str("SSSSSSSSSSS", BLUE);
	disp_color_str("HHHHHHHHHH", WHITE);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSSSSSS", BLUE);
	/* fifth line */
	disp_color_str("SSSSSSSSSSSS", BLUE);
	disp_color_str("AAAAA", RED);
	disp_color_str("SSSSSSSSSSS", BLUE);
	disp_color_str("HHHHHHHHHH", WHITE);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSSSS", BLUE);
	/* sixth line */
	disp_color_str("SSSSSSSSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("S", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSSSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSSSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSS", BLUE);
	/* seventh line */
	disp_color_str("SSSSSSSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSSS", BLUE);
	/* eighth line */
	disp_color_str("SSSSSSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSS", BLUE);
	disp_color_str("AAA", RED);
	disp_color_str("SSSSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSS", BLUE);
	disp_color_str("HH", WHITE);
	disp_color_str("SSSSSSSSSSSSS", BLUE);
	disp_color_str("OO",BLACK);
	disp_color_str("SSSSSSSSSSSSSSS", BLUE);
	disp_color_str("CC", RED);
	disp_color_str("SSSSSSSSSS", BLUE);
	for (j = 0; j < 3; j++)
		disp_color_str("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS", GREEN);
	for (j = 0; j < 300; j++)
		disp_color_str(" ",BLACK);
	milli_delay(8000);
}

PUBLIC int TESTA(char * topic)
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = XIA;
	strcpy(msg.content, topic);
	printf("%s\n", msg.content);
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

void pwd(){}
