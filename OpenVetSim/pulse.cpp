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
 * This process runs independently from the SimMgr. It has two timers; one for the heart rate (pulse) and
 * one for the breath rate (respiration). It runs as two threads. The primary thread listens for connections
 * from clients, and the child thread monitors the pulse and breath counts to send sync messages to the
 * clients.
 *
 * Listen for a connections on Port 50200 (SimMgr Event Port)
 *
 * 		1 - On connection, the daemon will fork a task to support the connection
 *		2 - Each connection waits on sync messages
 *
 * Copyright (C) 2016-2018 Terence Kelleher. All rights reserved.
 *
 */

#include "vetsim.h"

using namespace std;

extern struct simmgr_shm shmSpace;

/*
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
#include <math.h>       // 
#include <netinet/in.h>
#include <netinet/ip.h> 

#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <arpa/inet.h>
*/
// #define DEBUG
#define BUF_SIZE 2048
char p_msg[BUF_SIZE];

int quit_flag = 0;

int currentPulseRate = 0;
int currentVpcFreq = 0;

int currentBreathRate = 0;
unsigned int lastManualBreath = 0;

// WinSock2 and cross-platform socket headers provided by vetsim.h / platform.h


void getControllerVersion(int index );

void set_pulse_rate(int bpm);
void set_breath_rate(int bpm);
void calculateVPCFreq(void);
void sendStatusPort(int listener);

/* struct to hold data to be passed to a thread
   this shows how multiple data items can be passed to a thread */
struct listener
{
	int allocated;
	int thread_no;
	SOCKET cfd;
	char ipAddr[32];
	char version[32];
};
#define MAX_LISTENERS 10

struct listener listeners[MAX_LISTENERS];

char pulseWord[] = "pulse\n";
char pulseWordVPC[] = "pulseVPC\n";
char breathWord[] = "breath\n";

#define VPC_ARRAY_LEN	200
int vpcFrequencyArray[VPC_ARRAY_LEN];
int vpcFrequencyIndex = 0;
int vpcType = 0;
int afibActive = 0;
#define IS_CARDIAC	1
#define NOT_CARDIAC	0

void pulseTimer(void);
void pulseBroadcastLoop(void);

std::mutex breathSema;
std::mutex pulseSema;

// Beat trace instrumentation.
// Set to 1 to log every message sent to the hardware controller with an absolute
// millisecond timestamp.  Use this to answer, from data rather than inference:
//   - Are pulse messages actually evenly spaced at the source?
//   - Is a spurious extra beat being emitted (an unexpected "pulseVPC" will make
//     the controller sound a beat right next to a normal one)?
//   - Does a "breath" message land immediately before a delayed heart sound?
//     (If so, the controller is serializing breath handling ahead of the beat,
//     and the fix belongs in the firmware, not here.)
// Roughly 2-4 lines/second at typical rates -- fine for a diagnostic run.
// Set to 0 to silence.
int beatTraceEnabled = 1;

/*
 * Beat trace ring buffer.
 *
 * IMPORTANT: log_message() opens, writes and closes a file on every call, takes a
 * mutex, and on the Windows GUI build also updates an edit control.  That is far
 * too heavy to call from pulseBroadcastLoop, which runs at THREAD_PRIORITY_TIME_
 * CRITICAL and is the thread that actually delivers beats to the controller --
 * logging there would add exactly the kind of delay we are trying to measure.
 *
 * So the beat thread only writes plain integers into this ring buffer (no locks,
 * no I/O), and pulseProcessChild -- a normal-priority 50 ms loop -- drains it and
 * does the logging.  Single producer, single consumer.
 */
// Longest send() observed, in microseconds, since the last trace record.
// Written by broadcast_word (beat thread), sampled and reset by the trace push.
volatile long long sendMaxUsec = 0;

#define BEAT_TRACE_LEN 256
struct beatTraceRec
{
	char      kind[10];
	ULONGLONG t;
	long long interval;
	long long expected;
	int       extra;
	long long sendUsec;
};
static struct beatTraceRec beatTraceBuf[BEAT_TRACE_LEN];
static volatile unsigned int beatTraceHead = 0;	// advanced by the beat thread
static volatile unsigned int beatTraceTail = 0;	// advanced by the drain thread

static void
beatTracePush(const char* kind, ULONGLONG t, long long interval, long long expected, int extra)
{
	unsigned int h = beatTraceHead;
	struct beatTraceRec* r = &beatTraceBuf[h % BEAT_TRACE_LEN];
	sprintf_s(r->kind, sizeof(r->kind), "%s", kind);
	r->t        = t;
	r->interval = interval;
	r->expected = expected;
	r->extra    = extra;
	r->sendUsec = sendMaxUsec;	// longest send() since the previous record
	sendMaxUsec = 0;
	beatTraceHead = h + 1;	// publish last
}

// Called from pulseProcessChild (normal priority) -- safe to do file I/O here.
void
beatTraceDrain(void)
{
	char buf[BUF_SIZE];

	// If the producer lapped us, skip ahead rather than emitting stale records.
	if ((beatTraceHead - beatTraceTail) > BEAT_TRACE_LEN)
	{
		beatTraceTail = beatTraceHead - BEAT_TRACE_LEN;
	}

	while (beatTraceTail != beatTraceHead)
	{
		struct beatTraceRec* r = &beatTraceBuf[beatTraceTail % BEAT_TRACE_LEN];

		// Flag intervals that deviate, but only where the rate held steady
		// (expected > 0 means the caller confirmed the rate did not change).
		if (r->expected > 0)
		{
			long long deviation = r->interval - r->expected;
			if (deviation > 25 || deviation < -25)
			{
				sprintf_s(buf, BUF_SIZE,
					"BEAT-JITTER: interval %lld ms, expected %lld ms, deviation %+lld ms",
					r->interval, r->expected, deviation);
				log_message("", buf);
			}
		}

		if (beatTraceEnabled)
		{
			sprintf_s(buf, BUF_SIZE, "TRACE %-8s t=%llu  interval=%lld  listeners=%d  send=%lldus",
				r->kind, r->t, r->interval, r->extra, r->sendUsec);
			log_message("", buf);
		}

		beatTraceTail++;
	}
}

int beatPhase = 0;
int vpcState = 0;
int vpcCount = 0;
ULONGLONG nextBreathTime = 0;
ULONGLONG nextPulseTime = 0;
ULONGLONG breathInterval = 0;
ULONGLONG pulseInterval = 0;

void
resetVpc(void)
{
	beatPhase = 0;
	vpcState = 0;
	vpcCount = 0;
}

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
extern void setPulseState(int);
extern void hrLogBeat(void);

static void
pulse_beat_handler(void)
{
	pulseSema.lock();
	if (currentPulseRate > 0)
	{
		if ((vpcType > 0) || (afibActive))
		{
			if (beatPhase-- <= 0)
			{
				if (vpcState > 0)
				{
					// VPC Injection
					simmgr_shm->status.cardiac.pulseCountVpc++;
					hrLogBeat();
					vpcState--;
					switch (vpcState)
					{
					case 0: // Last VPC
						switch (simmgr_shm->status.cardiac.vpc_count)
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
					hrLogBeat();
					if (afibActive)
					{
						// Next beat phase is centered on the normal sinus value (9),
						// giving an average rate equal to the set rate.
						// Range 3-15 = 40% to 160% of standard interval around mean.
						beatPhase = 3 + (rand() % 13);
					}
					else if ((vpcType > 0) && (currentVpcFreq > 0))
					{
						vpcFrequencyIndex++;
						if (!(vpcFrequencyIndex < VPC_ARRAY_LEN))
						{
							vpcFrequencyIndex = 0;
						}
						if (vpcFrequencyArray[vpcFrequencyIndex] > 0)
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
		else
		{
			simmgr_shm->status.cardiac.pulseCount++;
			hrLogBeat();
			setPulseState(2);
		}
	}
	pulseSema.unlock();
}
static void
breath_beat_handler(void)
{
	breathSema.lock();
	if (simmgr_shm->status.respiration.rate > 0)
	{
		simmgr_shm->status.respiration.breathCount++;
	}
	breathSema.unlock();
}

void
calculateVPCFreq(void)
{
	int count = 0;
	int i;
	int val;

	if (simmgr_shm->status.cardiac.vpc_freq == 0)
	{
		currentVpcFreq = 0;
	}
	else
	{
		// get 100 samples for 100 cycles of sinus rhythm between 10 and 90
		for (i = 0; i < VPC_ARRAY_LEN; i++)
		{
			val = rand() % 100;
			if (val > currentVpcFreq)
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
		sprintf_s(p_msg, "calculateVPCFreq: request %d: result %d", currentVpcFreq, count);
		log_message("", p_msg);
#endif
		vpcFrequencyIndex = 0;
	}
}
/*
 * FUNCTION:
 *		getWaitTimeMsec
 *
 * ARGUMENTS:
 *		rate	- Rate in Beats per minute
 *		isCaridac	- Set to IS_CARDIAC for the cardiac timer
 *		isFib		- Set if 10 phase timer is needed
 *
 * DESCRIPTION:
 *		Calculate and set the timer, used for both heart and breath.
 *
 * ASSUMPTIONS:
 *		Called with pulseSema or breathSema held
*/
ULONGLONG
getWaitTimeMsec(int rate, int isCardiac, int isFib)
{
	double frate;	// Beats per minute
	double sec_per_beat;
	double msec_per_beat_f;
	ULONGLONG wait_time_msec;

	frate = (double)rate;
	sec_per_beat = 1 / (frate / 60);

	// Note that the heart beat handler is called 10 times per interval, 
	// to provide VPC and AFIB functions
	if (isFib)
	{
		sec_per_beat = sec_per_beat / 10;
	}
	msec_per_beat_f = sec_per_beat * 1000;
	wait_time_msec = (ULONGLONG)(msec_per_beat_f);
	return (wait_time_msec);
}
/*
 * FUNCTION:
 *		resetTimer
 *
 * ARGUMENTS:
 *		rate	- Rate in Beats per minute
 *		isCaridac	- Set to IS_CARDIAC for the cardiac timer
 *		isFib		- Set if 10 phase timer is needed
 *
 * DESCRIPTION:
 *		Calculate and set the timer, used for both heart and breath.
 *
 * ASSUMPTIONS:
 *		Called with pulseSema or breathSema held
*/
void
resetTimer(int rate, int isCardiac, int isFib)
{
	ULONGLONG wait_time_msec;
	ULONGLONG remaining;
	ULONGLONG now = GetTickCount64();

	wait_time_msec = getWaitTimeMsec(rate, isCardiac, isFib);

	//printf("Set Timer: Rate %d sec_per_beat %f %llu\n", rate, sec_per_beat, wait_time_msec);
	if (isCardiac)
	{
		pulseInterval = wait_time_msec;
		remaining = nextPulseTime - now;
		if (remaining > (now + pulseInterval))
		{
			nextPulseTime = now + wait_time_msec;
		}
		
	}
	else
	{
		breathInterval = wait_time_msec;
		remaining = nextBreathTime - now;
		// If the next scheduled breath is further away than one full new interval,
		// reschedule it so the rate ramp-up is reflected in actual breathing timing.
		if (remaining > wait_time_msec)
		{
			nextBreathTime = now + wait_time_msec;
		}
	}
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
set_pulse_rate(int bpm)
{
	// When the BPM is zero, we set the timer based on 60, to allow it to continue running.
	// No beats are sent when this occurs, but the timer still runs.
	if (bpm == 0)
	{
		bpm = 60;
	}
	if ((vpcType > 0) || (afibActive))
	{
		resetTimer(bpm, IS_CARDIAC, 1 );
	}
	else
	{
		resetTimer(bpm, IS_CARDIAC, 0);
	}
}

// restart_breath_timer is called when a manual respiration is flagged.
void
restart_breath_timer(void)
{
	ULONGLONG now = GetTickCount64();
	ULONGLONG wait_time_msec;

	// When rate is 0, getWaitTimeMsec would divide by zero (producing +inf or 0),
	// corrupting breathInterval and nextBreathTime.  Mirror set_breath_rate(0) behaviour
	// and push the timer far into the future instead.
	if (simmgr_shm->status.respiration.rate == 0)
	{
		breathInterval = 60000;
		nextBreathTime = now + 3600000ULL;	// 1 hour away
		return;
	}

	wait_time_msec = getWaitTimeMsec(simmgr_shm->status.respiration.rate, 0, 0);
	breathInterval = wait_time_msec;

	// For very slow cycles (less than 15 BPM), set initial timer to half the cycle plus add 0.1 seconds.
	if (simmgr_shm->status.respiration.rate < 15)
	{
		nextBreathTime = now + ((breathInterval / 2) + 100);
	}
	else
	{
		nextBreathTime = now + breathInterval;
	}
}

void
set_breath_rate(int bpm)
{
	if (bpm == 0)
	{
		// When rate is 0 (apnea / arrest), push nextBreathTime far into the future
		// instead of keeping a 60 bpm placeholder timer running.  The placeholder
		// caused a spurious breath the instant rate became non-zero, because
		// nextBreathTime was always only ~1 second away and would fire before
		// pulseProcessChild could reschedule it.  With nextBreathTime an hour out,
		// the timer cannot accidentally fire during the 0->positive rate transition.
		// breathInterval is set to 60 s so any stray reads get a sane value.
		breathInterval = 60000;
		nextBreathTime = GetTickCount64() + 3600000ULL;	// 1 hour away
		return;
	}

	resetTimer(bpm, NOT_CARDIAC, 0 );
}
#ifdef _WIN32
HANDLE pusleTimerH;
HANDLE bcastTimerH;
SECURITY_DESCRIPTOR timerSecDesc;

_SECURITY_ATTRIBUTES timerSecAttr
{
	sizeof(_SECURITY_ATTRIBUTES),

};
#endif  // _WIN32

int
pulseTask(void )
{
	int portno = PORT_PULSE;
	int i;
	int error;
	char* sesid = NULL;
	SOCKET sfd;
	SOCKET cfd;
	struct sockaddr client_addr;
	socklen_t socklen;
	WSADATA w;
	int found;
	printf("Pulse is on port %d\n", portno);

	

	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	{
		DWORD dwError;
		dwError = GetLastError();
		_tprintf(TEXT("Failed to enter background mode (%d)\n"), dwError);
	}

	DWORD dwThreadPri;
	dwThreadPri = GetThreadPriority(GetCurrentThread());
	_tprintf(TEXT("pulseTask: Current thread priority is 0x%x\n"), dwThreadPri);


	// Seed rand, needed for vpc array generation
	srand((unsigned)time(NULL));

	currentPulseRate = simmgr_shm->status.cardiac.rate;
	pulseSema.lock();
	set_pulse_rate(currentPulseRate);
	pulseSema.unlock();
	simmgr_shm->status.cardiac.pulseCount = 0;
	simmgr_shm->status.cardiac.pulseCountVpc = 0;

	currentBreathRate = simmgr_shm->status.respiration.rate;
	breathSema.lock();
	set_breath_rate(currentBreathRate);
	breathSema.unlock();
	simmgr_shm->status.respiration.breathCount = 0;

	//printf("Pulse Interval %llu Next %llu now %llu\n", pulseInterval, nextPulseTime, simmgr_shm->server.msec_time );
	//printf("Calling start_task for pulseProcessChild\n");
	(void)start_task("pulseProcessChild", pulseProcessChild);
	(void)start_task("pulseTimer", pulseTimer);
	(void)start_task("pulseBroadcastLoop", pulseBroadcastLoop);
	
	for (i = 0; i < MAX_LISTENERS; i++)
	{
		listeners[i].allocated = 0;
		simmgr_shm->simControllers[i].allocated = 0;
	}

	error = WSAStartup(0x0202, &w);  // Fill in WSA info
	if (error)
	{
		cout << "WSAStartup fails: " << GetLastErrorAsString();
		return false;                     //For some reason we couldn't start Winsock
	}
	if (w.wVersion != 0x0202)             //Wrong Winsock version?
	{
		WSACleanup();
		ios::fmtflags f(cout.flags());
		cout << "WSAStartup Bad Version: " << hex << w.wVersion;
		cout.flags(f);
		return false;
	}

	SOCKADDR_IN addr;                     // The address structure for a TCP socket

	addr.sin_family = AF_INET;            // Address family
	addr.sin_port = htons(portno);       // Assign port to this socket

    //Accept a connection from any IP using INADDR_ANY
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket

	if (sfd == INVALID_SOCKET)
	{
		cout << "pulseProcess - socket(): INVALID_SOCKET " << GetLastErrorAsString();
		return false;                     //Don't continue if we couldn't create a //socket!!
	}

	int enableKeepAlive = 1;
	setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enableKeepAlive, sizeof(enableKeepAlive));

	if ( ::bind(sfd, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR )
	{
		//We couldn't bind (this will happen if you try to bind to the same  
		//socket more than once)
		cout << "pulseProcess - bind(): SOCKET_ERROR " << GetLastErrorAsString();
		return false;
	}

	int listen_result = listen(sfd, SOMAXCONN);
	if (listen_result == SOCKET_ERROR)
	{
		printf("Listen failed with error: %lu\n", WSAGetLastError());
		closesocket(sfd);
		WSACleanup();
		return false;
	}
	socklen = sizeof(struct sockaddr_in);

	while (1)
	{
		cfd = accept(sfd, (struct sockaddr*)&client_addr, &socklen);
		if (cfd >= 0)
		{
			// Limit TCP retransmit time so a hard power-off is detected quickly.
			// Without this, Windows TCP may retransmit silently for several minutes
			// before send() returns an error.  TCP_MAXRT = 10 causes TCP to give up
			// after 10 seconds, so the next broadcast_word() call will fail and
			// simControllers[i].allocated will be cleared within ~10-12 seconds.
			// TCP_MAXRT is a Windows-specific socket option.  The previous
			// "#ifndef TCP_MAXRT / #define TCP_MAXRT 5" fallback also applied on
			// macOS/Linux, where IPPROTO_TCP option 5 is not TCP_MAXRT at all --
			// that call just failed silently with ENOPROTOOPT.  Guard it properly.
#ifdef _WIN32
#ifndef TCP_MAXRT
#define TCP_MAXRT 5   // defined in ws2def.h on most Windows SDKs
#endif
			int maxRetrySeconds = 10;
			setsockopt(cfd, IPPROTO_TCP, TCP_MAXRT, (const char*)&maxRetrySeconds, sizeof(maxRetrySeconds));
#endif

			// Disable Nagle's algorithm.  Every message sent here is tiny ("pulse\n"
			// is 6 bytes) and latency-critical, and none of them benefit from being
			// coalesced, so TCP_NODELAY is the correct setting for this socket.
			//
			// Note: this was NOT the cause of the intermittent irregular heart sound.
			// Nagle only withholds a small segment while earlier data is still
			// unacknowledged; at a 500 ms beat interval the previous segment has been
			// ACKed many times over before the next beat is queued, so Nagle never
			// engages here.  Keeping the option set anyway -- it is correct, and it
			// does matter for the back-to-back pulse+breath case where two messages
			// are written in the same millisecond.
			int noDelay = 1;
			setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

			char newIpAddr[STR_SIZE];
			sprintf_s(newIpAddr, STR_SIZE, "%d.%d.%d.%d",
				client_addr.sa_data[2] & 0xff,
				client_addr.sa_data[3] & 0xff,
				client_addr.sa_data[4] & 0xff,
				client_addr.sa_data[5] & 0xff
			);
#if 0
			// Change to restrict to one controller only
			if (listeners[0].allocated == 1 )
			{
				printf("Closing Controller Socket\n");
				closesocket(listeners[i].cfd);
			}
			listeners[0].allocated = 1;
			listeners[0].cfd = cfd;
			listeners[0].thread_no = i;
			simmgr_shm->simControllers[0].allocated = 1;
			sprintf_s(simmgr_shm->simControllers[0].ipAddr, STR_SIZE, "%d.%d.%d.%d",
				client_addr.sa_data[2] & 0xff,
				client_addr.sa_data[3] & 0xff,
				client_addr.sa_data[4] & 0xff,
				client_addr.sa_data[5] & 0xff
			);
			printf("Connecting Controller %d.%d.%d.%d\n",
				client_addr.sa_data[2] & 0xff,
				client_addr.sa_data[3] & 0xff,
				client_addr.sa_data[4] & 0xff,
				client_addr.sa_data[5] & 0xff
			);
			// Send the Status Port Number to the listener
			sendStatusPort(i);
			printf("Send Status Port complete\n");
			found = 1;
#else
			// Check for reopen from an existing controller
			found = 0;
			for (i = 0; i < MAX_LISTENERS; i++)
			{
				if (listeners[i].allocated == 1 && strcmp(newIpAddr, simmgr_shm->simControllers[i].ipAddr) == 0)
				{
					closesocket(listeners[i].cfd);
					listeners[i].cfd = cfd;
					found = 1;
					printf("ReOpened: %s\n", newIpAddr);
					// Send the Status Port Number to the listener
					sendStatusPort(i);

					break;
				}
			}
			if (found == 0)
			{
				for (i = 0; i < MAX_LISTENERS && found == 0; i++)
				{
					if (listeners[i].allocated == 0)
					{
						listeners[i].allocated = 1;
						listeners[i].cfd = cfd;
						listeners[i].thread_no = i;
						simmgr_shm->simControllers[i].allocated = 1;
						sprintf_s(simmgr_shm->simControllers[i].ipAddr, STR_SIZE, "%d.%d.%d.%d",
							client_addr.sa_data[2] & 0xff,
							client_addr.sa_data[3] & 0xff,
							client_addr.sa_data[4] & 0xff,
							client_addr.sa_data[5] & 0xff
						);
						printf("%d.%d.%d.%d\n",
							client_addr.sa_data[2] & 0xff,
							client_addr.sa_data[3] & 0xff,
							client_addr.sa_data[4] & 0xff,
							client_addr.sa_data[5] & 0xff
						);
						// Send the Status Port Number to the listener
						sendStatusPort(i);
						getControllerVersion(i);

						found = 1;
					}
				}

			}
			if (found == 0)
			{
				// Unable to allocate
				closesocket(cfd);
			}
#endif
		}
	}
	sprintf_s(p_msg, BUF_SIZE, "simpulse terminates");
	log_message("", p_msg);
	exit(222);
}

/*
 * FUNCTION: sendStatusPort
 *
 * ARGUMENTS:
 *		listener - Index of listener
 *
 * RETURNS:
 *		Never
 *
 * DESCRIPTION:
 *		Send the port number to the indicated listener.
*/
void
sendStatusPort(int listener)
{
	SOCKET fd;
	int len;
	char pbuf[64];

	sprintf_s(pbuf, "statusPort:%d", PORT_STATUS);
	len = (int)strlen(pbuf);

	if (listeners[listener].allocated == 1)
	{
		fd = listeners[listener].cfd;
		len = send(fd, pbuf, len, 0);
	}
}

/*
 * FUNCTION: broadcast_word
 *
 * ARGUMENTS:
 *		ptr - Unused
 *
 * RETURNS:
 *		Never
 *
 * DESCRIPTION:
 *		This process monitors the pulse and breath counts. When incremented (by the beat_handler)
 *		a message is sent to the listeners.
*/
int
broadcast_word(char* word)
{
	int count = 0;
	SOCKET fd;
	size_t len;
	int sendResult;  // send() returns int; SOCKET_ERROR = -1. Must NOT store in size_t
	                 // because size_t is unsigned and "size_t < 0" is always false.
	int i;

	for (i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i].allocated == 1)
		{
			fd = listeners[i].cfd;
			len = strlen(word);
			//printf("Send %s (%d) to %d - ", word, len, i);

			// Time the send() itself.  These sockets are in blocking mode, so if the
			// controller ever stops reading long enough to fill the kernel send
			// buffer, send() blocks here and stalls the beat path.  Recording the
			// duration tells us whether that is happening -- a healthy 6-byte send
			// is a few microseconds.
			auto sendT0 = std::chrono::steady_clock::now();
			sendResult = send(fd, word, (int)len, 0);
			auto sendT1 = std::chrono::steady_clock::now();
			{
				long long us = (long long)std::chrono::duration_cast<std::chrono::microseconds>(sendT1 - sendT0).count();
				if (us > sendMaxUsec) sendMaxUsec = us;
			}
			//printf("%d\n", sendResult);
			if (sendResult < 0) // This detects closed or disconnected listeners.
			{
				printf("Close listener %d\n", i);
				closesocket(fd);
				listeners[i].allocated = 0;
				simmgr_shm->simControllers[i].allocated = 0;
			}
			else
			{
				count++;
			}
		}
	}
	return (count);
}

/*
 * FUNCTION: process_child
 *
 * ARGUMENTS:
 *		ptr - Unused
 *
 * RETURNS:
 *		Never
 *
 * DESCRIPTION:
 *		This process monitors the pulse and breath counts. When incremented (by the beat_handler)
 *		a message is sent to the listeners.
 *		It also monitors the rates and adjusts the timeout for the beat_handler when a rate is changed.
*/
void
pulseTimer(void)
{
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	{
		DWORD dwError;
		dwError = GetLastError();
		_tprintf(TEXT("Failed to elevate priority (%d)\n"), dwError);
	}
	DWORD dwThreadPri;
	dwThreadPri = GetThreadPriority(GetCurrentThread());
	_tprintf(TEXT("pulseTimer: Current thread priority is 0x%x\n"), dwThreadPri);

	ULONGLONG now;
	ULONGLONG now2;
	while (1)
	{
		sim_sleep_ms(1);
		// Read wall-clock time directly — do NOT use simmgr_shm->server.msec_time here.
		// That field is updated by hrcheck_handler() in a separate non-time-critical thread.
		// If that thread is delayed by the OS (context switch, contention), the field can
		// be stale for 50-200 ms, causing beats to fire late.  The NEXT beat then fires at
		// its correct absolute time, making the interval between two consecutive beats
		// audibly short (sounds like an arrhythmia in sinus rhythm).
		// This thread is already THREAD_PRIORITY_TIME_CRITICAL — reading GetTickCount64()
		// directly gives precise, independent timing with no cross-thread dependency.
		now = GetTickCount64();
		if (nextPulseTime <= now)
		{
			pulse_beat_handler();
			nextPulseTime += pulseInterval;
			now2 = GetTickCount64();
			if (nextPulseTime <= (now2+1))
			{
				nextPulseTime = now2;
			}
		}
		now = GetTickCount64();
		if (nextBreathTime <= now)
		{
			breath_beat_handler();
			nextBreathTime += breathInterval;
			now2 = GetTickCount64();
			if (nextBreathTime <= (now2+1))
			{
				nextBreathTime = now2 + breathInterval;
			}
		}
	}
	printf("pulseTimer Exit\n");
	exit(205);
}
void
pulseBroadcastLoop(void)
{
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	{
		DWORD dwError;
		dwError = GetLastError();
		_tprintf(TEXT("Failed to elevate priority (%d)\n"), dwError);
	}
	DWORD dwThreadPri;
	dwThreadPri = GetThreadPriority(GetCurrentThread()); 
	_tprintf(TEXT("pulseBroadcastLoop: Current thread priority is 0x%x\n"), dwThreadPri);

	int count;
	int portUpdateLoops = 0;
	char pbuf[64];
	unsigned int last_pulse = simmgr_shm->status.cardiac.pulseCount;
	unsigned int last_pulseVpc = simmgr_shm->status.cardiac.pulseCountVpc;
	unsigned int last_breath = simmgr_shm->status.respiration.breathCount;
	unsigned int last_manual_breath = simmgr_shm->status.respiration.manual_count;

	while (1)
	{
		// Poll at 1 ms.  This loop is what actually delivers the "pulse" message to
		// the hardware controller, so any delay here lands directly on the audible
		// heart sound.  A 10 ms poll added a second quantization stage on top of
		// pulseTimer's, and the two loops drifted against each other, producing an
		// occasional short RR interval.  With 1 ms timer resolution now requested at
		// startup (see platform.h), this delivers beats within ~1 ms of generation.
		sim_sleep_ms(1);

		portUpdateLoops++;

		// Manual-breath bookkeeping involves no network I/O, so it is not gated.
		if (last_manual_breath != simmgr_shm->status.respiration.manual_count)
		{
			last_manual_breath = simmgr_shm->status.respiration.manual_count;
			simmgr_shm->status.respiration.breathCount++;
			printf("[BREATH-MANUAL] pulseBroadcastLoop manual_count change: breathCount=%u rate=%d manual_count=%u\n",
				simmgr_shm->status.respiration.breathCount,
				simmgr_shm->status.respiration.rate,
				simmgr_shm->status.respiration.manual_count);
		}

		/*
		 * ONE MESSAGE PER READ WINDOW.
		 *
		 * The sim-ctl controller (sim-ctl/comm/simCtlComm.cpp, simCtlComm::wait)
		 * reads up to 31 bytes and then matches ONLY the start of that buffer:
		 *
		 *     len = read(commFD, buffer, SM_BUF_MAX-1);
		 *     if (strncmp(buffer, "pulse",  5) == 0) return SYNC_PULSE;
		 *     else if (strncmp(buffer, "breath", 6) == 0) return SYNC_BREATH;
		 *     else  "bad sync msg"
		 *
		 * Anything after the first message in that read is silently discarded, and
		 * a read that does not START with a known keyword is discarded entirely.
		 * The controller's socket is left non-blocking, so it polls roughly every
		 * millisecond -- meaning any two messages we write within ~1 ms of each
		 * other are very likely to be collected by a single read() and one of them
		 * is thrown away.  A discarded "pulse" is a lost heartbeat.
		 *
		 * That is why "statusPort:..." is especially damaging: the controller has no
		 * case for it, so if a beat is coalesced behind one, the whole read is
		 * rejected as a bad sync message and the beat is lost.
		 *
		 * We cannot reflash the controller from here, so the sender guarantees the
		 * separation instead: at most one message per CONTROLLER_MSG_GAP_MS, in
		 * priority order, with anything not sent left pending for a later pass.
		 * A breath arriving a few ms late is inaudible; a dropped beat is not.
		 *
		 * A beat is NEVER delayed by this gate -- it always goes out immediately.
		 * Only the lower-priority messages wait, so the gap can never itself become
		 * a source of late heart sounds.
		 */
		const ULONGLONG CONTROLLER_MSG_GAP_MS = 15;	// >> the controller's ~1 ms poll
		static ULONGLONG lastMsgMsec = 0;
		ULONGLONG msgNow = GetTickCount64();
		int gapElapsed = ((msgNow - lastMsgMsec) >= CONTROLLER_MSG_GAP_MS);

		if (last_pulse != simmgr_shm->status.cardiac.pulseCount)
		{
			last_pulse = simmgr_shm->status.cardiac.pulseCount;
			lastMsgMsec = msgNow;	// pushes any pending breath/status back
			count = broadcast_word(pulseWord);

			// Beat-interval instrumentation.  See beatTrace notes near the top of
			// pulseBroadcastLoop.  Measures GENERATION timing (send() returns as soon
			// as the data is buffered), so a quiet log means beats leave this process
			// evenly and any remaining irregularity was added downstream.
			{
				static ULONGLONG lastBeatMsec = 0;
				static long long lastExpected = 0;
				ULONGLONG beatNow = GetTickCount64();
				long long expected = (long long)pulseInterval;
				// pulseTimer runs a 10-phase counter for VPC/afib, so a delivered
				// beat spans 10 intervals in those modes.
				if ((vpcType > 0) || (afibActive))
				{
					expected *= 10;
				}
				if (lastBeatMsec != 0)
				{
					// Only ask the drain to judge this interval when the rate held
					// steady across it.  Otherwise every rate change reports a false
					// deviation (the 80->120 change logged "interval 750, expected
					// 500" -- 750 was correct for the old rate).  Passing expected=0
					// means "trace it, but do not flag it".
					long long judge = (lastExpected > 0 && lastExpected == expected) ? lastExpected : 0;
					beatTracePush("pulse", beatNow, (long long)(beatNow - lastBeatMsec), judge, count);
				}
				lastBeatMsec  = beatNow;
				lastExpected  = expected;
			}

			if (count)
			{
#ifdef DEBUG
				//printf("Pulse sent to %d listeners\n", count);
#endif
			}
		}
		// "else if" from here down: a beat has priority over a breath, and a breath
		// over the status-port announcement.  Whatever is not sent this pass stays
		// pending (its last_* value is only updated when the message actually goes
		// out) and is picked up on the next pass, one gap later.
		else if (last_pulseVpc != simmgr_shm->status.cardiac.pulseCountVpc)
		{
			last_pulseVpc = simmgr_shm->status.cardiac.pulseCountVpc;
			lastMsgMsec = msgNow;
			count = broadcast_word(pulseWordVPC);

			// A VPC message makes the controller sound a beat.  If one is emitted while
			// the rhythm is supposed to be plain sinus (vpcType == 0) that is a spurious
			// beat landing next to a normal one.  "extra" carries vpcType, so extra=0
			// on a pulseVPC line means VPCs were disabled.
			beatTracePush("pulseVPC", GetTickCount64(), 0, 0, vpcType);
		}
		else if (gapElapsed && (last_breath != simmgr_shm->status.respiration.breathCount))
		{
			last_breath = simmgr_shm->status.respiration.breathCount;
			lastMsgMsec = msgNow;
			count = broadcast_word(breathWord);

			beatTracePush("breath", GetTickCount64(), 0, 0, count);
		}
		else if (gapElapsed && (portUpdateLoops > 5000))	// ~5 s cadence at a 1 ms poll
		{
			// Sent last and on its own.  The controller does not understand this
			// message and will log it as a bad sync message -- harmless in isolation,
			// but it must never share a read() with a beat.
			portUpdateLoops = 0;
			lastMsgMsec = msgNow;
			sprintf_s(pbuf, "statusPort:%d", PORT_STATUS);
			broadcast_word(pbuf);
		}
	}
	printf("pulseBroadcastLoop exit\n");
	exit(206);
}
void
pulseProcessChild(void)
{
	int checkCount = 0;

	while (1)
	{
		sim_sleep_ms(50);		// 50 msec wait

		// Flush any beat trace records recorded by the time-critical broadcast
		// thread.  Done here so the file I/O never happens on the beat path.
		beatTraceDrain();

		if (strcmp(simmgr_shm->status.scenario.state, "Running") == 0)
		{
			// A place for code to run only when a scenario is active
		}
		else
		{
			
		}
		
		if (currentPulseRate != simmgr_shm->status.cardiac.rate)
		{
			pulseSema.lock();
			set_pulse_rate(simmgr_shm->status.cardiac.rate);
			currentPulseRate = simmgr_shm->status.cardiac.rate;
			pulseSema.unlock();
#ifdef DEBUG
			sprintf_s(p_msg, "Set Pulse to %d", currentPulseRate);
			log_message("", p_msg);
#endif
		}
		if (currentVpcFreq != simmgr_shm->status.cardiac.vpc_freq ||
				vpcType != simmgr_shm->status.cardiac.vpc_type)
		{
			currentVpcFreq = simmgr_shm->status.cardiac.vpc_freq;
			vpcType = simmgr_shm->status.cardiac.vpc_type;
			calculateVPCFreq();
			set_pulse_rate(simmgr_shm->status.cardiac.rate);

		}

		if (strncmp(simmgr_shm->status.cardiac.rhythm, "afib", 4) == 0 &&
			! afibActive )
		{
			afibActive = 1;
			set_pulse_rate(simmgr_shm->status.cardiac.rate);
		}
		else if (afibActive && strncmp(simmgr_shm->status.cardiac.rhythm, "afib", 4) != 0)
		{
			afibActive = 0;
			set_pulse_rate(simmgr_shm->status.cardiac.rate);

		}
		
		if (lastManualBreath != simmgr_shm->status.respiration.manual_count)
		{
			// Manual Breath has started. Reset timer to run based on this breath
			lastManualBreath = simmgr_shm->status.respiration.manual_count;
			breathSema.lock();
			restart_breath_timer();
			breathSema.unlock();
		}
		
		// If the breath rate has changed, then reset the timer
		if (currentBreathRate != simmgr_shm->status.respiration.rate)
		{
			int prevBreathRate = currentBreathRate;
			breathSema.lock();
			set_breath_rate(simmgr_shm->status.respiration.rate);
			currentBreathRate = simmgr_shm->status.respiration.rate;
			// When starting from a stopped state (rate 0), the placeholder 60 bpm timer
			// would fire almost immediately as the first "real" breath.  Reschedule
			// nextBreathTime to one full interval from now so the first breath arrives
			// at the correct spacing rather than firing spuriously right away.
			if (prevBreathRate == 0 && currentBreathRate > 0)
			{
				// Delay the first breath long enough that the rate has ramped to a
				// "visible" level so the waveform doesn't appear as a tiny spike.
				// At a typical 0.9 bpm/s ramp, rate~8 is reached ~9 seconds after the
				// rate first goes non-zero, giving ~28% ETCO2 amplitude and a natural
				// waveform shape.  breathInterval/7 (~8.6 s at rate=1 / 60s interval)
				// scales reasonably with different ramp speeds.
				// Fix-1 (resetTimer) will NOT pull this in because the remaining time
				// stays well below breathInterval for the entire wait.
				nextBreathTime = GetTickCount64() + (breathInterval / 7);
			}
			breathSema.unlock();

			// awRR Calculation - TBD - Need real calculations
			//simmgr_shm->status.respiration.awRR = simmgr_shm->status.respiration.rate;
#ifdef DEBUG
			sprintf_s(p_msg, "Set Breath to %d", currentBreathRate);
			log_message("", p_msg);
#endif
		}
	}
	printf("pulseProcessChild Exit");
	exit(204);
}

#ifdef _WIN32  // WinHTTP-based controller-version query (Windows built-in, no external deps)
#include <winhttp.h>
#include <string>
#include <vector>

// Reads a web page using the Windows WinHTTP API directly.
// No external libraries needed — WinHTTP is built into every Windows installation.
// Handles plain http://host/path URLs as used by the local CGI endpoints.
std::string ReadWebPage(const std::string& url)
{
	// Strip "http://" prefix
	std::string rest = url;
	if (rest.size() > 7 && rest.substr(0, 7) == "http://")
		rest = rest.substr(7);

	// Split into host and path
	std::string host, path;
	size_t slash = rest.find('/');
	if (slash == std::string::npos) {
		host = rest;
		path = "/";
	} else {
		host = rest.substr(0, slash);
		path = rest.substr(slash);
	}

	std::wstring whost(host.begin(), host.end());
	std::wstring wpath(path.begin(), path.end());

	HINTERNET hSession = WinHttpOpen(L"WinVetSim/1.0",
		WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return "";

	HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
		INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wpath.c_str(),
		NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
		!WinHttpReceiveResponse(hRequest, NULL))
	{
		std::cout << "WinHTTP request failed for URL: " << url << std::endl;
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	std::string response;
	DWORD dwSize = 0;
	while (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
		std::vector<char> buf(dwSize + 1, '\0');
		DWORD dwRead = 0;
		if (WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead))
			response.append(buf.data(), dwRead);
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return response;
}

#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

/*
 * Try to parse a full JSON text and extract the "simCtlVersion" value.
 * If the value is an object/array it will be serialized to a string.
 * Returns true if extraction succeeded and `out` contains the value.
 */
static bool try_parse_full_json_for_simCtlVersion(const std::string& text, std::string& out)
{
	try {
		auto j = json::parse(text);
		if (j.contains("simCtlVersion")) {
			if (j["simCtlVersion"].is_string())
				out = j["simCtlVersion"].get<std::string>();
			else
				out = j["simCtlVersion"].dump();
			return true;
		}
	}
	catch (...) {
		// not a pure JSON document
	}
	return false;
}

/*
 * Locate the token "simCtlVersion" inside an arbitrary page (HTML or JS),
 * extract the JSON/value that follows the ':' and return it as a string.
 * Handles values that are:
 *   - JSON object/array (starts with '{' or '[') -> returns the serialized JSON substring
 *   - JSON/string literal (starts with '"') -> returns the string content unquoted
 *   - bare token/number -> returns the token text
 */
static bool extract_simCtlVersion_from_mixed_text(const std::string& page, std::string& out)
{
	const char* token = "simCtlVersion";
	auto pos = page.find(token);
	if (pos == std::string::npos) return false;

	// find the ':' after the token
	pos = page.find(':', pos + strlen(token));
	if (pos == std::string::npos) return false;

	// advance to first non-space
	pos++;
	while (pos < page.size() && isspace((unsigned char)page[pos])) pos++;
	if (pos >= page.size()) return false;

	char c = page[pos];
	if (c == '{' || c == '[') {
		// extract balanced JSON block
		char open = c;
		char close = (open == '{') ? '}' : ']';
		size_t i = pos;
		int depth = 0;
		for (; i < page.size(); ++i) {
			if (page[i] == open) depth++;
			else if (page[i] == close) {
				depth--;
				if (depth == 0) {
					out = page.substr(pos, i - pos + 1);
					// validate by parsing
					try {
						auto j = json::parse(out);
						// if j is object/array and contains simCtlVersion nested, try to extract
						if (j.is_object() && j.contains("simCtlVersion")) {
							if (j["simCtlVersion"].is_string())
								out = j["simCtlVersion"].get<std::string>();
							else
								out = j["simCtlVersion"].dump();
						}
					}
					catch (...) {
						// accept raw substring anyway
					}
					return true;
				}
			}
			// skip over string literals to avoid matching braces inside quotes
			else if (page[i] == '"') {
				// skip quoted string
				i++;
				for (; i < page.size(); ++i) {
					if (page[i] == '\\') { i++; continue; }
					if (page[i] == '"') break;
				}
			}
		}
		return false;
	}
	else if (c == '"') {
		// quoted string
		size_t i = pos + 1;
		std::string tmp;
		for (; i < page.size(); ++i) {
			if (page[i] == '\\' && i + 1 < page.size()) {
				// handle simple escape sequences
				i++;
				switch (page[i]) {
				case 'n': tmp.push_back('\n'); break;
				case 'r': tmp.push_back('\r'); break;
				case 't': tmp.push_back('\t'); break;
				case '\\': tmp.push_back('\\'); break;
				case '"': tmp.push_back('"'); break;
				default: tmp.push_back(page[i]); break;
				}
			}
			else if (page[i] == '"') {
				out = tmp;
				return true;
			}
			else {
				tmp.push_back(page[i]);
			}
		}
		return false;
	}
	else {
		// bare token or number: read until comma, semicolon, newline or non-token char
		size_t i = pos;
		for (; i < page.size(); ++i) {
			char cc = page[i];
			if (cc == ',' || cc == ';' || cc == '\n' || cc == '\r' || cc == '<' || cc == '}' || cc == ']') break;
		}
		if (i > pos) {
			out = page.substr(pos, i - pos);
			// trim trailing spaces
			out.erase(out.find_last_not_of(" \t\r\n") + 1);
			// trim leading spaces
			out.erase(0, out.find_first_not_of(" \t\r\n"));
			// remove optional quotes
			if (!out.empty() && out.front() == '"' && out.back() == '"') {
				out = out.substr(1, out.size() - 2);
			}
			return !out.empty();
		}
		return false;
	}
}


static bool extract_simCtlVersion(const std::string& page, std::string& out)
{
	// Strategy 1: page is pure JSON
	if (try_parse_full_json_for_simCtlVersion(page, out)) return true;

	// Strategy 2: page contains an embedded JSON snippet or JS variable
	if (extract_simCtlVersion_from_mixed_text(page, out)) return true;

	// Strategy 3: try to locate a JSON substring that contains simCtlVersion key explicitly
	auto keyPos = page.find("\"simCtlVersion\"");
	if (keyPos != std::string::npos) {
		// try to find open brace before key and parse object
		auto bracePos = page.rfind('{', keyPos);
		if (bracePos != std::string::npos) {
			// attempt to find matching closing brace
			size_t i = bracePos;
			int depth = 0;
			for (; i < page.size(); ++i) {
				if (page[i] == '{') depth++;
				else if (page[i] == '}') {
					depth--;
					if (depth == 0) {
						std::string sub = page.substr(bracePos, i - bracePos + 1);
						try {
							auto j = json::parse(sub);
							if (j.contains("simCtlVersion")) {
								if (j["simCtlVersion"].is_string()) out = j["simCtlVersion"].get<std::string>();
								else out = j["simCtlVersion"].dump();
								return true;
							}
						} catch (...) { }
						break;
					}
				}
			}
		}
	}

	return false;
}

void getControllerVersion(int index) {
	if (listeners[index].allocated == 1)
	{
		char url[128];
		sprintf_s(url, "http://%s/%s", simmgr_shm->simControllers[index].ipAddr, "cgi-bin/ctlstatus.cgi");
		std::string page = ReadWebPage(url);
		std::string version;
		bool sts = extract_simCtlVersion(page, version);
		if ( sts )
		{
			// Store the version string
			strncpy_s(simmgr_shm->simControllers[index].version, STR_SIZE, version.c_str(), STR_SIZE - 1);
			cout << "Controller " << index << " simCtlVersion extraction " << ": " << version << std::endl;
		}
		else
		{
			cout << "Controller " << index << " simCtlVersion extraction " << "failed" << std::endl;
		}

	}
}
#else  // ! _WIN32
// POSIX stub — libcurl controller-version query is Windows-only
void getControllerVersion(int /*index*/) {}
#endif  // libcurl
