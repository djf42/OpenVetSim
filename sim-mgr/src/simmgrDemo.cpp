/*
 * simmgrDemo.cpp
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
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>

#include <iostream>
#include <vector>  
#include <string>  
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

#include "../include/simmgr.h" 

using namespace std;

// In simPulseDemo
void pulseDemoTimerInit(void );
void pulseTimerRun(void );

//In simmgrCommon
extern int scenarioPid;
extern int runningAsDemo;
extern char sesid[];
extern char msgbuf[MSGBUF_LENGTH];
int scan_commands(void );
void comm_check(void );
void time_update(void );
int msec_time_update(void );
void awrr_check(void );
void cpr_check(void);
void shock_check(void);
int start_scenario(const char *name );
void checkEvents(void );
void clearAllTrends(void );
void resetAllParameters(void );
void setRespirationPeriods(int oldRate, int newRate );
void strToLower(char *buf );
void killScenario(int arg1, void *arg2 );
void simmgrInitialize(void );
void simmgrRun(void );

extern int scenarioPid;
extern int runningAsDemo;

int demoStartTime;
int demoEndLimit;
#define DEMO_MAX_RUNTIME	(2*60*60*1000)	// Limit to 2 hours runtime.


int
main(int argc, char *argv[] )
{
	int sts;
	
	char rbuf[512] = { 0, };
	char cmd[512];
	char user[128];
	
	FILE *fp;
	char *cp;
	int count;
	int pid;
	int ppid;
	int r_pid;
	int r_ppid;
	int r_cnt;
	
	runningAsDemo = 1;
	if ( argc > 1 )
	{
		snprintf(sesid, 1024, "%s", argv[1] );
		pid   = getpid();
		ppid  = getppid();
		
		// Check if Command Processing is available. This can happen when this task is started 
		// without appropriate permissions.
		if ( system(NULL) == 0 )
		{
			printf("No command processor\n" );
			perror("system" );
			snprintf(msgbuf, MSGBUF_LENGTH, "No command processor: %s", strerror(errno) );
			log_message("", msgbuf );
			exit ( 0 );
		}
		
		// If already running for this sessionId, then exit.
		// This ps call will match this task, plus any previously existing task with the same ssid
		snprintf(cmd, 512, "ps -ef | grep simmgrDemo | grep -v grep | grep %s\n", sesid );
		fp = popen(cmd, "r" );
		if ( fp == NULL )
		{
			cp = NULL;
			snprintf(msgbuf, MSGBUF_LENGTH, "popen fails: %s", strerror(errno) );
			log_message("", msgbuf );
			exit ( 0 );
		}
		else
		{
			count = 0;
			while ( (cp = fgets(rbuf, 512, fp )) != NULL )
			{
				r_cnt = sscanf(rbuf, "%s %d %d", user, &r_pid, &r_ppid );
				if ( r_cnt == 3 && r_pid != pid && r_ppid != ppid )	// Skip this process and its parent
				{
					count++;
				}
			}
			if ( count > 1 )
			{
				// Found a matching task, so exit
				snprintf(msgbuf, MSGBUF_LENGTH, "%s ", "Existing Demo, exiting" );
				log_message("", msgbuf );
				exit ( 0 );
			}
			else
			{
				pclose(fp );
			}
		}
	}
	else
	{
		snprintf(msgbuf, MSGBUF_LENGTH, "%s", "No Session ID specified" );
		log_message("", msgbuf );
		exit ( -1 );
	}
	
	daemonize();
	
	sts = on_exit(killScenario, (void *)0 );
	
	sts = initSHM(OPEN_WITH_CREATE, sesid );
	if ( sts < 0 )
	{
		perror("initSHM" );
		snprintf(msgbuf, MSGBUF_LENGTH, "initSHM Failed: %s", strerror(errno) );
		log_message("", msgbuf );
		
		exit ( -1 );
	}
	
	simmgrInitialize();
	pulseDemoTimerInit();
	
	demoStartTime = msec_time_update();
	demoEndLimit = demoStartTime + DEMO_MAX_RUNTIME;
	
	while ( simmgr_shm->server.msec_time < demoEndLimit )
	{	
		simmgrRun();
		pulseTimerRun();
		if ( strcmp(simmgr_shm->instructor.scenario.state, "running") == 0 )
		{
			demoEndLimit = simmgr_shm->server.msec_time + DEMO_MAX_RUNTIME;
		}
		usleep(10000);	// Sleep for 10 msec
	}
	snprintf(msgbuf, MSGBUF_LENGTH, "Demo Limit Exceeded: %d %d", simmgr_shm->server.msec_time, demoEndLimit );
	log_message("", msgbuf );
}
