/*
 * sim-util.c
 *
 * Common functions for the various SimMgr processes
 *
 * This file is part of the sim-mgr distribution (https://github.com/OpenVetSimDevelopers/sim-mgr).
 * 
 * Copyright (c) 2019 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
 
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <termios.h>

#define SIMUTIL	1

#include "../include/simmgr.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#define RUNNING_DIR 		"/"
#define LOCK_FILE_DIR        "/var/run/simmgr/"



int simmgrSHM;					// The File for shared memory
struct simmgr_shm *simmgr_shm;	// Data structure of the shared memory

char shmFileName[1024];
char msg_buf[2048];

/*
 * FUNCTION: initSHM
 *
 * Open the shared memory space
 *
 * Parameters: none
 *
 * Returns: 0 for success
 */

int
initSHM(int create, char *sesid )
{
	void *space;
	int mmapSize;
	int permission;
	
	mmapSize = sizeof(struct simmgr_shm );
	if ( ( sesid != NULL ) && ( strlen(sesid) > 0 ) )
	{
		sprintf(shmFileName, "%s_%s", SIMMGR_SHM_DEMO_NAME, sesid );
	}
	else
	{
		sprintf(shmFileName, "%s", SIMMGR_SHM_NAME );
	}
	switch ( create )
	{
		case OPEN_WITH_CREATE:
			permission = O_RDWR | O_CREAT | O_TRUNC;
			break;
			
		case OPEN_ACCESS:
		default:
			permission = O_RDWR;			
			break;
	}
	
	// Open the Shared Memory space
	simmgrSHM = shm_open(shmFileName, permission , 0755 );
	if ( simmgrSHM < 0 )
	{
		sprintf(msg_buf, "%s %s", shmFileName, strerror(errno) );

		log_message("", msg_buf );
		perror("shm_open" );
		return ( -3 );
	}
	if ( mmapSize < 4096 )
	{
		mmapSize = 4096;
	}
	
	if (  create == OPEN_WITH_CREATE )
	{
		lseek(simmgrSHM, mmapSize, SEEK_SET );
		write(simmgrSHM, "", 1 );
	}

	space = mmap((caddr_t)0,
				mmapSize, 
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				simmgrSHM,
				0 );
	if ( space == MAP_FAILED )
	{
		perror("mmap" );
		return ( -4 );
	}
	
	simmgr_shm = (struct simmgr_shm *)space;
	
	return ( 0 );
}


/*
 * Function: log_message
 *
 * Log a message to syslog. The message is also logged to a named file. The file is opened
 * for Append and closed on completion of the write. (The file write will be disabled after
 * development is completed).
 * 
 *
 * Parameters: filename - filename to open for writing
 *             message - Pointer to message string, NULL terminated
 *
 * Returns: none
 */
void log_message(const char *filename, const char* message)
{
#if LOG_TO_FILE == 1
	FILE *logfile;
#endif

	syslog(LOG_NOTICE, "%s", message);

#if LOG_TO_FILE == 1
	if ( strlen(filename > 0 )
	{
		logfile = fopen(filename,"a");
		if ( logfile )
		{
			fprintf(logfile, "%s\n", message);
			fclose(logfile );
		}
	}
#endif
}

/*
 * Function: signal_fault_handler
 *
 * Handle inbound signals.
 *		SIGHUP is ignored 
 *		SIGTERM closes the process
 *
 * Parameters: sig - the signal
 *
 * Returns: none
 */
void signal_fault_handler(int sig)
{
	int j, nptrs;
#define SIZE 100
	void *buffer[100];
	char **strings;

	nptrs = backtrace(buffer, SIZE);
	//printf("backtrace() returned %d addresses\n", nptrs);

	// The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	// would produce similar output to the following:
	syslog(LOG_NOTICE, "%s", "fault");
	
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) 
	{
		syslog(LOG_NOTICE, "fault: %s", "no symbols");
		exit(EXIT_FAILURE);
	}
	
	for ( j = 0; j < nptrs; j++ )
	{
		syslog(LOG_NOTICE, "%d: %s", j, strings[j]);
	}
	free(strings);
	exit ( -1 );
}
 /*
 * Function: signal_handler
 *
 * Handle inbound signals.
 *		SIGHUP is ignored 
 *		SIGTERM closes the process
 *
 * Parameters: sig - the signal
 *
 * Returns: none
 */
void signal_handler(int sig )
{
	switch(sig) {
	case SIGHUP:
		log_message("","hangup signal catched");
		break;
	case SIGTERM:
		log_message("","terminate signal catched");
		exit(0);
		break;
	}
}
/*
 * Function: daemonize
 *
 * Convert the task to a daemon. Make it independent of the parent to allow
 * running forever.
 *
 *   Change working directory
 *   Take Lock
 *
 * Parameters: none
 *
 * Returns: none
 */
void daemonize()
{
    pid_t pid;
	int i,lfp;
	char str[10];
	char buffer[128];
	extern int runningAsDemo;
	
	if ( getppid()==1 )
	{
		return; /* already a daemon */
	}
    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if ( pid < 0 )
	{
        exit(EXIT_FAILURE);
	}
    /* Success: Let the parent terminate */
    if ( pid > 0 )
	{
        exit(EXIT_SUCCESS);
	}
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
	{
        exit(EXIT_FAILURE);
	}
	for ( i = getdtablesize() ; i >= 0 ; --i )
	{
		close(i); /* close all descriptors */
	}
	
	i = open("/dev/null",O_RDWR);
	dup(i);
	dup(i); /* handle standard I/O */
	
	umask(027); /* set newly created file permissions */
	chdir(RUNNING_DIR); /* change running directory */
	
	// Create a Lock File to prevent multiple daemons
	// Skip lock for Demo runs
	if ( runningAsDemo == 0 )
	{
		sprintf(buffer, "%s%s.pid", LOCK_FILE_DIR, program_invocation_short_name );
		lfp = open(buffer, O_RDWR|O_CREAT, 0640 );
		if ( lfp < 0 )
		{
			syslog(LOG_DAEMON | LOG_ERR, "open(%s) failed: %s", buffer, strerror(errno));
			exit(1); /* can not open */
		}
		if ( lockf(lfp,F_TLOCK,0) < 0 )
		{
			syslog(LOG_DAEMON | LOG_WARNING, "Cannot take lock");
			exit(0); /* can not lock */
		}
	}
	/* first instance continues */
	sprintf(str, "%d\n", getpid());
	write(lfp, str, strlen(str)); /* record pid to lockfile */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,signal_handler); /* catch hangup signal */
	signal(SIGTERM,signal_handler); /* catch kill signal */
	signal(SIGSEGV,signal_fault_handler );   // install our fault handler
	
	syslog (LOG_DAEMON | LOG_NOTICE, "%s Started", program_invocation_short_name );
}

/*
 * FUNCTION: kbhit
 * 
 * Check if a user has hit a key on the keyboard.
 */
 
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}

/*
 * FUNCTION: checkExit
 * 
 * If a 'q' or 'Q' is typed on the keyboard, quit the program.
 * If any other key is entered, return it.
 */
  
int
checkExit(void )
{
	int cc = 0;

	if ( kbhit() )
	{
		cc = getchar();
		if ( cc == 'q' || cc == 'Q' )
		{
			printf("\n" );
			exit ( 0 );
		}
		return ( cc );
	}
	return ( 0 );
}

// Issue a shell command and read the first line returned from it.
char *
do_command_read(const char *cmd_str, char *buffer, int max_len )
{
	FILE *fp;
	char *cp;
	
	fp = popen(cmd_str, "r" );
	if ( fp == NULL )
	{
		cp = NULL;
	}
	else
	{
		cp = fgets(buffer, max_len, fp );
		if ( cp != NULL )
		{
			strtok(buffer, "\n"); // Strip the newline
		}
		pclose(fp );
	}
	return ( cp );
}

void
get_date(char *buffer )
{
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime (buffer,80,"%m%d%H%M%Y.%S",timeinfo);
}

char eth0_ip[512] = { 0, };
char wifi_ip[512] = { 0, };
char ipAddr[512] = { 0, };

char *get_IP(const char  *iface )
{
	int fd;
	struct ifreq ifr;
	int sts;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);
	sts = ioctl(fd, SIOCGIFADDR, &ifr);
	if ( sts == 0 )
	{
		sprintf(ipAddr, "%s %s", iface, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	}
	else
	{
		sprintf(ipAddr, "%s", "" );
	}
	close(fd);
	return ipAddr;
}
char *
getETH0_IP()
{
	char *addr;
	
	addr = get_IP("eth0" );
	if ( strlen(addr) == 0 )
	{
		addr = get_IP("eno1" );
	}
	sprintf(eth0_ip, "%s", addr );
	return eth0_ip;
}
	
char *
getWIFI_IP()
{
	char *addr;
	
	addr = get_IP("wlp58s0" );
	sprintf(wifi_ip, "%s", addr );
	return wifi_ip;
}
/*
 * itoa
 * @val - Value to be converted
 * @buf - Pointer to buffer for the string to be returned
 * @radix - the Radix (8,10,16)
 *
* Return the 'val' as a string in the requested radix
*/
char *
itoa(int val, char *buf, int radix )
{
	if ( radix == 10 )
	{
		sprintf(buf, "%d", val );
	}
	else if ( radix == 16 )
	{
		sprintf(buf, "%x", val );
	}
	else if ( radix == 8 )
	{
		sprintf(buf, "%o", val );
	}
	else
	{
		sprintf(buf, "%s", "" );
	}
	return ( buf );
}

/**
 * cleanString
 *
 * remove leading and trailing spaces. Reduce internal spaces to single. Remove tabs, newlines, CRs.
*/
#define WHERE_START		0
#define WHERE_TEXT		1
#define WHERE_SPACE		2

void
cleanString(char *strIn )
{
	char *in;
	char *out;
	int where = WHERE_START;
	
	in = (char *)strIn;
	out = (char *)strIn;
	while ( *in )
	{
		if ( isspace( *in ) )
		{
			switch ( where )
			{
				case WHERE_START:
				case WHERE_SPACE:
					break;
				
				case WHERE_TEXT:
					*out = ' ';
					out++;
					where = WHERE_SPACE;
					break;
			}
		}
		else
		{
			where = WHERE_TEXT;
			*out = *in;
			out++;
		}
		in++;
	}
	*out = 0;
}

/*
 * takeInstructorLock
 * @
 *
 * Call to take the Instructor area lock
 */	
 
int
takeInstructorLock()
{
	int trycount = 0;
	int sts;
	
	while ( ( sts = sem_trywait(&simmgr_shm->instructor.sema) ) != 0 )
	{
		if ( trycount++ > 50 )
		{
			fprintf(stderr, "%s", "failed to take Instructor lock" );
			return ( -1 );
		}
		usleep(1000 );
	}
	return ( 0 );
}


/*
 * releaseInstructorLock
 * @
 *
 * Call to release the Instructor area lock
 */	
void
releaseInstructorLock()
{
	sem_post(&simmgr_shm->instructor.sema);
}

/*
 * addEvent
 * @str - pointer to event to add
 *
 * Must be called with instructor lock held
 */
void
addEvent(char *str )
{
	int eventNext;

	// Event: add to event list at end and increment eventListNext
	eventNext = simmgr_shm->eventListNext;
	
	sprintf(simmgr_shm->eventList[eventNext].eventName, "%s", str );
	
	snprintf(msg_buf, 2048, "Event %d: %s", eventNext, str );
	log_message("", msg_buf );
	
	eventNext += 1;
	if ( eventNext >= EVENT_LIST_SIZE )
	{
		eventNext = 0;
	}
	simmgr_shm->eventListNext = eventNext;
	if ( strcmp(str, "aed" ) == 0 )
	{
		simmgr_shm->instructor.defibrillation.shock = 1;
	}
}

/*
 * addComment
 * @str - pointer to comment to add
 *
 * Must be called with instructor lock held
 */	
void
addComment(char *str )
{
	int commentNext;
	
	commentNext = simmgr_shm->commentListNext;

	// Event: add to Comment list at end and increment commentListNext
	sprintf(simmgr_shm->commentList[commentNext].comment, "%s", str );
	
	commentNext += 1;
	if ( commentNext >= COMMENT_LIST_SIZE )
	{
		commentNext = 0;
	}
	simmgr_shm->commentListNext = commentNext;
}

/*
 * lockAndComment
 * @str - pointer to comment to add
 *
 * Take Instructor Lock and and comment
 */	
void
lockAndComment(char *str )
{
	int sts;
	
	sts = takeInstructorLock();
	if ( !sts )
	{
		addComment(str );
		releaseInstructorLock();
	}
}

void
forceInstructorLock(void )
{
	while ( takeInstructorLock() )
	{
		// Force Release until we can take the lock
		releaseInstructorLock();
	}
	releaseInstructorLock();
}
