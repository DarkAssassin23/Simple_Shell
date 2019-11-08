/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
int verbose = 0;            /* if true, print additional output */

extern char **environ;      /* defined in libc */
static char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
static struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
char * removeWhiteSpace(const char * str);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);



/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    /* don't evaluate whitespace only inputs */ 
    int iterator = 0;
    while(1) {
        if(cmdline[iterator] == '\0') {
	    return;
	}
	if(!isspace(cmdline[iterator])) {
	    break;
	}
	iterator += 1;
    }
    char *commandVal = removeWhiteSpace(cmdline);
    int state = -1;
    int len;

    for(len=0;commandVal[len]!='\0'; ++len)
    {
        continue;
    }
    if(commandVal[len-1]=='&')
    {
        state = BG;
    }
    else
    {
        state = FG;
    }
    if(strcmp(commandVal, "quit")==0)
    {
        builtin_cmd(&commandVal);
    }
    else if(strcmp(commandVal, "exit")==0)
    {
        builtin_cmd(&commandVal);
    }
    else if(strcmp(commandVal, "jobs")==0)
    {
        builtin_cmd(&commandVal);
    }
    else if(commandVal[0]=='f' && commandVal[1]=='g')
    {
        builtin_cmd(&commandVal);
    }
    else if(commandVal[0]=='b' && commandVal[1]=='g')
    {
        builtin_cmd(&commandVal);
    }
    else
    {
	char commandCopy[len];
	char *argArray[1000];
	int argarrayLen = 0;
	sigset_t mask;

        strcpy(commandCopy,commandVal);	
	argArray[argarrayLen] = strtok(commandCopy," ");
	while(argArray[argarrayLen] != NULL) {
	    argarrayLen += 1;
	    argArray[argarrayLen] = strtok(NULL," ");
	}

	/* Remove & from arguments */
	if(strcmp(argArray[argarrayLen-1],"&") == 0) {
	    argArray[argarrayLen-1] = NULL;
            argarrayLen -= 1;
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	
        pid_t pid = fork();
	if(pid==0){
	    setpgid(0, 0);
	    sigprocmask(SIG_UNBLOCK, &mask, NULL);
	    if (execve(argArray[0],argArray,NULL) == -1) {
		printf("%s: Command not found.\n", argArray[0]);
		_exit(EXIT_FAILURE);
	    }
	    //_exit(EXIT_SUCCESS);
	}
	else {
            addjob(jobs,pid,state,commandVal);
	    sigprocmask(SIG_UNBLOCK, &mask, NULL);
	    int status;
	    if(state == BG) {
            printf("[%d] (%d) %s\n", pid2jid(jobs,pid), pid, commandVal);
	        waitpid(pid, &status, WNOHANG);
	    }
	    else {
		waitfg(pid);
	    }
	    if (status != EXIT_SUCCESS) {
                //printf("There was an error\n");
	    }
	}


    }
    return;
}


char * removeWhiteSpace(const char * str)
{
    int i, j;
    char * newString;

    newString = (char *)malloc(1000);

    i = 0;
    j = 0;

    while(str[i] != '\0')
    {
        /* If blank space is found */
        if(str[i] == ' ')
        {
            newString[j] = ' ';
            j++;

            /* Skip all consecutive spaces */
            while(str[i] == ' ')
                i++;
        }
        if(str[i]=='\n')
        {
            if(newString[j-1]==' ')
                newString[j-1]='\0';
            else
                newString[j]='\0';
            j++;
        }
        newString[j] = str[i];

        i++;
        j++;
    }
    // NULL terminate the new string
    newString[j] = '\0';

    return newString;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
    if(strcmp(argv[0],"quit")==0)
    {
        initjobs(jobs);
        exit(0);
    }
    else if(strcmp(argv[0],"exit")==0)
    {
        initjobs(jobs);
        exit(0);
    }
    else if(strcmp(argv[0],"jobs")==0)
    {
        listjobs(jobs);
    }
    else if(argv[0][0]=='f' && argv[0][1]=='g')
    {
        do_bgfg(argv);
    }
    else if(argv[0][0]=='b' && argv[0][1]=='g')
    {
        do_bgfg(argv);
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    struct job_t *job;
    int jid = 0;
    int id = 0;
    if(argv[0][3]=='%')
    {
        int x = 4;
        while(argv[0][x]!='\0')
        {
            if(x==3)
                jid = argv[0][x] - '0';
            else
                jid = jid*10 + (argv[0][x] - '0');
            x++;
        }
        
        if(getjobjid(jobs,jid)==NULL)
        {
            if(jid==-38)
            {
                printf("No job argument given\n");
                return;
            }
            printf("(%%%d): No such job\n",jid);
            return;
        }
    }
    else
    {
        int x = 3;
        while(argv[0][x]!='\0')
        {
            if(x==3)
                id = argv[0][x] - '0';
            else
                id = id*10 + (argv[0][x] - '0');
            x++;
        }
        if(getjobpid(jobs,id)==NULL)
        {
            if(id==-38)
            {
                printf("No job argument given\n");
                return;
            }
            printf("(%d): No such process\n",id);
            return;
        }
    }
    if(argv[0][0]=='f')
    {
        if(jid!=0)
            job = getjobjid(jobs,jid);
        else
            job = getjobpid(jobs,id);
        
        if(kill(-(job->pid),SIGCONT)<0)
        {
            if(errno!=ESRCH)
            {
                unix_error("Kill Error");
            }
        }
        job->state = FG;
        waitfg(job->pid);
    }
    else
    {
        if(jid!=0)
            job = getjobjid(jobs,jid);
        else
            job = getjobpid(jobs,id);
        
        if(kill(-(job->pid),SIGCONT)<0)
        {
            if(errno!=ESRCH)
            {
                unix_error("Kill Error");
            }
        }
        printf("[%d] (%d) %s\n", job->jid, job->pid, job->cmdline);
        job->state = BG;
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    while(pid==fgpid(jobs))
    {
        sleep(0);
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {
        if (WIFEXITED(status)){
            deletejob(jobs, pid);
        }
        else if(WIFSIGNALED(status))
        {
            deletejob(jobs, pid);
            printf("Job (%d) terminated by signal %d\n", pid,2);
        }
        else if(WSTOPSIG(status))
        {
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(jobs,pid), pid,20);
            getjobpid(jobs,pid)->state = ST;
        }
    }
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    pid_t foregroundJob = fgpid(jobs);

    if(foregroundJob == 0) {
        return;
    }

    kill(foregroundJob * -1, SIGINT);
    printf("Job (%d) terminated by signal %d\n", foregroundJob,2);
    //deletejob(jobs,foregroundJob);

    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    pid_t foregroundJob = fgpid(jobs);

    if(foregroundJob == 0) {
        return;
    }

    kill(foregroundJob * -1, SIGTSTP);

    return;
}

/*********************
 * End signal handlers
 *********************/



/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
