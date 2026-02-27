/*
 * simmgr.cpp
 *
 * SimMgr daemon.
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

//In simmgrCommon
extern int scenarioPid;

extern int runningAsDemo;
extern char sesid[];

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

// In simmgrVideo
int initOBSSHM(int create );

int
main(int argc, char *argv[] )
{
	int sts;
	runningAsDemo = 0;
	
	memset(sesid, 0, 4 );
	daemonize();
	
	sts = on_exit(killScenario, (void *)0 );
	
	sts = initSHM(OPEN_WITH_CREATE, sesid );
	if ( sts < 0 )
	{
		perror("initSHM SIMMGR" );
		exit ( -1 );
	}
	sts = initOBSSHM(OPEN_WITH_CREATE );
	if ( sts < 0 )
	{
		perror("initSHM OBS" );
		exit ( -1 );
	}
	
	simmgrInitialize();
	
	while ( 1 )
	{
		simmgrRun();
		usleep(10000);	// Sleep for 10 msec
	}
}
