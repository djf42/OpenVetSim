/*
 * simpulse.cpp
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
/*
 *
 * Time clock for the Cardiac and Respiratory systems. This application monitors the shared
 * memory to get the rate parameters and issues sync signals to the various systems.
 *
 * The demo version is integrated with SimMgr. It does not have any listen functions.
 *
 * It has two timers; one for the heart rate (pulse) and
 * one for the breath rate (respiration). 
																									   
 * Copyright (C) 2016-2018 Terence Kelleher. All rights reserved.
 *
 */
 
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

#include <iostream>
#include <vector>  
#include <string>  
#include <cstdlib>
#include <sstream>

#include <ctime>
#include <math.h>       /* modf */
#include <netinet/in.h>
#include <netinet/ip.h> 

// #define DEBUG

extern char msgbuf[2048];

#include "../include/simmgr.h" 

int quit_flag = 0;
timer_t pulse_timer;
timer_t breath_timer;
struct sigevent pulse_sev;
struct sigevent breath_sev;
int currentPulseRate = 0;
int currentVpcFreq = 0;

int currentBreathRate = 0;
extern int runningAsDemo;
void set_beat_time(int bpm );
void set_pulse_rate(int bpm );
void set_breath_rate(int bpm );
void calculateVPCFreq(void );

#define PULSE_TIMER_SIG		SIGRTMIN
#define BREATH_TIMER_SIG	(SIGRTMIN+1)

#define VPC_ARRAY_LEN	200
int vpcFrequencyArray[VPC_ARRAY_LEN];
int vpcFrequencyIndex = 0;
int vpcType = 0;
int afibActive = 0;
#define IS_CARDIAC	1
#define NOT_CARDIAC	0

int beatPhase = 0;
int vpcState = 0;
int vpcCount = 0;

/* vpcState is set at the beginning of a sinus cycle where VPCs will follow.
	vpcState is set to the number of VPCs to be injected.
	
	beatPhase is set to the number of beat ticks to wait for the next event. This is typically:
		From Sinus to Sinus:	10
		From Sinus to VPC1:		7
		From VPC1 to Sinus:		13
		From VPC1 to VPC2:		7
		From VPC2 to Sinus:		16
		From VPC2 to VPC3:		7
		From VPC3 to Sinus:		19
*/
static void
beat_handler(int sig, siginfo_t *si, void *uc)
{
	int rate;
	time_t t;
	
	/* Intializes random number generator */
    srand((unsigned) time(&t));
   
	if ( simmgr_shm->status.cpr.running )
	{
		return;
	}
		
	if ( sig == PULSE_TIMER_SIG )
	{
		if ( simmgr_shm->status.defibrillation.shock )
		{
			return;
		}
		 
		rate = currentPulseRate;

		if ( rate > 0 )
		{
			if ( beatPhase-- <= 0 )
			{
				if ( vpcState > 0 )
				{
					// VPC Injection
					simmgr_shm->status.cardiac.pulseCountVpc++;
					vpcState--;
					switch ( vpcState )
					{
						case 0: // Last VPC
							switch ( simmgr_shm->status.cardiac.vpc_count )
							{
								case 0:	// This should only occur if VPCs were just disabled.
								case 1:
								default:	// Should not happen
									beatPhase = 13;
									break;
								case 2:
									beatPhase = 16;
									break;
								case 3:
									beatPhase = 19;
									break;
							}
							break;
						default:
							beatPhase = 6;
							break;
					}
				}
				else
				{
					// Normal Cycle
					simmgr_shm->status.cardiac.pulseCount++;
					if ( afibActive )
					{
						// Next beat phase is between 50% and 200% of standard. 
						// Calculate a random from 0 to 14 and add to 5
						beatPhase = 5 + (rand() % 14);
					}
					else if ( ( vpcType > 0 ) && ( currentVpcFreq > 0 ) )
					{
						if ( vpcFrequencyIndex++ >= VPC_ARRAY_LEN )
						{
							vpcFrequencyIndex = 0;
						}
						if ( vpcFrequencyArray[vpcFrequencyIndex] > 0 )
						{
							vpcState = simmgr_shm->status.cardiac.vpc_count;
							beatPhase = 6;
						}
						else
						{
							beatPhase = 9;
						}
					}
					else
					{
						beatPhase = 9;	// Preset for "normal"
					}
				}
			}
		}
	}
	else if ( sig == BREATH_TIMER_SIG )
	{
		if ( simmgr_shm->status.respiration.rate > 0 )
		{
			simmgr_shm->status.respiration.breathCount++;
		}
	}	
}

void
calculateVPCFreq(void )
{
	int count = 0;
	int i;
	int val;
	
	if ( simmgr_shm->status.cardiac.vpc_freq == 0 )
	{
		currentVpcFreq = 0;
	}
	else
	{
		// get 100 samples for 100 cycles of sinus rhythm between 10 and 90
		for ( i = 0 ; i < VPC_ARRAY_LEN ; i++ )
		{
			val = rand() % 100;
			if ( val > currentVpcFreq )
			{
				vpcFrequencyArray[i] = 0;
			}
			else
			{
				vpcFrequencyArray[i] = 1;
				count++;
			}
		}
#ifdef DEBUG
		sprintf(msgbuf, "calculateVPCFreq: request %d: result %d", currentVpcFreq, count );
		log_message("", msgbuf );
#endif
		vpcFrequencyIndex = 0;
	}
}

/*
 * FUNCTION:
 *		resetTimer
 *
 * ARGUMENTS:
 *		rate	- Rate in Beats per minute
 *		its		- New timer spec
 *		itsRemaining - Running timer spec
 *		isCaridac	- Set to IS_CARDIAC for the cardiac timer
 *
 * DESCRIPTION:
 *		Calculate and set the timer, used for both heart and breath.
 *
 * ASSUMPTIONS:
 *		Called with pulseSema held
*/
void
resetTimer(int rate, struct itimerspec *its, struct itimerspec *itsRemaining, int isCardiac )
{
	float nsecRemaining;	// nano-seconds Remaining in current Timer
	float nsecNext;			// nano-seconds to Next based on new rate plus "delay"
	float nsecInitial;
	float intpart;
	float fractpart;
	float wait_time_nsec;
	float frate;
	float sec_per_beat;
	
	frate = (float)rate;
	sec_per_beat = 1 / (frate/ 60);
	
	if ( isCardiac )
	{
		sec_per_beat = sec_per_beat / 10;
	}
	fractpart = modff(sec_per_beat, &intpart );
	wait_time_nsec = ( fractpart * 1000 * 1000 * 1000 );
	its->it_interval.tv_sec = (long int)intpart;
	its->it_interval.tv_nsec = (long int)wait_time_nsec;

	if ( itsRemaining != NULL )
	{
		nsecRemaining = ( itsRemaining->it_value.tv_sec * 1000 * 1000 * 1000 ) + itsRemaining->it_value.tv_nsec;
		nsecNext = sec_per_beat * 1000 * 1000 * 1000;
		
		if ( nsecRemaining <= 0 )
		{
			nsecInitial = nsecNext;
		}
		else if ( nsecRemaining < nsecNext )
		{
			nsecInitial = nsecRemaining;
		}
		else
		{
			nsecInitial = nsecNext;
		}
		if ( nsecInitial > 1 )
		{
			fractpart = modff( (nsecInitial/1000/1000/1000), &intpart );
			wait_time_nsec = ( fractpart * 1000 * 1000 * 1000 );
		}
	}

	its->it_value.tv_sec = intpart;
	its->it_value.tv_nsec = wait_time_nsec;
}
	
/*
 * FUNCTION:
 *		set_pulse_rate
 *
 * ARGUMENTS:
 *		bpm	- Rate in Beats per Minute
 *
 * DESCRIPTION:
 *		Calculate and set the wait time in usec for the beats.
 *		The beat timer runs at 10x the heart rate
 *
 * ASSUMPTIONS:
 *		Called with pulseSema held
*/

void
set_pulse_rate(int bpm )
{
	struct itimerspec its;
	struct itimerspec itsRemaining;	
	int sts;
#ifdef DEBUG
	long int msec;
#endif

	// When the BPM is zero, we set the timer based on 60, to allow it to continue running.
	// No beats are sent when this occurs, but the timer still runs.
	if ( bpm == 0 )
	{
		bpm = 60;
	}

	sts = timer_gettime(pulse_timer, &itsRemaining );
	if ( sts == 0 )
	{
		resetTimer(bpm, &its, &itsRemaining, IS_CARDIAC );
	}
	else
	{
		sprintf(msgbuf, "set_pulse_rate:  timer_gettime Fails: %s", strerror(errno ) );
		log_message("", msgbuf );
		resetTimer(bpm, &its, NULL, IS_CARDIAC );
	}
	
	if (timer_settime(pulse_timer, 0, &its, NULL) == -1)
	{
		//perror("set_pulse_rate: timer_settime");
		sprintf(msgbuf, "set_pulse_rate(%d): timer_settime: %s", bpm, strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
#ifdef DEBUG
	else
	{
		msec = its.it_interval.tv_nsec / 1000 / 1000;

		sprintf(msgbuf, "set_pulse_rate(%d): %ld.%ld", bpm, its.it_interval.tv_sec, msec );
		log_message("", msgbuf );
	}
#endif
}

void
set_breath_rate(int bpm )
{
	struct itimerspec its;
	struct itimerspec itsRemaining;	
	int sts;
	
	if ( bpm == 0 )
	{
		bpm = 60;
	}

	sts = timer_gettime(breath_timer, &itsRemaining );
	if ( sts == 0 )
	{
		resetTimer(bpm, &its, &itsRemaining, NOT_CARDIAC );
	}
	else
	{
		sprintf(msgbuf, "set_breath_rate:  timer_gettime Fails: %s", strerror(errno ) );
		log_message("", msgbuf );
		resetTimer(bpm, &its, NULL, NOT_CARDIAC );
	}
	
	if (timer_settime(breath_timer, 0, &its, NULL) == -1)
	{
		//perror("set_breath_rate: timer_settime");
		sprintf(msgbuf, "set_breath_rate(%d): timer_settime %s", bpm, strerror(errno)  );
		log_message("", msgbuf );
		exit ( -1 );
	}
#ifdef DEBUG
	else
	{
		//sprintf(msgbuf, "set_breath_rate: RR %d Sec %f nsec %f", bpm, intpart, wait_time_nsec );
		sprintf(msgbuf, "set_breath_rate:  RR %d Initial %f, Interval %f", bpm, secInitial, secInterval );
		log_message("", msgbuf );
	}
#endif
}

void
termination_handler (int signum)
{
	quit_flag = 1;
}

unsigned int last_pulse = 0;
unsigned int last_pulseVpc = 0;
unsigned int last_breath = 0;
int last_manual_breath = 0;
int scenarioRunning = false;

void
pulseDemoTimerInit(void )
{
	struct sigaction new_action;
	sigset_t mask;

	// Seed rand, needed for vpc array generation
	srand(time(NULL) );

	// Pulse Timer Setup
	new_action.sa_flags = SA_SIGINFO;
	new_action.sa_sigaction = beat_handler;
	sigemptyset(&new_action.sa_mask);
	if (sigaction(PULSE_TIMER_SIG, &new_action, NULL) == -1)
	{
		perror("sigaction");
		sprintf(msgbuf, "sigaction() fails for Pulse Timer: %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Block timer signal temporarily
	sigemptyset(&mask);
	sigaddset(&mask, PULSE_TIMER_SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		perror("sigprocmask");
		sprintf(msgbuf, "sigprocmask() fails for Pulse Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Create the Timer
	pulse_sev.sigev_notify = SIGEV_SIGNAL;
	pulse_sev.sigev_signo = PULSE_TIMER_SIG;
	pulse_sev.sigev_value.sival_ptr = &pulse_timer;
	
	if ( timer_create(CLOCK_REALTIME, &pulse_sev, &pulse_timer ) == -1 )
	{
		perror("timer_create" );
		sprintf(msgbuf, "timer_create() fails for Pulse Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit (-1);
	}
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
		perror("sigprocmask");
		sprintf(msgbuf, "sigprocmask() fails for Pulse Timer%s ", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	
	// Breath Timer Setup
	new_action.sa_flags = SA_SIGINFO;
	new_action.sa_sigaction = beat_handler;
	sigemptyset(&new_action.sa_mask);
	if (sigaction(BREATH_TIMER_SIG, &new_action, NULL) == -1)
	{
		perror("sigaction");
		sprintf(msgbuf, "sigaction() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit(-1 );
	}
	// Block timer signal temporarily
	sigemptyset(&mask);
	sigaddset(&mask, BREATH_TIMER_SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		perror("sigprocmask");
		sprintf(msgbuf, "sigprocmask() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Create the Timer
	breath_sev.sigev_notify = SIGEV_SIGNAL;
	breath_sev.sigev_signo = BREATH_TIMER_SIG;
	breath_sev.sigev_value.sival_ptr = &breath_timer;
	
	if ( timer_create(CLOCK_REALTIME, &breath_sev, &breath_timer ) == -1 )
	{
		perror("timer_create" );
		sprintf(msgbuf, "timer_create() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit (-1);
	}
	
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
		perror("sigprocmask");
		sprintf(msgbuf, "sigprocmask() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}	
	
	currentPulseRate = simmgr_shm->status.cardiac.rate;
	set_pulse_rate(currentPulseRate );
	simmgr_shm->status.cardiac.pulseCount = 0;
	simmgr_shm->status.cardiac.pulseCountVpc = 0;
	
	currentBreathRate = simmgr_shm->status.respiration.rate;
	set_breath_rate(currentBreathRate );
	simmgr_shm->status.respiration.breathCount = 0;
	
	last_pulse = simmgr_shm->status.cardiac.pulseCount;
	last_pulseVpc = simmgr_shm->status.cardiac.pulseCountVpc;
	last_breath = simmgr_shm->status.respiration.breathCount;
	last_manual_breath = simmgr_shm->status.respiration.manual_count;
	
}

/*
 * FUNCTION: pulseTimerRun
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *		Never
 *
 * DESCRIPTION:
 *		This process monitors the pulse and breath counts. When incremented (by the beat_handler)
 *		a message is sent to the listeners.
 *		It also monitors the rates and adjusts the timeout for the beat_handler when a rate is changed.
*/
 
int checkCount = 0;

void
pulseTimerRun(void )
{
	// Call every 10 msec
	
	
	if ( scenarioRunning )
	{
		if ( last_pulse != simmgr_shm->status.cardiac.pulseCount )
		{
			last_pulse = simmgr_shm->status.cardiac.pulseCount;
		}
		if ( last_pulseVpc != simmgr_shm->status.cardiac.pulseCountVpc )
		{
			last_pulseVpc = simmgr_shm->status.cardiac.pulseCountVpc;
		}
	
		if ( last_breath != simmgr_shm->status.respiration.breathCount )
		{
			last_breath = simmgr_shm->status.respiration.breathCount;
			if ( last_manual_breath != simmgr_shm->status.respiration.manual_count )
			{
				last_manual_breath = simmgr_shm->status.respiration.manual_count;
			}
			else
			{

			}
		}
	}
	checkCount++;
	if ( checkCount == 1 ) // This runs every 50 ms.
	{
		if ( strcmp(simmgr_shm->status.scenario.state, "Running" ) == 0 )
		{
			scenarioRunning = true;
		}
		else
		{
			scenarioRunning = false;
		}
	}
	if ( checkCount == 2 )	
	{
		// If the pulse rate has changed, then reset the timer
		
		if ( currentPulseRate != simmgr_shm->status.cardiac.rate )
		{
			set_pulse_rate(simmgr_shm->status.cardiac.rate );
			currentPulseRate = simmgr_shm->status.cardiac.rate;
#ifdef DEBUG
			sprintf(msgbuf, "Set Pulse to %d", currentPulseRate );
			log_message("", msgbuf );
#endif
		}
	}
	else if ( checkCount == 3 )	
	{
		if ( currentVpcFreq != simmgr_shm->status.cardiac.vpc_freq ||
		 vpcType != simmgr_shm->status.cardiac.vpc_type )
		{
			currentVpcFreq = simmgr_shm->status.cardiac.vpc_freq;
			vpcType = simmgr_shm->status.cardiac.vpc_type;
			calculateVPCFreq();
		}
	}
	else if ( checkCount == 4 )	
	{
		if ( strncmp(simmgr_shm->status.cardiac.rhythm, "afib", 4 ) == 0 )
		{
			afibActive = 1;
		}
		else
		{
			afibActive = 0;
		}
	}
	else if ( checkCount == 5 )
	{
		// If the breath rate has changed, then reset the timer
		if ( currentBreathRate != simmgr_shm->status.respiration.rate )
		{
			set_breath_rate(simmgr_shm->status.respiration.rate );
			currentBreathRate = simmgr_shm->status.respiration.rate;
			
			// awRR Calculation - TBD - Need real calculations
			//simmgr_shm->status.respiration.awRR = simmgr_shm->status.respiration.rate;
#ifdef DEBUG
			sprintf(msgbuf, "Set Breath to %d", currentBreathRate );
			log_message("", msgbuf );
#endif
		}
		checkCount = 0;
	}
}
