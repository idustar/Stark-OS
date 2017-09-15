
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
char location[128] = "";
char filepath[128] = "";
char users[10][128] = {"noname","noname","noname","noname","noname","noname","noname","noname","noname","noname"};
char passwords[10][128] = {"no","no","no","no","no","no","no","no","no","no"};
int permissions[10] = {2,1,1,1,1,1,1,1,1,1};
char files[60][128];
char userfiles[60][128];
int filequeue[200];
int filecount = 0;
int usercount = 0;
int leiflag = 0;
int currentUser = -1;
int userFlag = -1;
int currentState = 0;   // 0 = normal; 1 = require username; 2 = require password; 3 = edit files

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
 TestC
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
    turnOn();
    clear();
    //animation();
    welcome();
//    printf("press any key to start:\n");
//    int r = read(0, rdbuf, 70);
    initFs();
    login();

    while (1) {

        clearArr(rdbuf, 128);
        clearArr(cmd, 128);
        clearArr(arg1, 128);
        clearArr(arg2, 128);
        clearArr(buf, 1024);

        if (currentState == 0) {
            if (currentUser >= 0) {
                printf ("%s@", users[currentUser]);
            }
            printf("%s $ ", location);
        }
        else
            printf(">> ");
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
        if (currentState == 0) {
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
                    else if(strcmp(cmd, "mk") == 0)
                    {
                        createFilepath(arg1);
                        createFile(filepath, arg2, 1);
                        clearArr(filepath, 128);
                    }
                    /* make directory */
                    else if(strcmp(cmd, "mkdir") == 0)
                    {
                        createFilepath(arg1);
                        mkdir(filepath);
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
                    /* notepad */
                    else if(strcmp(cmd, "vi") == 0)
                    {
                        createFilepath(arg1);
                        notepad(filepath);
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
                    /* remove a file */
                    else if(strcmp(cmd, "rm") == 0)
                    {
                        createFilepath(arg1);
                        deleteFile(filepath);
                        clearArr(filepath, 128);
                    }
                    /* remove a directory */
                    else if(strcmp(cmd, "rmdir") == 0)
                    {
                        createFilepath(arg1);
                        rmdir(filepath);
                        clearArr(filepath, 128);
                    }
                    /* login */
                    else if(strcmp(cmd, "login") == 0)
                    {
                        login();
                    }
                    /* login out */
                    else if(strcmp(cmd, "logout") == 0)
                    {
                        logout();
                    }
                    /* ls */
                    else if(strcmp(cmd, "ls") == 0)
                    {
                        ls();
                    }
                    /* cd */
                    else if(strcmp(cmd, "cd") == 0)
                    {
                        cd(arg1);
                        clearArr(filepath, 128);
                    }
                    /* pwd */
                    else if(strcmp(cmd, "user") == 0)
                    {
                        pwd();
                    }
                    /* add user */
                    else if(strcmp(cmd, "adduser") == 0)
                    {
                        addUser(arg1, arg2, 0, 1);
                    }
                    /* move user */
                    else if(strcmp(cmd, "rmuser") == 0)
                    {
                        removeUser(arg1, arg2);
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
        } else if (currentState == 1) {
            checkUsername(argv[0]);
        } else if (currentState == 2) {
            checkPassword(argv[0]);
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

/* Match file path and name */
char * getFilePath(char * filename)
{
    int j = 0, k = -1, i = 0;
    char filepath[128];
    clearArr(filepath, 128);
    for (j = strlen(filename) - 2; j >= 0 ; j--)
    {
        if (filename[j] == '_') {
            k = j;
            break;
        }
    }
    filepath[0] = '\0';
    if (k <= 0)
        return "_";
    if (filename[0] == '_')
        i = 1;
    for (j = 0; j < k; j++, i++)
    {
        filepath[j] = filename[i];
    }
    if (filepath[k-1] == '_') filepath[k-1] = '\0';
    filepath[k] = '_';
    filepath[k+1] = '\0';
    return filepath;
}

char * getFileName(char * filename)
{
    int j = 0, k = -1, i = 0;
    char truename[128];
    clearArr(truename, 128);
    for (j = strlen(filename) - 2; j >= 0 ; j--)
    {
        if (filename[j] == '_') {
            k = j;
            break;
        }
    }
    truename[0] = '\0';
    if (filename[k + 1] == '_')
    {
        k = k + 1;
    }
    for (i = 0, j = k + 1; j < strlen(filename); j++, i++)
    {
        truename[i] = filename[j];
    }
    truename[i + 1] = '\0';
    return truename;
}
char * getTrueName(char * name)
{
    int j = 0, k = -1, i = 0;
    char tn[128];
    clearArr(tn, 128);
    for (j = strlen(name) - 2; j >= 0 ; j--)
    {
        if (name[j] == '_') {
            k = j;
            break;
        }
    }
    tn[0] = '\0';
    if (name[k + 1] == '_')
    {
        k = k + 1;
    }
    for (i = 0, j = k + 1; j < strlen(name); j++, i++)
    {
        tn[i] = name[j];
    }
    tn[i + 1] = '\0';
    printf("gettruename:%s\n", tn);
    return tn;
}

/* Combine File Path And File Name */
void createFilepath(char * filename)
{
    int k = 0, j = 0;
    for (k = 0; k < strlen(location); k++)
    {
        filepath[k] = location[k];
    }
    if (k != 0) {
        filepath[k] = '_';
        k++;
    }
    for(j = 0; j < strlen(filename); j++, k++)
    {
        filepath[k] = filename[j];
    }
    filepath[k] = '\0';
}

/* Update File Logs */
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
    for (i = 0; i < usercount; i++) {
        editAppand("myUsers", users[i]);
        editAppand("myUsers", " ");
    }
}

/* Update myUsersPassword */
void updateMyUsersPassword()
{
    int i = 0, count = 0;
    editCover("myUsersPassword", "");
    for (i = 0; i < usercount; i++) {
        editAppand("myUsersPassword", passwords[i]);
        editAppand("myUsersPassword", " ");
    }
}

/* Update myUsersPermissions */
void updateMyUsersPermissions()
{
    int i = 0, count = 0;
    editCover("myUsersPermissions", "");
    for (i = 0; i < usercount; i++) {
        editAppand("myUsersPermissions", permissions[i]);
    }
}


/* Add File Log */
void addLog(char * filepath)
{
    int pos = -1, i = 0;
    pos = getPos();
    filecount++;
    while (filepath[i] && filepath[i] >= '\31') {
        files[pos][i] = filepath[i];
        i++;
    }
    files[pos][i] = '\0';
    printf("new file: '%s' has been added into filelogs.\n", files[pos]);
    updateFileLogs();
    filequeue[pos] = 0;
    char fname[128];
    clearArr(fname, 128);
    char fpath[128];
    clearArr(fpath, 128);
    //
    i = 0;
    int j = 0, k = -1;
    for (j = strlen(filepath) - 2; j >= 0 ; j--)
    {
        if (filepath[j] == '_') {
            k = j;
            break;
        }
    }
    fname[0] = '\0';
    for (i = 0, j = k + 1; j < strlen(filepath); j++, i++)
    {
        fname[i] = filepath[j];
    }
    fname[i + 1] = '\0';
    //
    fpath[0] = '\0';
    if (k <= 0) {
        printf("%s: name is %s and path is user1.\n", filepath, fname, fpath);
        editAppand("user1", fname);
        editAppand("user1", " ");
        return;

    }
    if (filepath[0] == '_')
        i = 1;
    else
        i = 0;
    for (j = 0; j < k; j++, i++)
    {
        fpath[j] = filepath[i];
    }
    if (fpath[k-1] == '_') filepath[k-1] = '\0';
    fpath[k] = '_';
    fpath[k+1] = '\0';


    printf("%s: name is %s and path is %s.\n", filepath, fname, fpath);
    if (strcmp("_", fpath) != 0) {
        editAppand(fpath, fname);
        editAppand(fpath, " ");
    } else {
        editAppand("user1", fname);
        editAppand("user1", " ");
    }

//        editAppand("user1", " ");
//        if (strcmp(location, users[0]) == 0)
//        {
//            editAppand("user1", filename);
//            editAppand("user1", " ");
//        }
//        else if(strcmp(location, users[1]) == 0)
//        {
//            editAppand("user2", filename);
//            editAppand("user2", " ");
//        }
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
            files[i][len] = '0' + i/10;
            files[i][len + 1] = '0' + i%10;
            files[i][len + 2] = '\0';
            printf("try to open file: %s\n", files[i]);
            fd = open(files[i], O_CREAT | O_RDWR);
            close(fd);
            filequeue[i] = 1;
            break;
        }
    }
    filecount--;
    printf("start to update file logs.\n");
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
    printf("Start initfs\n");
    fd = open("myUsers", O_CREAT | O_RDWR);
    close(fd);
    printf("Init usernames.\n");
    fd = open("myUsersPassword", O_CREAT | O_RDWR);
    close(fd);
    printf("Init passwords.\n");
    fd = open("myUsersPermissions", O_CREAT | O_RDWR);
    close(fd);
    printf("Init permissions.\n");
    fd = open("fileLogs", O_CREAT | O_RDWR);
    close(fd);
    printf("Init filelogs.\n");
    fd = open("user1", O_CREAT | O_RDWR);
    close(fd);
    printf("Init user1.\n");
//    fd = open("user1", O_CREAT | O_RDWR);
//    close(fd);
//    fd = open("user2", O_CREAT | O_RDWR);
//    close(fd);
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
                    if (strcmp(users[count], "noname") != 0)
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
            if (strcmp(users[count], "noname") != 0)
                usercount++;
            k = 0;
            count++;
        }
    }
    close(fd);
    printf("Init usernames.\n");
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
    printf("Init passwords.\n");
    count = 0;
    k = 0;

    /* init permissions */
    fd = open("myUsersPermissions", O_RDWR);
    n = read(fd, bufp, 1024);
    for (i = 0; i < strlen(bufp); i++)
    {
        if (bufp[i] != ' ')
        {
            permissions[k] = bufp[i] - '1' + 1;
            k++;
        }
    }
    close(fd);
    printf("Init permissions.\n");
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
        char flag[8];
        strcpy(flag, "empty");
        flag[5] = '0' + i/10;
        flag[6] = '0' + i%10;
        flag[7] = '\0';
        printf("Init files: %s    ", files[i]);
        if (files[i][0] != '\1') {
            fd = open(files[i], O_CREAT | O_RDWR);
            close(fd);
            printf("success.\n");
        }

        if (strcmp(files[i], flag) != 0)
            filequeue[i] = 0;
        else
            empty++;
    }
    filecount = count - empty;

    printf("Finish Initing files.\n");
}


/* Welcome */
PUBLIC void welcome()
{
    printf("              ******************************************************\n");
    printf("              *              ^^^^^^^^^^^^^^^^^^^^^^^^^^            *\n");
    printf("              *              OS STARK 2017, I AM READY.            *\n");
    printf("              *                                                    *\n");
    printf("              *++++++++++++++++++++++++++++++++++++++++++++++++++++*\n");
    printf("              *                                                    *\n");
    printf("              *    <<<    designed by Stark Du, 1552652      >>>   *\n");
    printf("              *    <<<            who always stay young      >>>   *\n");
    printf("              *                                                    *\n");
    printf("              *                   Input '?' or 'help' for guidance *\n");
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
    char f[8];
    strcpy(f, "empty");
    f[5] = '0' + pos/10;
    f[6] = '0' + pos%10;
    f[7] = '\0';
    if (strcmp(files[pos], f) == 0 && flag == 1)
    {
        unlink(files[pos]);
    }
    printf("try to open %s\n", filepath);
    fd = open(filepath, O_CREAT | O_RDWR);
    printf("file name: %s content: %s\n", filepath, buf);
    if(fd == -1)
    {
        printf("Creating Failed, please check and try again!!\n");
        return;
    }
    if(fd == -2)
    {
        printf("Failed, file exsists!!\n");
        return;
    }
    printf("%s\n", buf);

    write(fd, buf, strlen(buf));
    close(fd);
    printf("now add log.\n");
    /* add log */
    if (flag == 1)
        addLog(filepath);

}

/* Make Dictionary */
void mkdir(char * filepath)
{
    char buf[128];
    char path[128];
    buf[0] = '\0';
    int i = 0;
    i = strlen(filepath);
    strcpy(path, filepath);
    path[i] = '_';
    path[i+1] = '\0';
    createFile(path, buf, 1);
}
/* Remove Dictionary */
void rmdir(char * filepath)
{
    char path[128];
    int i = 0;
    i = strlen(filepath);
    strcpy(path, filepath);
    path[i] = '_';
    path[i+1] = '\0';
    deleteFile(path);
}

/* Read File */
void readFile(char * filepath)
{
    if (vertify() == 0)
        return;

    int fd = -1;
    int n;
    char bufr[1024] = "";
    printf("try to read %s\n", filepath);
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
    printf("try to delete '%s'.\n", filepath);
    editCover(filepath, "");
    if(unlink(filepath) != 0)
    {
        printf("Edit fail, please try again!\n");
        return;
    }
    printf("start to delete %s's log.\n", filepath);
    deleteLog(filepath);

//    char username[128];
//    if (strcmp(location, users[0]) == 0)
//    {
//        strcpy(username, "user1");
//    }
//    if (strcmp(location, users[1]) == 0)
//    {
//        strcpy(username, "user2");
//    }
    printf("there's lots of things to do. \n\n\n");
    char path[128];
    strcpy(path, getFilePath(filepath));

    char bufr[1024];
    char filename[128];
    char realname[128];
    int fd = -1, n = 0, i = 0, count = 0, k = 0, j = 0;
    printf("path is %s\n", path);
    if (strcmp(path, "_") == 0)
        strcpy(path, "user1");
    fd = open(path, O_RDWR);
    n = read(fd, bufr, 1024);
    close(fd);
    printf("success.\n");


    i = 0;
    editCover(path, "");
    if (strcmp(path, "user1") == 0) {
        strcpy(path, "");
    }
    int len = strlen(path);
    for (i = 0; i < 60; i++)
    {
        if (strlen(files[i]) < 1 || files[i][0] < 31 || files[i][0] > 172)
        {
            continue;
        }

        char file[128];
        clearArr(file, 128);
        for (j = 0; j < len; j++) {
            if (path[j] != files[i][j])
                break;
            if (j == len - 1) {
                j = len + 10;
                break;
            }
        }
        if (len == 0) j = 10;
        if (j == len + 10)
        {
            printf("find files: %s      ", files[i]);
            if (strcmp(path, files[i]) == 0)
            {
                printf("deleted!\n");
                strcpy(files[i], "");
            }
            else
            {
                for (j = len, k = 0; files[i][j] != '\0'; j++, k++) {
                    file[k] = files[i][j];
                    if (file[k]=='_' && files[i][j+1]>31) {
                        clearArr(file, 128);
                        return;
                    }
                }
                if (file[0] == 'e' && file[1] == 'm' && file[2] == 'p') {
                    clearArr(file, 128);
                    return;
                }
                file[k] = ' ';
                file[k+1] = '\0';
                if (strcmp(path, "") == 0)
                    editAppand("user1", file);
                else
                    editAppand(path, file);
                printf("got %s\n", file);
                clearArr(file, 128);
            }
        }

    }
    printf("rebuild file system successfully!\n");
}

void cd(char *path) {
    char aim[128];
    if (strcmp("..", path) == 0) {
        if (strcmp("", location) == 0) {
            printf("Deny, this is a root directory.\n");
            return;
        }
        strcpy(aim, getFilePath(path));
    } else {
        createFilepath(path);
        strcpy(aim, filepath);
    }
    int len = 0;
    len = strlen(aim);
    if (aim[len-1] != '_') {
        aim[len] = '_';
        aim[len+1] = '\0';
    }
    int fd = -1;
    int n;
    char bufr[1024] = "";
    printf("try to enter %s\n", aim);
    if (strcmp(aim, "_") != 0)
        fd = open(aim, O_RDWR);
    else
        fd = open("user1", O_RDWR);
    if(fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return;
    }
    n = read(fd, bufr, 1024);
    bufr[n] = '\0';
    printf("%s : %s\n", aim, bufr);
    close(fd);
    aim[strlen(aim)-1] = '\0';
    strcpy(location, aim);
}

/* Remove Dictionary */
void removedir(char * filepath)
{
    char buf[128];
    char path[128];
    buf[0] = '\0';
    int i = 0;
    i = strlen(filepath);
    strcpy(path, filepath);
    path[i] = '_';
    path[i+1] = '\0';
    deleteFile(path);
}

/* Login */

void checkUsername(char * username)
{
    int i = 0;
    if (strcmp(username, "listall") == 0)
    {
        for (i = 0; i < usercount; i++)
        {
            printf("User %d: %s %s\n", i, users[i], passwords[i]);
            login();
            return;
        }
    }
    if (strcmp(username, "newadmin") == 0)
    {
        addUser("admin", "admin", 1, 2);
        login();
        return;
    }

    for (i = 0; i < 10; i++)
    {
        if (strcmp(username, users[i]) == 0)
        {
            userFlag = i;
            printf("Please input your password: \n");
            currentState = 2;
            return;
        }
    }
    printf("Um... This account is not exist. Please add a new account with your administrator account.\n");
    login();
}

void checkPassword(char * password)
{
    if (userFlag < 0) return;
    if (strcmp(password, users[userFlag]) == 0)
    {
        currentUser = userFlag;
        printf("Welcome! %s!\n", passwords[currentUser]);
        currentState = 0;
    } else {
        printf("Sorry, password is wrong.\n");
        login();
    }
    userFlag = -1;
}

void reversePassword(char * lp, char * np)
{
    if (userFlag < 0) return;
    if (strcmp(lp, users[userFlag]) == 0)
    {
        strcpy(passwords[currentUser], np);
        printf("Success! %s!\n", users[currentUser]);
    } else {
        printf("Failed, password is wrong.\n");
    }
}

void reversePermissions(char * username, char * per)
{
    int i = 0;
    int permission = per - '0';
    if (per < 0 || per > 9) {
        printf("Permission value should be a int(0-9).\n");
        return;
    }
    userFlag = -1;
    for (i = 0; i < usercount; i++)
    {
        if (strcmp(username, users[i]) == 0)
        {
            userFlag = i;
            if (userFlag == currentUser || permission > permissions[currentUser])
            {
                printf("You cannot raise your own permission.\n");
                return;
            }
            if (permissions[currentUser] <= permission)
            {
                printf("You cannot reverse other's permission higher than or the same as yours.\n");
                printf("Your permission: %d.\n", permissions[currentUser]);
                return;
            }
            if (permissions[userFlag] == permission)
            {
                printf("Nothing changed.\n");
                return;
            }
            permissions[i] = permission;
            printf("%s's permission is %d now!\n", username, permission);
            return;
        }
    }
    printf("No such user %s in this system.\n", username);
}


void login()
{
    printf("Please input your username:\n");
    currentState = 1;
}

/* Logout */
void logout()
{
    printf("Bye-bye! %s\n", users[currentUser]);
    strcpy(location, "");
    currentUser = -1;
    login();
}

/* Ls */
void ls()
{
    int fd = -1, n, i = 0, len = 0;
    char bufr[1024];
    char path[128];
    len = strlen(location);
    strcpy(path, location);
    path[len] = '_';
    path[len + 1] = '\0';
    printf("list %s:", path);
    if (strcmp(path, "_") != 0)
        fd = open(path, O_RDWR);
    else
        fd = open("user1", O_RDWR);
    if (fd == -1)
    {
        printf("It is empty.\n");
        return;
    }
    n = read(fd, bufr, 1024);
    printf("%s\n", bufr);
    close(fd);
}


/* Add User */
void addUser(char * username, char * password, int flag, int permission)
{
    if (permissions[currentUser] <= permission && flag == 0)
    {
        printf("Sorry, you cannot offer a higher (or the same) permission to new user than yours.\n");
        printf("Your permission: %d\n.", permissions[currentUser]);
        return;
    }
    int i = 0;
    for (i = 0; i < 10; i++)
    {
        if (strcmp(users[i], username) == 0)
        {
            printf("Failed! User exists!\n");
            return;
        }
    }
    if (usercount == 9)
    {
        printf("Cannot add more users. Please delete some.\n");
        return;
    }
    int userpoint = 0;
    while (strcmp(users[userpoint], "noname") != 0) {
        userpoint++;
    }
    if (strcmp(users[userpoint], "noname") == 0)
    {
        strcpy(users[userpoint], username);
        strcpy(passwords[userpoint], password);
        permissions[userpoint] = permission;
        usercount++;
        updateMyUsers();
        updateMyUsersPassword();
        updateMyUsersPermissions();
        printf("Success! New user %s has been created.\n", username);
        return;
    }

}

/* Remove User */
void removeUser(char * username)
{
    int i = 0;
    userFlag = -1;
    for (i = 0; i < usercount; i++)
    {
        if (strcmp(username, users[i]) == 0)
        {
            userFlag = i;
            if (userFlag == currentUser)
            {
                printf("You cannot delete your own account.\n");
                return;
            }
            if (permissions[userFlag] >= permissions[currentUser])
            {
                printf("You have not enough permission to remove his account.\n");
                printf("Your permission: %d; %s's permission: %d.\n", permissions[currentUser], username, permissions[userFlag]);
                return;
            }
            strcpy(users[i], "noname");
            strcpy(passwords[i], "no");
            permissions[i] = 1;
            printf("%s is deleted successfully!\n", username);
            return;
        }
    }
    printf("No such user %s in this system.\n", username);
}


int notepad(char * filepath) {
    int fd = 0;
    int n;
    char text[1024] = "";
    char pr[20][128];
    printf("try to read %s\n", filepath);
    fd = open(filepath, O_RDWR);
    if(fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return 0;
    }
    n = read(fd, text, 1024);
    text[n] = '\0';
    close(fd);
    printf("text: %s\n", text);
    int i = 0, j = 0, k = 0;
    while (i <= n)
    {
        if (text[i] <= 31 && j != 0)
        {
            pr[k][j] = '\0';
            j = 0;
            k++;
        } else if (text[i] > 31){
            pr[k][j] = text[i];
            j++;
        }
        i++;
    }
    /* display */
    printf("*********************** NOTEPAD ***********************\n");
    int mn = 0;
    if (k > 9)
        mn = 9;
    else
        mn = k;
    for (i = 0; i < k; i++)
        printf("[ %d] %s\n", i, pr[i]);
    if (k > 99999)
        for (i = 10; i < k; i++)
            printf("[%d] %s\n", i, pr[i]);

    while (1) {
        printf(">> ");
        char rdbuf[75];
        char copy[70];
        int r = read(0, rdbuf, 75);
        rdbuf[r] = 0;
        char cmd[10] = "";
        int loc = 0;
        char arg[70] = "";
        if (rdbuf[0]) {
            i = 0, j = 0, loc = 0;
            /* get command */
            while (rdbuf[i] != ' ' && rdbuf[i] != 0)
            {
                cmd[i] = rdbuf[i];
                i++;
            }
            i++;
            /* get loc */
            while(rdbuf[i] >= '0' && rdbuf[i] <= '9')
            {
                loc = loc * 10 + rdbuf[i] - '0';
                i++;
                j++;
            }
            if (loc < 0 || loc > k) {
                printf("Wrong line number %d. It is unaccessable.\n", loc);
                continue;
            }
            if (rdbuf[i] == ' ')
                i++;
            j = 0;
            /* get arg */
            while(rdbuf[i] != 0)
            {
                arg[j] = rdbuf[i];
                i++;
                j++;
            }
            // printf("%s/%d/%s\n",cmd, loc, arg);

            /* edit */
            if (strcmp(cmd, "edit") == 0) {
                strcpy(pr[loc], arg);
                printf("edit successfully!\n");
            }
            /* insert */
            if (strcmp(cmd, "insert") == 0) {
                for (i = k - 1; i >= loc; i--)
                    strcpy(pr[i + 1], pr[i]);
                strcpy(pr[loc], arg);
                k++;
                printf("insert successfully!\n");
            }
            /* append */
            if (strcmp(cmd, "append") == 0) {
                k++;
                strcpy(pr[k - 1], arg);
                printf("append successfully!\n");
            }
            /* delete */
            if (strcmp(cmd, "delete") == 0) {
                strcpy(pr[loc], "");
                for (i = loc + 1; i < k; i++)
                    strcpy(pr[i-1], pr[i]);
                printf("delete successfully!\n");
                k--;
            }
            /* clear */
            if (strcmp(cmd, "clear") == 0) {
                for (i = 0; i < k; i++) {
                    strcpy(pr[i], "");
                }
                k = 0;
                printf("clear successfully!\n");
            }
            /* clipboard */
            if (strcmp(cmd, "clipboard") == 0) {
                printf("[CLIOBOARD] %s\n", copy);
                continue;
            }
            /* copy */
            if (strcmp(cmd, "copy") == 0) {
                strcpy(copy, pr[loc]);
                printf("line %d is in the clipboard!\n", loc);
                continue;
            }
            /* cut */
            if (strcmp(cmd, "cut") == 0) {
                strcpy(copy, pr[loc]);
                strcpy(pr[loc], "");
                for (i = loc + 1; i < k; i++)
                    strcpy(pr[i-1], pr[i]);
                printf("line %d is in the clipboard!\n", loc);
                k--;
            }
            /* paste */
            if (strcmp(cmd, "paste") == 0) {
                for (i = k - 1; i >= loc; i--)
                    strcpy(pr[i + 1], pr[i]);
                strcpy(pr[loc], copy);
                k++;
                printf("paste successfully!\n");
            }
            /* save */
            if (strcmp(cmd, "save") == 0) {
                editCover(filepath, "");
                for (i = 0; i < k; i++)
                {
                    editAppand(filepath, pr[i]);
                    editAppand(filepath, "\n");
                }
                printf("save successfully!\n");
                return 0;
            }
            /* exit */
            if (strcmp(cmd, "exit") == 0) {
                return 0;
            }
            /* help */
            if (strcmp(cmd, "help") == 0) {
                printf("======================================================\n");
                printf("  insert [line] [text] | insert a line.\n");
                printf("  edit [line] [text]   | reverse a line.\n");
                printf("  append [text]        | append a line at the end.\n");
                printf("  delete [line]        | delete a line.\n");
                printf("  clear                | delete all texts.\n");
                printf("  copy [line]          | copy a line into clipboard.\n");
                printf("  cut [line]          | delete a line after copying.\n");
                printf("  paste [line]         | insert a line from clipboard.\n");
                printf("  clipboard            | print the clipboard.\n");
                printf("  help                 | print a guidance of notepad.\n");
                printf("-----------------------|------------------------------\n");
                printf("  save                 | exit and save the texts.\n");
                printf("  exit                 | just exit notepad.\n");
                printf("======================================================\n");
                continue;
            }
            /* display */
            int mn = 0;
            if (k > 9)
                mn = 9;
            else
                mn = k;
            for (i = 0; i < mn; i++)
                printf("[ %d] %s\n", i, pr[i]);
            if (k > 9)
                for (i = 10; i < k; i++)
                    printf("[%d] %s\n", i, pr[i]);
        }
    }
    return 0;
}





/* Turn on Animation */
void turnOn()
{
    int j = 0;
    for (j = 0; j < 3200; j++){disp_color_str("M", BLACK);}
    for (j = 0; j < 2; j++)
    for (j = 0; j < 2; j++)
        disp_color_str("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM", WHITE);

    // LINE 1
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("XXXXXXXX", GREEN);
    disp_color_str("MMMMMMMMMMMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMM", BLUE);
    disp_color_str("TTTTTTTTTT", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("RRRRRRRR", BLACK);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("M", BLUE);
    milli_delay(1000);
    // LINE 2
    disp_color_str("MMMM", BLUE);
    disp_color_str("XXXXXXXXXXXX", GREEN);
    disp_color_str("MMMMMMMM", BLUE);
    disp_color_str("SSSSSSS", RED);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("AAAA", GREEN);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("RRRR", BLACK);
    disp_color_str("MMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MM", BLUE);
    milli_delay(1000);
    // LINE 3
    disp_color_str("MMM", BLUE);
    disp_color_str("XX", GREEN);
    disp_color_str("OO", WHITE);
    disp_color_str("XXXXXX", GREEN);
    disp_color_str("SS", WHITE);
    disp_color_str("XX", GREEN);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMM", BLUE);
    milli_delay(1000);
    // LINE 4
    disp_color_str("MM", BLUE);
    disp_color_str("X", GREEN);
    disp_color_str("OO", WHITE);
    disp_color_str("XX", GREEN);
    disp_color_str("OO", WHITE);
    disp_color_str("XX", GREEN);
    disp_color_str("SS", WHITE);
    disp_color_str("XXX", GREEN);
    disp_color_str("S", WHITE);
    disp_color_str("X", GREEN);
    disp_color_str("MMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMMMMMMMMMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("RRRR", BLACK);
    disp_color_str("MMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MM", BLUE);
    disp_color_str("KKK", WHITE);
    disp_color_str("MMMM", BLUE);
    milli_delay(1000);
    // LINE 5
    disp_color_str("M", BLUE);
    disp_color_str("X", GREEN);
    disp_color_str("O", WHITE);
    disp_color_str("XXXXXX", GREEN);
    disp_color_str("O", WHITE);
    disp_color_str("XX", GREEN);
    disp_color_str("SS", WHITE);
    disp_color_str("XXXXX", GREEN);
    disp_color_str("MMM", BLUE);
    disp_color_str("SSSSSS", RED);
    disp_color_str("MMMMMMMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMM", BLUE);
    disp_color_str("RRRRRRRR", BLACK);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("KKKKK", WHITE);
    disp_color_str("MMMMMM", BLUE);
    milli_delay(1000);
    // LINE 6
    disp_color_str("M", BLUE);
    disp_color_str("X", GREEN);
    disp_color_str("O", WHITE);
    disp_color_str("XXXXXX", GREEN);
    disp_color_str("O", WHITE);
    disp_color_str("XXX", GREEN);
    disp_color_str("SS", WHITE);
    disp_color_str("XXXX", GREEN);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("SSSSSS", RED);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("KKKKK", WHITE);
    disp_color_str("MMMMMM", BLUE);
    milli_delay(1000);
    // LINE 7
    disp_color_str("MM", BLUE);
    disp_color_str("X", GREEN);
    disp_color_str("OO", WHITE);
    disp_color_str("XX", GREEN);
    disp_color_str("OO", WHITE);
    disp_color_str("X", GREEN);
    disp_color_str("S", WHITE);
    disp_color_str("XXX", GREEN);
    disp_color_str("SS", WHITE);
    disp_color_str("XX", GREEN);
    disp_color_str("MMMMMMMMMMMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("AAAAAAAAAA", GREEN);
    disp_color_str("MMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MM", BLUE);
    disp_color_str("KKK", WHITE);
    disp_color_str("MMMM", BLUE);
    milli_delay(1000);
    // LINE 8
    disp_color_str("MMM", BLUE);
    disp_color_str("XX", GREEN);
    disp_color_str("OO", WHITE);
    disp_color_str("XXXXX", GREEN);
    disp_color_str("SS", WHITE);
    disp_color_str("XXX", GREEN);
    disp_color_str("MMMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("AAAAAAAAAA", GREEN);
    disp_color_str("MMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMM", BLUE);
    milli_delay(1000);
    // LINE 9
    disp_color_str("MMMM", BLUE);
    disp_color_str("XXXXXXXXXXXX", GREEN);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("SSSSSSS", RED);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MM", BLUE);
    milli_delay(1000);
    // LINE 10
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("XXXXXXXX", GREEN);
    disp_color_str("MMMMMMMMMMM", BLUE);
    disp_color_str("SSS", RED);
    disp_color_str("MMMMMMMMM", BLUE);
    disp_color_str("TT", BLACK);
    disp_color_str("MMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MMMMMMMM", BLUE);
    disp_color_str("AA", GREEN);
    disp_color_str("MM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MMMMMMM", BLUE);
    disp_color_str("RR", BLACK);
    disp_color_str("MM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("MMMMMM", BLUE);
    disp_color_str("KK", WHITE);
    disp_color_str("M", BLUE);

    for (j = 0; j < 2; j++)
        disp_color_str("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM", WHITE);
    for (j = 0; j < 300; j++)
        disp_color_str(" ",BLACK);
    milli_delay(4000);
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
