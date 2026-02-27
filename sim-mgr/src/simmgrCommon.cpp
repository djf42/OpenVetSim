/*
 * simmgrCommon.cpp
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
 
/* The simmgr Daemon and the simmgr Demo use the common code in this file */

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
int scan_commands(void );
void checkScenarioProcess(void );
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

// In simmgrVideo
int initOBSSHM(int create );
void recordStartStop(int record );
int getVideoFileCount(void );


int lastEventLogged = 0;
int lastCommentLogged = 0;

int iiLockTaken = 0;
char buf[1024];

int runningAsDemo;
int scenarioPid = -1;
char sesid[1024] = { 0, };
char msgbuf[MSGBUF_LENGTH];
char sessionsPath[MSGBUF_LENGTH];

// Time values, to track start time and elapsed time
// This is the "absolute" time
std::time_t scenario_start_time;
std::time_t now;
std::time_t scenario_run_time;


ScenarioState scenario_state = ScenarioStopped;
NibpState nibp_state = NibpIdle;
std::time_t nibp_next_time;
std::time_t nibp_run_complete_time;

/* str_thdata
	structure to hold data to be passed to a thread
*/
typedef struct str_thdata
{
    int thread_no;
    char message[100];
} thdata;


timer_t hrcheck_timer;
struct sigevent hrcheck_sev;
static void hrcheck_handler(int sig, siginfo_t *si, void *uc);

/* prototype for thread routines */
void heart_thread ( void *ptr );
void nibp_thread ( void *ptr );


void
simmgrInitialize(void )
{
	char *ptr;
	struct sigaction new_action;
	sigset_t mask;
	struct itimerspec its;
	
// Zero out the shared memory and reinit the values
	memset(simmgr_shm, 0, sizeof(struct simmgr_shm) );

	// hdr
	simmgr_shm->hdr.version = SIMMGR_VERSION;
	simmgr_shm->hdr.size = sizeof(struct simmgr_shm);


	// server
	do_command_read("/bin/hostname", simmgr_shm->server.name, sizeof(simmgr_shm->server.name)-1 );
	ptr = getETH0_IP();
	sprintf(simmgr_shm->server.ip_addr, "%s", ptr );
	// server_time and msec_time are updated in the loop
	
	resetAllParameters();
	
	// status/scenario
	sprintf(simmgr_shm->status.scenario.active, "%s", "default" );
	sprintf(simmgr_shm->status.scenario.state, "%s", "Stopped" );
	simmgr_shm->status.scenario.record = 0;
	
	// instructor/sema
	sem_init(&simmgr_shm->instructor.sema, 1, 1 ); // pshared =1, value =1
	iiLockTaken = 0;
	
	// instructor/scenario
	sprintf(simmgr_shm->instructor.scenario.active, "%s", "" );
	sprintf(simmgr_shm->instructor.scenario.state, "%s", "" );
	simmgr_shm->instructor.scenario.record = -1;
	
	// Log File
	sem_init(&simmgr_shm->logfile.sema, 1, 1 ); // pshared =1, value =1
	simmgr_shm->logfile.active = 0;
	sprintf(simmgr_shm->logfile.filename, "%s", "" );
	simmgr_shm->logfile.lines_written = 0;
	
	// Event List
	simmgr_shm->eventListNext = 0;
	lastEventLogged = 0;
	
	// Comment List
	simmgr_shm->commentListNext = 0;
	lastCommentLogged = 0;
	
	// instructor/cpr
	simmgr_shm->instructor.cpr.compression = -1;
	simmgr_shm->instructor.cpr.last = -1;
	simmgr_shm->instructor.cpr.release = -1;
	simmgr_shm->instructor.cpr.duration = -1;
	// instructor/defibrillation
	simmgr_shm->instructor.defibrillation.last = -1;
	simmgr_shm->instructor.defibrillation.energy = -1;
	simmgr_shm->instructor.defibrillation.shock = -1;
	
	clearAllTrends();

	// Use a timer for HR checks
	new_action.sa_flags = SA_SIGINFO;
	new_action.sa_sigaction = hrcheck_handler;
	sigemptyset(&new_action.sa_mask);

#define HRCHECK_TIMER_SIG	(SIGRTMIN+2)
	if (sigaction(HRCHECK_TIMER_SIG, &new_action, NULL) == -1)
	{
		perror("sigaction");
		sprintf(msgbuf, "sigaction() fails for HRCheck Timer: %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Block timer signal temporarily
	sigemptyset(&mask);
	sigaddset(&mask, HRCHECK_TIMER_SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		perror("sigprocmask");
		sprintf(msgbuf, "sigprocmask() fails for Pulse Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Create the Timer
	hrcheck_sev.sigev_notify = SIGEV_SIGNAL;
	hrcheck_sev.sigev_signo = HRCHECK_TIMER_SIG;
	hrcheck_sev.sigev_value.sival_ptr = &hrcheck_timer;
	
	if ( timer_create(CLOCK_REALTIME, &hrcheck_sev, &hrcheck_timer ) == -1 )
	{
		perror("timer_create" );
		sprintf(msgbuf, "timer_create() fails for HRCheck Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit (-1);
	}
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
		perror("sigprocmask");
		sprintf(msgbuf, "sigprocmask() fails for HRCheck Timer%s ", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	its.it_interval.tv_sec = (long int)0;
	its.it_interval.tv_nsec = (long int)(5 * 1000 * 1000);	// 10 msec
	its.it_value.tv_sec = (long int)0;
	its.it_value.tv_nsec = (long int)(5 * 1000 * 1000);	// 10 msec
	timer_settime(hrcheck_timer, 0, &its, NULL);

}

#define SCENARIO_LOOP_COUNT		19	// Run the scenario every SCENARIO_LOOP_COUNT iterations of the 10 msec loop
#define SCENARIO_PROCCHECK  	1
#define SCENARIO_COMMCHECK  	5
#define SCENARIO_EVENTCHECK		10
#define SCENARIO_CPRCHECK		13
#define SCENARIO_SHOCKCHECK		14
#define SCENARIO_AWRRCHECK		15
#define SCENARIO_TIMECHECK		18

int scenarioCount = 0;

void
simmgrRun(void )
{
	scenarioCount++;
	switch ( scenarioCount )
	{
		case SCENARIO_PROCCHECK:
			checkScenarioProcess();
			break;
		case SCENARIO_COMMCHECK:
			comm_check();
			break;
		case SCENARIO_EVENTCHECK:
			checkEvents();
			break;
		case SCENARIO_CPRCHECK:
			cpr_check();
			break;
		case SCENARIO_SHOCKCHECK:
			shock_check();
			break;
		case SCENARIO_AWRRCHECK:
			awrr_check();
			break;
		case SCENARIO_TIMECHECK:
			time_update();
			break;
		case SCENARIO_LOOP_COUNT:
			scenarioCount = 0;
			(void)scan_commands();
			break;
		default:
			break;
	}
}

int
updateScenarioState(ScenarioState new_state)
{
	int rval = true;
	ScenarioState old_state;
	old_state = scenario_state;
	if ( new_state != old_state )
	{
		if ( ( new_state == ScenarioTerminate ) && ( ( old_state != ScenarioRunning ) && ( old_state != ScenarioPaused )) )
		{
			rval = false;
			sprintf(msgbuf, "Terminate requested while in state %d", old_state );
			simlog_entry(msgbuf );
		}
		else if ( ( new_state == ScenarioPaused ) && ( ( old_state != ScenarioRunning ) && ( old_state != ScenarioPaused )) )
		{
			rval = false;
			sprintf(msgbuf, "Pause requested while in state %d", old_state );
			simlog_entry(msgbuf );
		}
		else 
		{
			scenario_state = new_state;
			
			switch ( scenario_state )
			{
				case ScenarioStopped: // Set by scenario manager after Terminate Cleanup is complete
					sprintf(simmgr_shm->status.scenario.state, "Stopped" );
					if ( ! runningAsDemo && simmgr_shm->status.scenario.record > 0 )
					{
						recordStartStop(0 );
					}
					(void)simlog_end();
					(void)resetAllParameters();
					break;
				case ScenarioRunning:
					if ( old_state == ScenarioPaused )
					{
						sprintf(msgbuf, "Resume" );
						simlog_entry(msgbuf );
					}
					sprintf(simmgr_shm->status.scenario.state, "Running" );
					break;
				case ScenarioPaused:
					sprintf(msgbuf, "Pause" );
					simlog_entry(msgbuf );
					sprintf(simmgr_shm->status.scenario.state, "Paused" );
					break;
				case ScenarioTerminate:	// Request from SIM II
					sprintf(simmgr_shm->status.scenario.state, "Terminate" );
					break;
				default:
					sprintf(simmgr_shm->status.scenario.state, "Unknown" );
					break;
			}
			sprintf(msgbuf, "State: %s ", simmgr_shm->status.scenario.state );
			log_message("", msgbuf ); 
		}
	}
	return ( rval );
}


/*
 * awrr_check
 *
 * Calculate awrr based on count of breaths, both manual and 'normal'
 *
 * 1 - Detect and log breath when either natural or manual start
 * 2 - If no breaths are recorded in the past 20 seconds (excluding the past 2 seconds) report AWRR as zero
 * 3 - Calculate AWRR based on the average time of the recorded breaths within the past 90 seconds, excluding the past 2 seconds
 *
*/
#define BREATH_CALC_LIMIT		4		// Max number of recorded breaths to count in calculation
#define BREATH_LOG_LEN	128
unsigned int breathLog[BREATH_LOG_LEN] = { 0, };
unsigned int breathLogNext = 0;

#define BREATH_LOG_STATE_IDLE	0
#define BREATH_LOG_STATE_DETECT	1
int breathLogState = BREATH_LOG_STATE_IDLE;

unsigned int breathLogLastNatural = 0;	// breathCount, last natural
int breathLogLastManual = 0;	// manual_count, last manual
unsigned int breathLogLast = 0;			// Time of last breath

#define BREATH_LOG_DELAY	(2)
int breathLogDelay = 0;

#define BREATH_LOG_CHANGE_LOOPS	1
int breathLogReportLoops = 0;

void
awrr_check(void)
{
	int now = simmgr_shm->server.msec_time; //  time(NULL);	// Current sec time
	int prev;
	int breaths;
	int totalTime;
	unsigned int lastTime;
	unsigned int firstTime;
	int diff;
	float awRR;
	int i;
	int intervals;
	int oldRate;
	int newRate;

	oldRate = simmgr_shm->status.respiration.awRR;
	
	// Breath Detect and Log
	if ( breathLogState == BREATH_LOG_STATE_DETECT )
	{
		// After detect, wait 400 msec (two calls) before another detect allowed
		if ( breathLogDelay++ >= BREATH_LOG_DELAY)
		{
			breathLogState = BREATH_LOG_STATE_IDLE;
			breathLogLastNatural = simmgr_shm->status.respiration.breathCount;
			breathLogLastManual = simmgr_shm->status.respiration.manual_count;
		}
	}
	else if ( breathLogState == BREATH_LOG_STATE_IDLE )
	{
		if ( breathLogLastNatural != simmgr_shm->status.respiration.breathCount )
		{
			breathLogState = BREATH_LOG_STATE_DETECT;
			breathLogLastNatural = simmgr_shm->status.respiration.breathCount;
			breathLogLast = now;
			breathLog[breathLogNext] = now;
			breathLogDelay = 0;
			breathLogNext += 1;
			if ( breathLogNext >= BREATH_LOG_LEN )
			{
				breathLogNext = 0;
			}
		}
		else if ( breathLogLastManual != simmgr_shm->status.respiration.manual_count )
		{
			breathLogState = BREATH_LOG_STATE_DETECT;
			breathLogLastManual = simmgr_shm->status.respiration.manual_count;
			breathLogLast = now;
			breathLog[breathLogNext] = now;
			breathLogDelay = 0;
			breathLogNext += 1;
			if ( breathLogNext >= BREATH_LOG_LEN )
			{
				breathLogNext = 0;
			}
		}
	}
	
	// AWRR Calculation - Look at no more than BREATH_CALC_LIMIT breaths - Skip if no breaths within 20 seconds
	lastTime = 0;
	firstTime = 0;
	prev = breathLogNext - 1;
	if ( prev < 0 ) 
	{
		prev = BREATH_LOG_LEN - 1;
	}
	breaths = 0;
	intervals = 0;
	
	lastTime = breathLog[prev];
	if ( lastTime <= 0 )  // Don't look at empty logs
	{
		simmgr_shm->status.respiration.awRR = 0;
	}
	else
	{
		diff = now - lastTime;
		if ( diff > 20000 )
		{
			simmgr_shm->status.respiration.awRR = 0;
		}
		else
		{
			prev -= 1;
			if ( prev < 0 ) 
			{
				prev = BREATH_LOG_LEN - 1;
			}
			for ( i = 0 ; i < BREATH_CALC_LIMIT ; i++ )
			{
				diff = now - breathLog[prev];
				if ( diff > 20000 ) // Over Limit seconds since this recorded breath
				{
					break;
				}
				else
				{
					firstTime = breathLog[prev]; // Recorded start of the first breath
					breaths += 1;
					intervals += 1;
					prev -= 1;
					if ( prev < 0 ) 
					{
						prev = BREATH_LOG_LEN - 1;
					}
				}
			}
		}
		if ( intervals > 0 )
		{
			totalTime = lastTime - firstTime;
			if ( totalTime == 0 )
			{
				awRR = 0;
			}
			else
			{
				awRR = ( ( ( (float)intervals / (float)totalTime ) ) * 60000 );
			}
			if ( awRR < 0 )
			{
				awRR = 0;
			}
			else if ( awRR > 60 )
			{
				awRR = 60;
			}
		}
		else
		{
			awRR = 0;
		}
		if ( breathLogReportLoops++ == BREATH_LOG_CHANGE_LOOPS )
		{
			breathLogReportLoops = 0;
			newRate = roundf(awRR );
			if ( oldRate != newRate )
			{
				// setRespirationPeriods(oldRate, newRate );
				simmgr_shm->status.respiration.awRR = newRate;
			}
		}
	}
}

int cprLast = 0;
int cprStartTime = 0;
int cprDuration = 2000;

void
cpr_check(void)
{
	int now = simmgr_shm->server.msec_time; 
	int cprCurrent = simmgr_shm->status.cpr.last;
	
	if ( cprCurrent != cprLast )
	{
		if ( cprCurrent == 0 )
		{
			simmgr_shm->status.cpr.running = 0;
			cprLast = cprCurrent;
		}
		else
		{
			cprStartTime = now;
			simmgr_shm->status.cpr.running = 1;
			cprLast = cprCurrent;
		}
	}
	if ( simmgr_shm->status.cpr.running > 0 )
	{
		if ( ( cprStartTime + cprDuration ) < now )
		{
			simmgr_shm->status.cpr.running = 0;
		}
	}
}
int shockLast = 0;
int shockStartTime = 0;
int shockDuration = 2000;

void
shock_check(void)
{
	int now = simmgr_shm->server.msec_time; 
	int shockCurrent = simmgr_shm->status.defibrillation.last;
	
	if ( shockCurrent != shockLast )
	{
		if ( shockCurrent == 0 )
		{
			simmgr_shm->status.defibrillation.shock = 0;
			shockLast = shockCurrent;
		}
		else
		{
			shockStartTime = now;
			simmgr_shm->status.defibrillation.shock = 1;
			shockLast = shockCurrent;
		}
	}
	if ( simmgr_shm->status.defibrillation.shock > 0 )
	{
		if ( ( shockStartTime + shockDuration ) < now )
		{
			simmgr_shm->status.defibrillation.shock = 0;
		}
	}
}
/*
 * hrcheck_handler
 *
 * Calculate heart rate based on count of beats, (normal, CPR and  VPC)
 *
 * 1 - Detect and log beat when any of natural, vpc or CPR
 * 2 - If no beats are recorded in the past 20 seconds (excluding the past 2 seconds) report rate as zero
 * 3 - Calculate beats based on the average time of the recorded beats within the past 90 seconds, excluding the past 2 seconds
 *
*/
#define HR_CALC_LIMIT		10		// Max number of recorded beats to count in calculation
#define HR_LOG_LEN	128
unsigned int hrLog[HR_LOG_LEN] = { 0, };
int hrLogNext = 0;

unsigned int hrLogLastNatural = 0;	// beatCount, last natural
unsigned int hrLogLastVPC = 0;	// VPC count, last VPC

#define HR_LOG_DELAY	(40)
int hrLogDelay = 0;

#define HR_LOG_CHANGE_LOOPS	50	// hr_check is called every 5 msec. So this will cause a recalc every 250 msec
int hrLogReportLoops = 0;

static void 
hrcheck_handler(int sig, siginfo_t *si, void *uc)
{
#if 0
	simmgr_shm->status.cardiac.avg_rate = simmgr_shm->status.cardiac.rate;
#else
	int now; // Current msec time
	int prev;
	int beats;
	int totalTime;
	unsigned int lastTime;
	int firstTime;
	int diff;
	float avg_rate;
	float seconds;
	float minutes;
	int i;
	int intervals;
	/*
	int oldRate;
	int newRate;
	*/
	int newBeat = 0;
	
	now = msec_time_update();
	
	if ( hrLogLastNatural != simmgr_shm->status.cardiac.pulseCount )
	{
		hrLogLastNatural = simmgr_shm->status.cardiac.pulseCount;
		newBeat = 1;
	}
	else if ( hrLogLastVPC != simmgr_shm->status.cardiac.pulseCountVpc )
	{
		hrLogLastVPC = simmgr_shm->status.cardiac.pulseCountVpc;
		newBeat = 1;
	}

	if ( newBeat )
	{
		prev = hrLogNext;
		hrLog[hrLogNext] = now;
		hrLogNext += 1;
		if ( hrLogNext >= HR_LOG_LEN )
		{
			hrLogNext = 0;
		}
	}
	else
	{
		prev = hrLogNext - 1;
		if ( prev < 0 ) 
		{
			prev = (HR_LOG_LEN - 1);
		}
	}
	if ( hrLogReportLoops++ >= HR_LOG_CHANGE_LOOPS )
	{
		hrLogReportLoops = 0;
		// AVG Calculation - Look at no more than 10 beats - Skip if no beats within 20 seconds
		lastTime = 0;
		firstTime = 0;
		
		beats = 1;
		intervals = 0;
		
		lastTime = hrLog[prev];
		if ( lastTime < 0 )  // Don't look at empty logs
		{
			simmgr_shm->status.cardiac.avg_rate = 0;
		}
		else if ( lastTime == 0 )  // Don't look at empty logs
		{
			simmgr_shm->status.cardiac.avg_rate = 0;
		}
		else
		{
			diff = now - lastTime;
			if ( diff > 20000 )
			{
				simmgr_shm->status.cardiac.avg_rate = 0;
			}
			else
			{
				prev -= 1;
				if ( prev < 0 ) 
				{
					prev = (HR_LOG_LEN - 1);
				}
				for ( i = 0 ; i < HR_CALC_LIMIT ; i++ )
				{
					diff = now - hrLog[prev];
					if ( diff > 20000 ) // Over Limit seconds since this recorded beat
					{
						break;
					}
					else
					{
						firstTime = hrLog[prev]; // Recorded start of the first beat
						beats += 1;
						intervals += 1;
						prev -= 1;
						if ( prev < 0 ) 
						{
							prev = HR_LOG_LEN - 1;
						}
					}
				}
			}
			
			if ( intervals > 0 )
			{
				totalTime = lastTime - firstTime;
				if ( totalTime == 0 )
				{
					avg_rate = 0;
				}
				else
				{
					seconds = (float)totalTime / 1000;
					minutes = seconds / 60;
					avg_rate = (float)intervals / minutes;
				}
				if ( avg_rate < 0 )
				{
					avg_rate = 0;
				}
				else if ( avg_rate > 360 )
				{
					avg_rate = 360;
				}
			}
			else
			{
				avg_rate = 0;
			}
			simmgr_shm->status.cardiac.avg_rate = round(avg_rate );
		}
	}
#endif
}
/*
 * msec_timer_update
*/
int
msec_time_update(void )
{
	int msec;
	struct timeval tv;
	int sts;
	
	sts = gettimeofday(&tv, NULL );
	if ( sts )
	{
		msec = 0;
	}
	else
	{
		msec = ( tv.tv_sec * 1000 ) + ( tv.tv_usec / 1000) ;
	}
	simmgr_shm->server.msec_time = msec;
	return ( msec );
}
/*
 * time_update
 *
 * Get the localtime and write it as a string to the SHM data
 */ 
int last_time_sec = -1;

void
time_update(void )
{
	time_t the_time;
	struct tm tm;
	int hour;
	int min;
	int elapsedTimeSeconds;
	int seconds;
	int sec;
	
	the_time = time(NULL );
	(void)localtime_r(&the_time, &tm );
	(void)asctime_r(&tm, buf );
	strtok(buf, "\n");		// Remove Line Feed
	sprintf(simmgr_shm->server.server_time, "%s", buf );
	
	now = std::time(nullptr );
	elapsedTimeSeconds = (int)difftime(now, scenario_start_time );
	
	if ( ( scenario_state == ScenarioRunning ) || 
		 ( scenario_state == ScenarioPaused ) )
	{
		sec = elapsedTimeSeconds;
		min = (sec / 60);
		hour = min / 60;
		sprintf(simmgr_shm->status.scenario.runtimeAbsolute, "%02d:%02d:%02d", hour, min%60, sec%60);
	}
	if ( ( elapsedTimeSeconds > MAX_SCENARIO_RUNTIME ) &&
		 ( ( scenario_state == ScenarioRunning ) || 
		   ( scenario_state == ScenarioPaused ) ) )
	{
		sprintf(buf, "MAX Scenario Runtime exceeded. Terminating." );
		simlog_entry(buf );
		
		takeInstructorLock();
		sprintf(simmgr_shm->instructor.scenario.state, "%s", "Terminate" );
		releaseInstructorLock();
	}
	else if ( scenario_state == ScenarioRunning )
	{
		sec = simmgr_shm->status.scenario.elapsed_msec_scenario / 1000;
		min = (sec / 60);
		hour = min / 60;
		sprintf(simmgr_shm->status.scenario.runtimeScenario, "%02d:%02d:%02d", hour, min%60, sec%60);
		
		sec = simmgr_shm->status.scenario.elapsed_msec_scene / 1000;
		min = (sec / 60);
		hour = min / 60;
		sprintf(simmgr_shm->status.scenario.runtimeScene, "%02d:%02d:%02d", hour, min%60, sec%60);
		
		seconds = elapsedTimeSeconds % 60;
		if ( ( seconds == 0 ) && ( last_time_sec != 0 ) )
		{
			// Do periodic Stats update every minute
			sprintf(buf, "VS: Temp: %0.1f; RR: %d; awRR: %d; HR: %d; %s; BP: %d/%d; SPO2: %d; etCO2: %d mmHg; Probes: ECG: %s; BP: %s; SPO2: %s; ETCO2: %s; Temp %s",
				((double)simmgr_shm->status.general.temperature) / 10,
				simmgr_shm->status.respiration.rate,
				simmgr_shm->status.respiration.awRR,
				simmgr_shm->status.cardiac.rate,
				(simmgr_shm->status.cardiac.arrest == 1 ? "Arrest" : "Normal"  ),
				simmgr_shm->status.cardiac.bps_sys,
				simmgr_shm->status.cardiac.bps_dia,
				simmgr_shm->status.respiration.spo2,
				simmgr_shm->status.respiration.etco2,
				(simmgr_shm->status.cardiac.ecg_indicator == 1 ? "on" : "off"  ),
				(simmgr_shm->status.cardiac.bp_cuff == 1 ? "on" : "off"  ),
				(simmgr_shm->status.respiration.spo2_indicator == 1 ? "on" : "off"  ),
				(simmgr_shm->status.respiration.etco2_indicator == 1 ? "on" : "off"  ),
				(simmgr_shm->status.general.temperature_enable == 1 ? "on" : "off"  )
			);
			simlog_entry(buf );
		}
		last_time_sec = seconds;
	}
	else if ( scenario_state == ScenarioStopped )
	{
		last_time_sec = -1;
	}
}
/*
 * comm_check
 *
 * verify that the communications path to the SimCtl is open and ok.
 * If not, try to reestablish.
 */
void
comm_check(void )
{
	// TBD
}


/*
 * Cardiac Process
 *
 * Based on the rate and target selected, modify the pulse rate
 */
struct trend cardiacTrend;
struct trend respirationTrend;
struct trend sysTrend;
struct trend diaTrend;
struct trend tempTrend;
struct trend spo2Trend;
struct trend etco2Trend;

int 
clearTrend(struct trend *trend, int current )
{
	trend->end = current;
	trend->current = current;
	
	return ( trend->current );
}

void
clearAllTrends(void )
{
	// Clear running trends
	(void)clearTrend(&cardiacTrend, simmgr_shm->status.cardiac.rate );
	(void)clearTrend(&sysTrend, simmgr_shm->status.cardiac.bps_sys  );
	(void)clearTrend(&diaTrend, simmgr_shm->status.cardiac.bps_dia  );
	(void)clearTrend(&respirationTrend, simmgr_shm->status.respiration.rate );
	(void)clearTrend(&spo2Trend, simmgr_shm->status.respiration.spo2 );
	(void)clearTrend(&etco2Trend, simmgr_shm->status.respiration.etco2 );
	(void)clearTrend(&tempTrend, simmgr_shm->status.general.temperature );
}

/*
 * duration is in seconds
*/


int 
setTrend(struct trend *trend, int end, int current, int duration )
{
	double diff;
	
	trend->end = (double)end;
	diff = (double)abs(end - current);

	if ( ( duration > 0 ) && ( diff > 0 ) )
	{
		trend->current = (double)current;
		trend->changePerSecond = diff / duration;
		trend->nextTime = time(0) + 1;
	}
	else
	{
		trend->current = end;
		trend->changePerSecond = 0;
		trend->nextTime = 0;
	}
	return ( (int)trend->current );
}

int
trendProcess(struct trend *trend )
{
	time_t now;
	double newval;
	int rval;
	
	now = time(0);
	
	if ( trend->nextTime && ( trend->nextTime <= now ) )
	{
		if ( trend->end > trend->current )
		{
			newval = trend->current + trend->changePerSecond;
			if ( newval > trend->end )
			{
				newval = trend->end;
			}
		}
		else
		{
			newval = trend->current - trend->changePerSecond;
			if ( newval < trend->end )
			{
				newval = trend->end;
			}
		}
		trend->current = newval;
		if ( trend->current == trend->end )
		{
			trend->nextTime = 0;
		}
		else
		{
			trend->nextTime = now + 1;
		}
	}
	rval = (int)round(trend->current );
	return ( rval );
}

int
isRhythmPulsed(char *rhythm ) 
{
	if ( ( strcmp(rhythm, "asystole" ) == 0 ) ||
		 ( strcmp(rhythm, "vfib" ) == 0 ) )
	{
		return ( false );
	}
	else
	{
		return ( true );
	}
}


void
setRespirationPeriods(int oldRate, int newRate )
{
	int period;
	
	//if ( oldRate != newRate )
	{
		if ( newRate > 0 )
		{
			simmgr_shm->status.respiration.rate = newRate;
			period = (1000*60)/newRate;	// Period in msec from rate per minute
			if ( period > 10000 )
			{
				period = 10000;
			}
			period = (period * 7) / 10;	// Use 70% of the period for duration calculations
			simmgr_shm->status.respiration.inhalation_duration = period / 2;
			simmgr_shm->status.respiration.exhalation_duration = period - simmgr_shm->status.respiration.inhalation_duration;
		}
		else
		{
			simmgr_shm->status.respiration.rate = 0;
			simmgr_shm->status.respiration.inhalation_duration = 0;
			simmgr_shm->status.respiration.exhalation_duration = 0;
		}
	}
}
	
/*
 * Scan commands from Initiator Interface
 *
 * Reads II commands and changes operating parameters
 *
 * Note: Events are added to the Event List directly by the source initiators and read
 * by the scenario process. Events are not handled here.
 */
int
scan_commands(void )
{
	int sts;
	int trycount;
	int oldRate;
	int newRate;
	int currentIsPulsed;
	int newIsPulsed;
	// Lock the command interface before processing commands
	trycount = 0;
	while ( ( sts = sem_trywait(&simmgr_shm->instructor.sema) ) != 0 )
	{
		if ( trycount++ > 50 )
		{
			// Could not get lock soon enough. Try again next time.
			return ( -1 );
		}
		usleep(100000 );
	}
	iiLockTaken = 1;
	
	// Check for instructor commands
	
	// Scenario
	if ( simmgr_shm->instructor.scenario.record >= 0 )
	{
		simmgr_shm->status.scenario.record = simmgr_shm->instructor.scenario.record;
		simmgr_shm->instructor.scenario.record = -1;
	}
	
	if ( strlen(simmgr_shm->instructor.scenario.state ) > 0 )
	{
		strToLower(simmgr_shm->instructor.scenario.state );
		
		sprintf(msgbuf, "State Request: %s Current %s State %d", 
			simmgr_shm->instructor.scenario.state,
			simmgr_shm->status.scenario.state,
			scenario_state );
		log_message("", msgbuf ); 
		
		if ( strcmp(simmgr_shm->instructor.scenario.state, "paused" ) == 0 )
		{
			if ( scenario_state == ScenarioRunning )
			{
				updateScenarioState(ScenarioPaused );
			}
		}
		else if ( strcmp(simmgr_shm->instructor.scenario.state, "running" ) == 0 )
		{
			if ( scenario_state == ScenarioPaused )
			{
				updateScenarioState(ScenarioRunning );
			}
			else if ( scenario_state == ScenarioStopped )
			{
				sts = start_scenario(simmgr_shm->status.scenario.active );
			}
		}
		else if ( strcmp(simmgr_shm->instructor.scenario.state, "terminate" ) == 0 )
		{
			if ( scenario_state != ScenarioTerminate )
			{
				updateScenarioState(ScenarioTerminate );
			}
		}
		else if ( strcmp(simmgr_shm->instructor.scenario.state, "stopped" ) == 0 )
		{
			if ( scenario_state != ScenarioStopped )
			{
				updateScenarioState(ScenarioStopped );
			}
		}
		sprintf(simmgr_shm->instructor.scenario.state, "%s", "" );
	}
	if ( strlen(simmgr_shm->instructor.scenario.active) > 0 )
	{
		sprintf(msgbuf, "Set Active: %s State %d", simmgr_shm->instructor.scenario.active, scenario_state );
		log_message("", msgbuf ); 
		switch ( scenario_state )
		{
			case ScenarioTerminate:
			default:
				break;
			case ScenarioStopped:
				sprintf(simmgr_shm->status.scenario.active, "%s", simmgr_shm->instructor.scenario.active );
				break;
		}
		sprintf(simmgr_shm->instructor.scenario.active, "%s", "" );
	}
	
	// Cardiac
	if ( strlen(simmgr_shm->instructor.cardiac.rhythm ) > 0 )
	{
		if ( strcmp(simmgr_shm->status.cardiac.rhythm, simmgr_shm->instructor.cardiac.rhythm ) != 0 )
		{
			// When changing to pulseless rhythm, the rate will be set to zero.
			newIsPulsed = isRhythmPulsed(simmgr_shm->instructor.cardiac.rhythm );
			if ( newIsPulsed == false )
			{
				simmgr_shm->instructor.cardiac.rate = 0;
				simmgr_shm->instructor.cardiac.transfer_time = 0;
			}
			else
			{
				// When changing from a pulseless rhythm to a pulse rhythm, the rate will be set to 100
				// This can be overridden in the command by setting the rate explicitly
				currentIsPulsed = isRhythmPulsed(simmgr_shm->status.cardiac.rhythm );
				if ( ( currentIsPulsed == false ) && ( newIsPulsed == true ) )
				{
					if ( simmgr_shm->instructor.cardiac.rate < 0 )
					{
						simmgr_shm->instructor.cardiac.rate = 100;
						simmgr_shm->instructor.cardiac.transfer_time = 0;
					}
				}
			}
			sprintf(simmgr_shm->status.cardiac.rhythm, "%s", simmgr_shm->instructor.cardiac.rhythm );
			sprintf(buf, "%s: %s", "Cardiac Rhythm", simmgr_shm->instructor.cardiac.rhythm );
			simlog_entry(buf );
		}
		sprintf(simmgr_shm->instructor.cardiac.rhythm, "%s", "" );
		
	}
	if ( simmgr_shm->instructor.cardiac.rate >= 0 )
	{
		currentIsPulsed = isRhythmPulsed(simmgr_shm->status.cardiac.rhythm );
		if ( currentIsPulsed == true )
		{
			if ( simmgr_shm->instructor.cardiac.rate != simmgr_shm->status.cardiac.rate )
			{
				simmgr_shm->status.cardiac.rate = setTrend(&cardiacTrend, 
												simmgr_shm->instructor.cardiac.rate,
												simmgr_shm->status.cardiac.rate,
												simmgr_shm->instructor.cardiac.transfer_time );
				if ( simmgr_shm->instructor.cardiac.transfer_time >= 0 )
				{
					sprintf(buf, "%s: %d time %d", "Cardiac Rate", simmgr_shm->instructor.cardiac.rate, simmgr_shm->instructor.cardiac.transfer_time );
				}
				else
				{
					sprintf(buf, "%s: %d", "Cardiac Rate", simmgr_shm->instructor.cardiac.rate );
				}
				simlog_entry(buf );
			}
		}
		else 
		{
			if ( simmgr_shm->instructor.cardiac.rate > 0 )
			{
				sprintf(buf, "%s: %d", "Cardiac Rate cannot be set while in pulseless rhythm", simmgr_shm->instructor.cardiac.rate );
				simlog_entry(buf );
			}
			else
			{
				simmgr_shm->status.cardiac.rate = setTrend(&cardiacTrend, 
												0,
												simmgr_shm->status.cardiac.rate,
												0 );
			}
		}
		simmgr_shm->instructor.cardiac.rate = -1;
	}
	if ( simmgr_shm->instructor.cardiac.nibp_rate >= 0 )
	{
		if ( simmgr_shm->status.cardiac.nibp_rate != simmgr_shm->instructor.cardiac.nibp_rate )
		{
			simmgr_shm->status.cardiac.nibp_rate = simmgr_shm->instructor.cardiac.nibp_rate;
			sprintf(buf, "%s: %d", "NIBP Rate", simmgr_shm->instructor.cardiac.rate );
			simlog_entry(buf );
		}
		simmgr_shm->instructor.cardiac.nibp_rate = -1;
	}
	if ( simmgr_shm->instructor.cardiac.nibp_read >= 0 )
	{
		if ( simmgr_shm->status.cardiac.nibp_read != simmgr_shm->instructor.cardiac.nibp_read )
		{
			simmgr_shm->status.cardiac.nibp_read = simmgr_shm->instructor.cardiac.nibp_read;
		}
		simmgr_shm->instructor.cardiac.nibp_read = -1;
	}
	if ( simmgr_shm->instructor.cardiac.nibp_linked_hr >= 0 )
	{
		if ( simmgr_shm->status.cardiac.nibp_linked_hr != simmgr_shm->instructor.cardiac.nibp_linked_hr )
		{
			simmgr_shm->status.cardiac.nibp_linked_hr = simmgr_shm->instructor.cardiac.nibp_linked_hr;
		}
		simmgr_shm->instructor.cardiac.nibp_linked_hr = -1;
	}
	if ( simmgr_shm->instructor.cardiac.nibp_freq >= 0 )
	{
		if ( simmgr_shm->status.cardiac.nibp_freq != simmgr_shm->instructor.cardiac.nibp_freq )
		{
			simmgr_shm->status.cardiac.nibp_freq = simmgr_shm->instructor.cardiac.nibp_freq;
			if ( nibp_state == NibpWaiting ) // Cancel current wait and allow reset to new rate
			{
				nibp_state = NibpIdle;
			}
		}
		simmgr_shm->instructor.cardiac.nibp_freq = -1;
	}
	if ( strlen(simmgr_shm->instructor.cardiac.pwave ) > 0 )
	{
		sprintf(simmgr_shm->status.cardiac.pwave, "%s", simmgr_shm->instructor.cardiac.pwave );
		sprintf(simmgr_shm->instructor.cardiac.pwave, "%s", "" );
	}
	if ( simmgr_shm->instructor.cardiac.pr_interval >= 0 )
	{
		simmgr_shm->status.cardiac.pr_interval = simmgr_shm->instructor.cardiac.pr_interval;
		simmgr_shm->instructor.cardiac.pr_interval = -1;
	}
	if ( simmgr_shm->instructor.cardiac.qrs_interval >= 0 )
	{
		simmgr_shm->status.cardiac.qrs_interval = simmgr_shm->instructor.cardiac.qrs_interval;
		simmgr_shm->instructor.cardiac.qrs_interval = -1;
	}
	if ( simmgr_shm->instructor.cardiac.qrs_interval >= 0 )
	{
		simmgr_shm->status.cardiac.qrs_interval = simmgr_shm->instructor.cardiac.qrs_interval;
		simmgr_shm->instructor.cardiac.qrs_interval = -1;
	}
	if ( simmgr_shm->instructor.cardiac.bps_sys >= 0 )
	{
		simmgr_shm->status.cardiac.bps_sys = setTrend(&sysTrend, 
											simmgr_shm->instructor.cardiac.bps_sys,
											simmgr_shm->status.cardiac.bps_sys,
											simmgr_shm->instructor.cardiac.transfer_time );
		simmgr_shm->instructor.cardiac.bps_sys = -1;
	}
	if ( simmgr_shm->instructor.cardiac.bps_dia >= 0 )
	{
		simmgr_shm->status.cardiac.bps_dia = setTrend(&diaTrend, 
											simmgr_shm->instructor.cardiac.bps_dia,
											simmgr_shm->status.cardiac.bps_dia,
											simmgr_shm->instructor.cardiac.transfer_time );
		simmgr_shm->instructor.cardiac.bps_dia = -1;
	}
	if ( simmgr_shm->instructor.cardiac.pea >= 0 )
	{
		simmgr_shm->status.cardiac.pea = simmgr_shm->instructor.cardiac.pea;
		simmgr_shm->instructor.cardiac.pea = -1;
	}	
	if ( simmgr_shm->instructor.cardiac.right_dorsal_pulse_strength >= 0 )
	{
		simmgr_shm->status.cardiac.right_dorsal_pulse_strength = simmgr_shm->instructor.cardiac.right_dorsal_pulse_strength;
		simmgr_shm->instructor.cardiac.right_dorsal_pulse_strength = -1;
	}
	if ( simmgr_shm->instructor.cardiac.right_femoral_pulse_strength >= 0 )
	{
		simmgr_shm->status.cardiac.right_femoral_pulse_strength = simmgr_shm->instructor.cardiac.right_femoral_pulse_strength;
		simmgr_shm->instructor.cardiac.right_femoral_pulse_strength = -1;
	}
	if ( simmgr_shm->instructor.cardiac.left_dorsal_pulse_strength >= 0 )
	{
		simmgr_shm->status.cardiac.left_dorsal_pulse_strength = simmgr_shm->instructor.cardiac.left_dorsal_pulse_strength;
		simmgr_shm->instructor.cardiac.left_dorsal_pulse_strength = -1;
	}
	if ( simmgr_shm->instructor.cardiac.left_femoral_pulse_strength >= 0 )
	{
		simmgr_shm->status.cardiac.left_femoral_pulse_strength = simmgr_shm->instructor.cardiac.left_femoral_pulse_strength;
		simmgr_shm->instructor.cardiac.left_femoral_pulse_strength = -1;
	}
	if ( simmgr_shm->instructor.cardiac.vpc_freq >= 0 )
	{
		simmgr_shm->status.cardiac.vpc_freq = simmgr_shm->instructor.cardiac.vpc_freq;
		simmgr_shm->instructor.cardiac.vpc_freq = -1;
	}
	/*
	if ( simmgr_shm->instructor.cardiac.vpc_delay >= 0 )
	{
		simmgr_shm->status.cardiac.vpc_delay = simmgr_shm->instructor.cardiac.vpc_delay;
		simmgr_shm->instructor.cardiac.vpc_delay = -1;
	}
	*/
	if ( strlen(simmgr_shm->instructor.cardiac.vpc) > 0 )
	{
		sprintf(simmgr_shm->status.cardiac.vpc, "%s", simmgr_shm->instructor.cardiac.vpc );
		sprintf(simmgr_shm->instructor.cardiac.vpc, "%s", "" );
		switch ( simmgr_shm->status.cardiac.vpc[0] )
		{
			case '1':
				simmgr_shm->status.cardiac.vpc_type = 1;
				break;
			case '2':
				simmgr_shm->status.cardiac.vpc_type = 2;
				break;
			default:
				simmgr_shm->status.cardiac.vpc_type = 0;
				break;
		}
		switch ( simmgr_shm->status.cardiac.vpc[2] )
		{
			case '1':
				simmgr_shm->status.cardiac.vpc_count = 1;
				break;
			case '2':
				simmgr_shm->status.cardiac.vpc_count = 2;
				break;
			case '3':
				simmgr_shm->status.cardiac.vpc_count = 3;
				break;
			default:
				simmgr_shm->status.cardiac.vpc_count = 0;
				simmgr_shm->status.cardiac.vpc_type = 0;
				break;
		}
	}
	if ( strlen(simmgr_shm->instructor.cardiac.vfib_amplitude) > 0 )
	{
		sprintf(simmgr_shm->status.cardiac.vfib_amplitude, "%s", simmgr_shm->instructor.cardiac.vfib_amplitude );
		sprintf(simmgr_shm->instructor.cardiac.vfib_amplitude, "%s", "" );
	}
	if ( strlen(simmgr_shm->instructor.cardiac.heart_sound) > 0 )
	{
		sprintf(simmgr_shm->status.cardiac.heart_sound, "%s", simmgr_shm->instructor.cardiac.heart_sound );
		sprintf(simmgr_shm->instructor.cardiac.heart_sound, "%s", "" );
	}
	if ( simmgr_shm->instructor.cardiac.heart_sound_volume >= 0 )
	{
		simmgr_shm->status.cardiac.heart_sound_volume = simmgr_shm->instructor.cardiac.heart_sound_volume;
		simmgr_shm->instructor.cardiac.heart_sound_volume = -1;
	}
	if ( simmgr_shm->instructor.cardiac.heart_sound_mute >= 0 )
	{
		simmgr_shm->status.cardiac.heart_sound_mute = simmgr_shm->instructor.cardiac.heart_sound_mute;
		simmgr_shm->instructor.cardiac.heart_sound_mute = -1;
	}
	
	if ( simmgr_shm->instructor.cardiac.ecg_indicator >= 0 )
	{
		if ( simmgr_shm->status.cardiac.ecg_indicator != simmgr_shm->instructor.cardiac.ecg_indicator )
		{
			simmgr_shm->status.cardiac.ecg_indicator = simmgr_shm->instructor.cardiac.ecg_indicator;
			sprintf(buf, "%s %s", "ECG Probe", (simmgr_shm->status.cardiac.ecg_indicator == 1 ? "Attached": "Removed") );
			simlog_entry(buf );
		}
		simmgr_shm->instructor.cardiac.ecg_indicator = -1;
	}
	if ( simmgr_shm->instructor.cardiac.bp_cuff >= 0 )
	{
		if ( simmgr_shm->status.cardiac.bp_cuff != simmgr_shm->instructor.cardiac.bp_cuff )
		{
			simmgr_shm->status.cardiac.bp_cuff = simmgr_shm->instructor.cardiac.bp_cuff;
			sprintf(buf, "%s %s", "BP Cuff", (simmgr_shm->status.cardiac.bp_cuff == 1 ? "Attached": "Removed") );
			simlog_entry(buf );
		}
		simmgr_shm->instructor.cardiac.bp_cuff = -1;
	}
	if ( simmgr_shm->instructor.cardiac.arrest >= 0 )
	{
		if ( simmgr_shm->status.cardiac.arrest != simmgr_shm->instructor.cardiac.arrest )
		{
			simmgr_shm->status.cardiac.arrest = simmgr_shm->instructor.cardiac.arrest;
			sprintf(buf, "%s %s", "Arrest", (simmgr_shm->status.cardiac.arrest == 1 ? "Start": "Stop") );
			simlog_entry(buf );
		}
		simmgr_shm->instructor.cardiac.arrest = -1;
	}
	simmgr_shm->instructor.cardiac.transfer_time = -1;
	
	// Respiration
	if ( strlen(simmgr_shm->instructor.respiration.left_lung_sound) > 0 )
	{
		sprintf(simmgr_shm->status.respiration.left_lung_sound, "%s", simmgr_shm->instructor.respiration.left_lung_sound );
		sprintf(simmgr_shm->instructor.respiration.left_lung_sound, "%s", "" );
	}
	if ( strlen(simmgr_shm->instructor.respiration.right_lung_sound ) > 0 )
	{
		sprintf(simmgr_shm->status.respiration.right_lung_sound, "%s", simmgr_shm->instructor.respiration.right_lung_sound );
		sprintf(simmgr_shm->instructor.respiration.right_lung_sound, "%s", "" );
	}
	/*
	if ( simmgr_shm->instructor.respiration.inhalation_duration >= 0 )
	{
		simmgr_shm->status.respiration.inhalation_duration = simmgr_shm->instructor.respiration.inhalation_duration;
		simmgr_shm->instructor.respiration.inhalation_duration = -1;
	}
	if ( simmgr_shm->instructor.respiration.exhalation_duration >= 0 )
	{
		simmgr_shm->status.respiration.exhalation_duration = simmgr_shm->instructor.respiration.exhalation_duration;
		simmgr_shm->instructor.respiration.exhalation_duration = -1;
	}
	*/
	if ( simmgr_shm->instructor.respiration.left_lung_sound_volume >= 0 )
	{
		simmgr_shm->status.respiration.left_lung_sound_volume = simmgr_shm->instructor.respiration.left_lung_sound_volume;
		simmgr_shm->instructor.respiration.left_lung_sound_volume = -1;
	}
	if ( simmgr_shm->instructor.respiration.left_lung_sound_mute >= 0 )
	{
		simmgr_shm->status.respiration.left_lung_sound_mute = simmgr_shm->instructor.respiration.left_lung_sound_mute;
		simmgr_shm->instructor.respiration.left_lung_sound_mute = -1;
	}
	if ( simmgr_shm->instructor.respiration.right_lung_sound_volume >= 0 )
	{
		simmgr_shm->status.respiration.right_lung_sound_volume = simmgr_shm->instructor.respiration.right_lung_sound_volume;
		simmgr_shm->instructor.respiration.right_lung_sound_volume = -1;
	}
	if ( simmgr_shm->instructor.respiration.right_lung_sound_mute >= 0 )
	{
		simmgr_shm->status.respiration.right_lung_sound_mute = simmgr_shm->instructor.respiration.right_lung_sound_mute;
		simmgr_shm->instructor.respiration.right_lung_sound_mute = -1;
	}
	if ( simmgr_shm->instructor.respiration.rate >= 0 )
	{
		sprintf(msgbuf, "Set Resp Rate = %d -> %d : %d", simmgr_shm->status.respiration.rate, simmgr_shm->instructor.respiration.rate, simmgr_shm->instructor.respiration.transfer_time );
		log_message("", msgbuf);
		simmgr_shm->status.respiration.rate = setTrend(&respirationTrend, 
												simmgr_shm->instructor.respiration.rate,
												simmgr_shm->status.respiration.rate,
												simmgr_shm->instructor.respiration.transfer_time );
		if ( simmgr_shm->instructor.respiration.transfer_time == 0 )
		{
			setRespirationPeriods(simmgr_shm->status.respiration.rate, simmgr_shm->instructor.respiration.rate );
		}
		simmgr_shm->instructor.respiration.rate = -1;
		simmgr_shm->instructor.respiration.transfer_time = -1;
	}
	
	if ( simmgr_shm->instructor.respiration.spo2 >= 0 )
	{
		simmgr_shm->status.respiration.spo2 = setTrend(&spo2Trend, 
											simmgr_shm->instructor.respiration.spo2,
											simmgr_shm->status.respiration.spo2,
											simmgr_shm->instructor.respiration.transfer_time );
		simmgr_shm->instructor.respiration.spo2 = -1;
	}
	
	if ( simmgr_shm->instructor.respiration.etco2 >= 0 )
	{
		simmgr_shm->status.respiration.etco2 = setTrend(&etco2Trend, 
											simmgr_shm->instructor.respiration.etco2,
											simmgr_shm->status.respiration.etco2,
											simmgr_shm->instructor.respiration.transfer_time );
		simmgr_shm->instructor.respiration.etco2 = -1;
	}
	if ( simmgr_shm->instructor.respiration.etco2_indicator >= 0 )
	{
		if ( simmgr_shm->status.respiration.etco2_indicator != simmgr_shm->instructor.respiration.etco2_indicator )
		{
			simmgr_shm->status.respiration.etco2_indicator = simmgr_shm->instructor.respiration.etco2_indicator;
			sprintf(buf, "%s %s", "ETCO2 Probe", (simmgr_shm->status.respiration.etco2_indicator == 1 ? "Attached": "Removed") );
			simlog_entry(buf );
		}
		
		simmgr_shm->instructor.respiration.etco2_indicator = -1;
	}
	if ( simmgr_shm->instructor.respiration.spo2_indicator >= 0 )
	{
		if ( simmgr_shm->status.respiration.spo2_indicator != simmgr_shm->instructor.respiration.spo2_indicator )
		{
			simmgr_shm->status.respiration.spo2_indicator = simmgr_shm->instructor.respiration.spo2_indicator;
			sprintf(buf, "%s %s", "SPO2 Probe", (simmgr_shm->status.respiration.spo2_indicator == 1 ? "Attached": "Removed") );
			simlog_entry(buf );
		}
		simmgr_shm->instructor.respiration.spo2_indicator = -1;
	}
	if ( simmgr_shm->instructor.respiration.chest_movement >= 0 )
	{
		if ( simmgr_shm->status.respiration.chest_movement != simmgr_shm->instructor.respiration.chest_movement )
		{
			simmgr_shm->status.respiration.chest_movement = simmgr_shm->instructor.respiration.chest_movement;
		}
		simmgr_shm->instructor.respiration.chest_movement = -1;
	}
	if ( simmgr_shm->instructor.respiration.manual_breath >= 0 )
	{
		simmgr_shm->status.respiration.manual_count++;
		simmgr_shm->instructor.respiration.manual_breath = -1;
	}
	simmgr_shm->instructor.respiration.transfer_time = -1;
	
	// General
	if ( simmgr_shm->instructor.general.temperature >= 0 )
	{
		simmgr_shm->status.general.temperature = setTrend(&tempTrend, 
											simmgr_shm->instructor.general.temperature,
											simmgr_shm->status.general.temperature,
											simmgr_shm->instructor.general.transfer_time );
		simmgr_shm->instructor.general.temperature = -1;
	}
	if ( simmgr_shm->instructor.general.temperature_enable >= 0 )
	{
		if ( simmgr_shm->status.general.temperature_enable != simmgr_shm->instructor.general.temperature_enable )
		{
			simmgr_shm->status.general.temperature_enable = simmgr_shm->instructor.general.temperature_enable;
			sprintf(buf, "%s %s", "Temp Probe", (simmgr_shm->status.general.temperature_enable == 1 ? "Attached": "Removed") );
			simlog_entry(buf );
		}
		simmgr_shm->instructor.general.temperature_enable = -1;
	}
	simmgr_shm->instructor.general.transfer_time = -1;
	
	// vocals
	if ( strlen(simmgr_shm->instructor.vocals.filename) > 0 )
	{
		sprintf(simmgr_shm->status.vocals.filename, "%s", simmgr_shm->instructor.vocals.filename );
		sprintf(simmgr_shm->instructor.vocals.filename, "%s", "" );
	}
	if ( simmgr_shm->instructor.vocals.repeat >= 0 )
	{
		simmgr_shm->status.vocals.repeat = simmgr_shm->instructor.vocals.repeat;
		simmgr_shm->instructor.vocals.repeat = -1;
	}
	if ( simmgr_shm->instructor.vocals.volume >= 0 )
	{
		simmgr_shm->status.vocals.volume = simmgr_shm->instructor.vocals.volume;
		simmgr_shm->instructor.vocals.volume = -1;
	}
	if ( simmgr_shm->instructor.vocals.play >= 0 )
	{
		simmgr_shm->status.vocals.play = simmgr_shm->instructor.vocals.play;
		simmgr_shm->instructor.vocals.play = -1;
	}
	if ( simmgr_shm->instructor.vocals.mute >= 0 )
	{
		simmgr_shm->status.vocals.mute = simmgr_shm->instructor.vocals.mute;
		simmgr_shm->instructor.vocals.mute = -1;
	}
	
	// media
	if ( strlen(simmgr_shm->instructor.media.filename) > 0 )
	{
		sprintf(simmgr_shm->status.media.filename, "%s", simmgr_shm->instructor.media.filename );
		sprintf(simmgr_shm->instructor.media.filename, "%s", "" );
	}
	if ( simmgr_shm->instructor.media.play != -1 )
	{
		simmgr_shm->status.media.play = simmgr_shm->instructor.media.play;
		simmgr_shm->instructor.media.play = -1;
	}
	
	// CPR
	if ( simmgr_shm->instructor.cpr.compression >= 0 )
	{
		simmgr_shm->status.cpr.compression = simmgr_shm->instructor.cpr.compression;
		if ( simmgr_shm->status.cpr.compression )
		{
			simmgr_shm->status.cpr.last = simmgr_shm->server.msec_time;
			simmgr_shm->status.cpr.running = 1;
		}
		simmgr_shm->instructor.cpr.compression = -1;
	}
	// Defibbrilation
	if ( simmgr_shm->instructor.defibrillation.shock >= 0 )
	{
		if ( simmgr_shm->instructor.defibrillation.shock > 0 )
		{
			simmgr_shm->status.defibrillation.last += 1;
		}
		simmgr_shm->instructor.defibrillation.shock = -1;
	}
	if ( simmgr_shm->instructor.defibrillation.energy >= 0 )
	{
		simmgr_shm->status.defibrillation.energy = simmgr_shm->instructor.defibrillation.energy;
		simmgr_shm->instructor.defibrillation.energy = -1;
	}
	// Release the MUTEX
	sem_post(&simmgr_shm->instructor.sema);
	iiLockTaken = 0;
	
	// Process the trends
	// We do this even if no scenario is running, to allow an instructor simple, manual control
	simmgr_shm->status.cardiac.rate = trendProcess(&cardiacTrend );
	simmgr_shm->status.cardiac.bps_sys = trendProcess(&sysTrend );
	simmgr_shm->status.cardiac.bps_dia = trendProcess(&diaTrend );
	oldRate = simmgr_shm->status.respiration.rate;
	newRate = trendProcess(&respirationTrend );
	setRespirationPeriods(oldRate, newRate );

	// simmgr_shm->status.respiration.awRR = simmgr_shm->status.respiration.rate;
	simmgr_shm->status.respiration.spo2 = trendProcess( &spo2Trend );
	simmgr_shm->status.respiration.etco2 = trendProcess( &etco2Trend );
	simmgr_shm->status.general.temperature = trendProcess(&tempTrend );

	// NIBP processing
	now = std::time(nullptr );
	switch ( nibp_state )
	{
		case NibpIdle:	// Not started or BP Cuff detached
			if ( simmgr_shm->status.cardiac.bp_cuff > 0 )
			{
				if ( simmgr_shm->status.cardiac.nibp_read == 1 )
				{
					// Manual Start - Go to Running for the run delay time
					nibp_run_complete_time = now + NIBP_RUN_TIME;
					nibp_state = NibpRunning;
					snprintf(msgbuf, MSGBUF_LENGTH, "NIBP Read Manual" );
					lockAndComment(msgbuf );
				}
				else if ( simmgr_shm->status.cardiac.nibp_freq != 0 )
				{
					// Frequency set
					nibp_next_time = now + (simmgr_shm->status.cardiac.nibp_freq * 60);
					nibp_state = NibpWaiting;
				}
			}
			break;
		case NibpWaiting:
			if ( simmgr_shm->status.cardiac.bp_cuff == 0 ) // Cuff removed
			{
				nibp_state = NibpIdle;
			}
			else 
			{
				if ( simmgr_shm->status.cardiac.nibp_read == 1 )
				{
					// Manual Override
					nibp_next_time = now;
				}
				if ( nibp_next_time <= now )
				{
					nibp_run_complete_time = now + NIBP_RUN_TIME;
					nibp_state = NibpRunning;
					
					snprintf(msgbuf, MSGBUF_LENGTH, "NIBP Read Periodic" );
					lockAndComment(msgbuf );
					simmgr_shm->status.cardiac.nibp_read = 1;
				}
			}
			break;
		case NibpRunning:
			if ( simmgr_shm->status.cardiac.bp_cuff == 0 ) // Cuff removed
			{
				nibp_state = NibpIdle;
				
				snprintf(msgbuf, MSGBUF_LENGTH, "NIBP Cuff removed while running" );
				lockAndComment(msgbuf );
			}
			else 
			{
				if ( nibp_run_complete_time <= now )
				{
					int meanValue;
					
					simmgr_shm->status.cardiac.nibp_read = 0;
					
					meanValue = ((simmgr_shm->status.cardiac.bps_sys - simmgr_shm->status.cardiac.bps_dia) / 3) + simmgr_shm->status.cardiac.bps_dia;
					snprintf(msgbuf, MSGBUF_LENGTH, "NIBP %d/%d (%d)mmHg %d bpm",
						simmgr_shm->status.cardiac.bps_sys,
						simmgr_shm->status.cardiac.bps_dia,
						meanValue,
						simmgr_shm->status.cardiac.nibp_rate );
					lockAndComment(msgbuf );
				
					if ( simmgr_shm->status.cardiac.nibp_freq != 0 )
					{
						// Frequency set
						nibp_next_time = now + (simmgr_shm->status.cardiac.nibp_freq * 60);
						nibp_state = NibpWaiting;
						
						sprintf(msgbuf, "NibpState Change: Running to Waiting" );
						log_message("", msgbuf ); 
					}
					else
					{
						nibp_state = NibpIdle;
						sprintf(msgbuf, "NibpState Change: Running to Idle" );
						log_message("", msgbuf ); 
					}
				}
			}
			break;
	}
	/*
		if the BP Cuff is attached and we see nibp_read set, then 
	*/
	if ( scenario_state == ScenarioTerminate )
	{
		if ( simmgr_shm->logfile.active == 0 )
		{
			updateScenarioState(ScenarioTerminate );
		}
	}
	else if ( scenario_state == ScenarioStopped )
	{
		if ( simmgr_shm->logfile.active == 0 )
		{
			updateScenarioState(ScenarioStopped );
		}
		if ( scenarioPid )
		{
			killScenario(1, NULL );
		}
	}
	
	return ( 0 );
}

int
start_scenario(const char *name )
{
	char timeBuf[64];
	char fname[128];
	int fileCountBefore;
	int fileCountAfter;
	
	sprintf(msgbuf, "Start Scenario Request: %s", name );
	log_message("", msgbuf ); 

	if ( runningAsDemo )
	{
		sprintf(sessionsPath, "/var/www/html/demo/%s", sesid );
	}
	else
	{
		sprintf(sessionsPath, "/var/www/html/scenarios" );
	}
	sprintf(fname, "%s/%s/main.xml", sessionsPath, name );

	resetAllParameters();
	
	if ( simmgr_shm->status.scenario.record > 0 )
	{
		fileCountBefore = getVideoFileCount();
		recordStartStop(1 );
		while ( ( fileCountAfter = getVideoFileCount() ) == fileCountBefore )
			;
	}
	
	scenario_start_time = std::time(nullptr );
	std::strftime(timeBuf, 60, "%c", std::localtime(&scenario_start_time ));
	
	// exec the new scenario
	scenarioPid = fork();
	if ( scenarioPid == 0 )
	{
		// Child
		
		sprintf(msgbuf, "Start Scenario: execl %s  %s", "/usr/local/bin/scenario", fname );
		log_message("", msgbuf ); 
		
		if ( runningAsDemo )
		{
			execl("/usr/local/bin/scenario", "scenario", fname, "-S", sesid, (char *)0 );
		}
		else
		{
			execl("/usr/local/bin/scenario", "scenario", fname, (char *)0 );
		}
		// execl does not return on success 
		sprintf(msgbuf, "Start Scenario: execl fails for %s :%s", name, strerror(errno ) );
		log_message("", msgbuf ); 
	}
	else if ( scenarioPid > 0 )
	{
		// Parent
		sprintf(msgbuf, "Start Scenario: %s Pid is %d", name, scenarioPid );
		log_message("", msgbuf ); 
		sprintf(simmgr_shm->status.scenario.active, "%s", name );
		
		sprintf(simmgr_shm->status.scenario.start, "%s", timeBuf );
		sprintf(simmgr_shm->status.scenario.runtimeAbsolute, "%s", "00:00:00" );		
		sprintf(simmgr_shm->status.scenario.runtimeScenario, "%s", "00:00:00" );
		sprintf(simmgr_shm->status.scenario.runtimeScene, "%s", "00:00:00" );
		//sprintf(simmgr_shm->status.scenario.scene_name, "%s", "init" );
		//simmgr_shm->status.scenario.scene_id = 0;
		
		updateScenarioState(ScenarioRunning );
		(void)simlog_create();
	}
	else
	{
		// Failed
		sprintf(msgbuf, "fork Fails %s", strerror(errno) );
		log_message("", msgbuf );
	}

	return ( 0 );
}

/*
 * checkEvents
 * 
 * Scan through the event list and log any new events
 * Also scan comment list and and new ones to log file
 */

void
checkEvents(void )
{
	if ( ( lastEventLogged != simmgr_shm->eventListNext ) ||
		 ( lastCommentLogged != simmgr_shm->commentListNext ) )
	{
		takeInstructorLock();
		while ( lastEventLogged != simmgr_shm->eventListNext )
		{
			sprintf(msgbuf, "Event: %s", simmgr_shm->eventList[lastEventLogged].eventName );
			simlog_entry(msgbuf );
			lastEventLogged++;
			if ( lastEventLogged >= EVENT_LIST_SIZE )
			{
				lastEventLogged = 0;
			}
		}
		while ( lastCommentLogged != simmgr_shm->commentListNext )
		{
			if ( strlen(simmgr_shm->commentList[lastCommentLogged].comment ) == 0 )
			{
				sprintf(msgbuf, "Null Comment: lastCommentLogged is %d simmgr_shm->commentListNext is %d State is %d\n",
					lastCommentLogged, simmgr_shm->commentListNext, scenario_state );
				log_message("Error", msgbuf );
			}
			else
			{
				simlog_entry(simmgr_shm->commentList[lastCommentLogged].comment );
			}
			lastCommentLogged++;
			if ( lastCommentLogged >= COMMENT_LIST_SIZE )
			{
				lastCommentLogged = 0;
			}
		}
		releaseInstructorLock();
	}
}

void
resetAllParameters(void )
{
	// status/cardiac
	sprintf(simmgr_shm->status.cardiac.rhythm, "%s", "sinus" );
	sprintf(simmgr_shm->status.cardiac.vpc, "%s", "none" );
	sprintf(simmgr_shm->status.cardiac.vfib_amplitude, "%s", "high" );
	simmgr_shm->status.cardiac.vpc_freq = 10;
	simmgr_shm->status.cardiac.vpc_delay = 0;
	simmgr_shm->status.cardiac.pea = 0;
	simmgr_shm->status.cardiac.rate = 80;
	simmgr_shm->status.cardiac.nibp_rate = 80;
	simmgr_shm->status.cardiac.nibp_read = -1;
	simmgr_shm->status.cardiac.nibp_linked_hr = 1;
	simmgr_shm->status.cardiac.nibp_freq = 0;
	sprintf(simmgr_shm->status.cardiac.pwave, "%s", "none" );
	simmgr_shm->status.cardiac.pr_interval = 140; // Good definition at http://lifeinthefastlane.com/ecg-library/basics/pr-interval/
	simmgr_shm->status.cardiac.qrs_interval = 85;
	simmgr_shm->status.cardiac.bps_sys = 105;
	simmgr_shm->status.cardiac.bps_dia = 70;
	simmgr_shm->status.cardiac.right_dorsal_pulse_strength = 2;
	simmgr_shm->status.cardiac.right_femoral_pulse_strength = 2;
	simmgr_shm->status.cardiac.left_dorsal_pulse_strength = 2;
	simmgr_shm->status.cardiac.left_femoral_pulse_strength = 2;
	sprintf(simmgr_shm->status.cardiac.heart_sound, "%s", "none" );
	simmgr_shm->status.cardiac.heart_sound_volume = 10;
	simmgr_shm->status.cardiac.heart_sound_mute = 0;
	simmgr_shm->status.cardiac.ecg_indicator = 0;
	simmgr_shm->status.cardiac.bp_cuff = 0;
	simmgr_shm->status.cardiac.arrest = 0;
	
	// status/respiration
	sprintf(simmgr_shm->status.respiration.left_lung_sound, "%s", "normal" );
	sprintf(simmgr_shm->status.respiration.left_sound_in, "%s", "normal" );
	sprintf(simmgr_shm->status.respiration.left_sound_out, "%s", "normal" );
	sprintf(simmgr_shm->status.respiration.left_sound_back, "%s", "normal" );
	sprintf(simmgr_shm->status.respiration.right_lung_sound, "%s", "normal" );
	sprintf(simmgr_shm->status.respiration.right_sound_in, "%s", "normal" );
	sprintf(simmgr_shm->status.respiration.right_sound_out, "%s", "normal" );
	sprintf(simmgr_shm->status.respiration.right_sound_back, "%s", "normal" );
	simmgr_shm->status.respiration.left_lung_sound_volume = 10;
	simmgr_shm->status.respiration.left_lung_sound_mute = 1;
	simmgr_shm->status.respiration.right_lung_sound_volume = 10;
	simmgr_shm->status.respiration.right_lung_sound_mute = 0;
	simmgr_shm->status.respiration.inhalation_duration = 1350;
	simmgr_shm->status.respiration.exhalation_duration = 1050;
	simmgr_shm->status.respiration.rate = 20;
	simmgr_shm->status.respiration.spo2 = 95;
	simmgr_shm->status.respiration.etco2 = 34;
	simmgr_shm->status.respiration.etco2_indicator = 0;
	simmgr_shm->status.respiration.spo2_indicator = 0;
	simmgr_shm->status.respiration.chest_movement = 0;
	simmgr_shm->status.respiration.manual_breath = 0;
	simmgr_shm->status.respiration.manual_count = 0;
	
	// status/vocals
	sprintf(simmgr_shm->status.vocals.filename, "%s", "" );
	simmgr_shm->status.vocals.repeat = 0;
	simmgr_shm->status.vocals.volume = 10;
	simmgr_shm->status.vocals.play = 0;
	simmgr_shm->status.vocals.mute = 0;
	
	// status/auscultation
	simmgr_shm->status.auscultation.side = 0;
	simmgr_shm->status.auscultation.row = 0;
	simmgr_shm->status.auscultation.col = 0;
	
	// status/pulse
	simmgr_shm->status.pulse.right_dorsal = 0;
	simmgr_shm->status.pulse.left_dorsal = 0;
	simmgr_shm->status.pulse.right_femoral = 0;
	simmgr_shm->status.pulse.left_femoral = 0;
	
	// status/cpr
	simmgr_shm->status.cpr.last = 0;
	simmgr_shm->status.cpr.compression = 0;
	simmgr_shm->status.cpr.release = 0;
	simmgr_shm->status.cpr.duration = 0;
	
	// status/defibrillation
	simmgr_shm->status.defibrillation.last = 0;
	simmgr_shm->status.defibrillation.energy = 100;
	simmgr_shm->status.defibrillation.shock = 0;
	
	// status/general
	simmgr_shm->status.general.temperature = 1017;
	simmgr_shm->status.general.temperature_enable = 0;
	
	// status/media
	sprintf(simmgr_shm->status.media.filename, "%s", "" );
	simmgr_shm->status.media.play = 0;
		
	// instructor/cardiac
	sprintf(simmgr_shm->instructor.cardiac.rhythm, "%s", "" );
	simmgr_shm->instructor.cardiac.rate = -1;
	simmgr_shm->instructor.cardiac.nibp_rate = -1;
	simmgr_shm->instructor.cardiac.nibp_read = -1;
	simmgr_shm->instructor.cardiac.nibp_linked_hr = -1;
	simmgr_shm->instructor.cardiac.nibp_freq = -1;
	sprintf(simmgr_shm->instructor.cardiac.pwave, "%s", "" );
	simmgr_shm->instructor.cardiac.pr_interval = -1;
	simmgr_shm->instructor.cardiac.qrs_interval = -1;
	simmgr_shm->instructor.cardiac.bps_sys = -1;
	simmgr_shm->instructor.cardiac.bps_dia = -1;
	simmgr_shm->instructor.cardiac.pea = -1;
	simmgr_shm->instructor.cardiac.vpc_freq = -1;
	simmgr_shm->instructor.cardiac.vpc_delay = -1;
	sprintf(simmgr_shm->instructor.cardiac.vpc, "%s", "" );
	sprintf(simmgr_shm->instructor.cardiac.vfib_amplitude, "%s", "" );
	simmgr_shm->instructor.cardiac.right_dorsal_pulse_strength = -1;
	simmgr_shm->instructor.cardiac.right_femoral_pulse_strength = -1;
	simmgr_shm->instructor.cardiac.left_dorsal_pulse_strength = -1;
	simmgr_shm->instructor.cardiac.left_femoral_pulse_strength = -1;
	sprintf(simmgr_shm->instructor.cardiac.heart_sound, "%s", "" );
	simmgr_shm->instructor.cardiac.heart_sound_volume = -1;
	simmgr_shm->instructor.cardiac.heart_sound_mute = -1;
	simmgr_shm->instructor.cardiac.ecg_indicator = -1;
	simmgr_shm->instructor.cardiac.bp_cuff = -1;
	simmgr_shm->instructor.cardiac.arrest = -1;
	
	// instructor/respiration
	sprintf(simmgr_shm->instructor.respiration.left_lung_sound, "%s", "" );
	sprintf(simmgr_shm->instructor.respiration.left_sound_in, "%s", "" );
	sprintf(simmgr_shm->instructor.respiration.left_sound_out, "%s", "" );
	sprintf(simmgr_shm->instructor.respiration.left_sound_back, "%s", "" );
	simmgr_shm->instructor.respiration.left_lung_sound_volume = -1;
	simmgr_shm->instructor.respiration.left_lung_sound_mute = -1;
	simmgr_shm->instructor.respiration.right_lung_sound_volume = -1;
	simmgr_shm->instructor.respiration.right_lung_sound_mute = -1;
	sprintf(simmgr_shm->instructor.respiration.right_lung_sound, "%s", "" );
	sprintf(simmgr_shm->instructor.respiration.right_sound_in, "%s", "" );
	sprintf(simmgr_shm->instructor.respiration.right_sound_out, "%s", "" );
	sprintf(simmgr_shm->instructor.respiration.right_sound_back, "%s", "" );
	simmgr_shm->instructor.respiration.inhalation_duration = -1;
	simmgr_shm->instructor.respiration.exhalation_duration = -1;
	simmgr_shm->instructor.respiration.rate = -1;
	simmgr_shm->instructor.respiration.spo2 = -1;
	simmgr_shm->instructor.respiration.etco2 = -1;
	simmgr_shm->instructor.respiration.etco2_indicator = -1;
	simmgr_shm->instructor.respiration.spo2_indicator = -1;
	simmgr_shm->instructor.respiration.chest_movement = -1;
	simmgr_shm->instructor.respiration.manual_breath = -1;
	simmgr_shm->instructor.respiration.manual_count = -1;
	
	// instructor/media
	sprintf(simmgr_shm->instructor.media.filename, "%s", "" );
	simmgr_shm->instructor.media.play = -1;
	
	// instructor/general
	simmgr_shm->instructor.general.temperature = -1;
	simmgr_shm->instructor.general.temperature_enable = -1;

	// instructor/vocals
	sprintf(simmgr_shm->instructor.vocals.filename, "%s", "" );
	simmgr_shm->instructor.vocals.repeat = -1;
	simmgr_shm->instructor.vocals.volume = -1;
	simmgr_shm->instructor.vocals.play = -1;
	simmgr_shm->instructor.vocals.mute = -1;
	
	// instructor/defibrillation
	simmgr_shm->instructor.defibrillation.energy = -1;
	simmgr_shm->instructor.defibrillation.shock = -1;
	
	clearAllTrends();
}
void
strToLower(char *buf )
{
	int i;
	for ( i = 0 ; buf[i] != 0 ; i++ )
	{
		buf[i] = (char)tolower(buf[i] );
	}
}

void
checkScenarioProcess(void )
{
	int sts;
	
	if ( scenario_state != ScenarioTerminate )
	{
		sts = kill( scenarioPid, 0 );
		if ( sts != 0 )
		{
			// Use "force" to clear InstructorLock, in case scenario process held the lock when it terminated
			forceInstructorLock();
			
			snprintf(msgbuf, MSGBUF_LENGTH, "Scenario Process Died" );
			log_message("", msgbuf );
			updateScenarioState(ScenarioStopped );
		}
	}
}
void
killScenario(int arg1, void *arg2 )
{
	if ( scenarioPid > 0 )
	{
		kill( scenarioPid, SIGTERM );
		scenarioPid = -1;
	}
}

