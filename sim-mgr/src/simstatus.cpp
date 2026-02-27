/*
 * simstatus.cpp
 *
 * Open the SHM segment and provide status/control operations.
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

#include <iostream>
#include <vector>  
#include <string>  
#include <cstdlib>
#include <sstream>
#include <utility>
#include <algorithm>

#include "../include/simmgr.h" 

#include <cgicc/CgiDefs.h> 
#include <cgicc/Cgicc.h> 
#include <cgicc/HTTPHTMLHeader.h> 
#include <cgicc/HTMLClasses.h>  


using namespace std;
using namespace cgicc;

void sendStatus(void );
void sendQuickStatus(void );
void sendSimctrData(void );
int runningAsDemo = 0;

struct paramCmds
{
	char cmd[16];
};

void makejson(ostream & output, string key, string content)
{
    output << "\"" << key << "\":\"" << content << "\"";
}

std::vector<std::string> explode(std::string const & s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(token );
    }

    return result;
}

void
releaseLock(int taken )
{
	if ( taken )
	{
		releaseInstructorLock();
	}
}

struct paramCmds paramCmds[] =
{
	{ "check", },
	{ "uptime", },
	{ "date", },
	{ "",  }
};
int debug = 0;
int iiLockTaken = 0;
char buf[2048];		// Used for logging messages

/*
 * Function: simstatus_signal_handler
 *
 * Handle inbound signals.
 *		SIGHUP is ignored 
 *		SIGTERM closes the process
 *
 * Parameters: sig - the signal
 *
 * Returns: none
 */
void simstatus_signal_handler(int sig )
{
	switch(sig) {
	case SIGHUP:
	case SIGTERM:
		log_message("","terminate signal catched");
		releaseLock( iiLockTaken );
		
		exit(0);
		break;
	}
}

int
main( int argc, const char* argv[] )
{
    char buffer[256];
	char cmd[32];
    //struct tm* tm_info;
	//unsigned int val;
	//int index;
	int i;
	int set_count = 0;
	int sts;
	char *cp;
	char sesid[512] = { 0, };
	int userid = -1;
	
	std::string key;
	std::string value;
	std::string command;
	std::string theTime;
	
	std::vector<std::string> v;
	
	const_form_iterator iter;
	const_cookie_iterator c_iter;
	
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,simstatus_signal_handler); /* catch hangup signal */
	signal(SIGTERM,simstatus_signal_handler); /* catch kill signal */
	signal(SIGSEGV,signal_fault_handler );   // install our handler
	cout << "Content-Type: application/json\r\n\r\n";
	cout << "{\n";

	try 
	{
		Cgicc cgi;
		const CgiEnvironment& env = cgi.getEnvironment();
		for(c_iter = env.getCookieList().begin(); 
		c_iter != env.getCookieList().end(); 
			++c_iter ) 
		{
			key = c_iter->getName();
			if ( key.compare("PHPSESSID" ) == 0 )
			{
				value = c_iter->getValue();
				sprintf(sesid, "%s", value.c_str() );
				makejson(cout, key, sesid );
				cout << ",\n";
			}
			else if ( key.compare("simIIUserID" ) == 0 )
			{
				value = c_iter->getValue();
				userid = atoi(value.c_str() );
				makejson(cout, key, value );
				cout << ",\n";
			}
			else if ( key.compare("userID" ) == 0 )
			{
				value = c_iter->getValue();
				userid = atoi(value.c_str() );
				makejson(cout, key, value );cout << ",\n";
			}
		}
	}
	catch(exception& ex) 
	{
		// Caught a standard library exception
		makejson(cout, "status", "Fail");
		cout << ",\n    ";
		makejson(cout, "err", ex.what() );
		cout << "\n}\n";
		releaseLock( iiLockTaken );
		return ( 0 );
	}	
	if ( userid == 5 )
	{
		runningAsDemo = 1;
	}
	else
	{
		sesid[0] = 0;
		runningAsDemo = 0;
	}
	sts = initSHM(OPEN_ACCESS, sesid );
	if ( sts < 0 )
	{
		sprintf(buffer, "%d: %s %s", sts, "initSHM failed", sesid );
		makejson(cout, "error", buffer );
		cout << "\n}\n";
		return ( 0 );
	}

	i = 0;
	sprintf(cmd, "none" );
	try 
	{
		Cgicc cgi;

		// If any "set" commands are in the GET/POST list, then take the Instructor Interface lock
		for( iter = cgi.getElements().begin(); iter != cgi.getElements().end(); ++iter )
		{
			key = iter->getName();
			if ( key.compare(0, 4, "set:" ) == 0 )
			{
				if ( takeInstructorLock() != 0 )
				{
					makejson(cout, "status", "Fail");
					cout << ",\n    ";
					makejson(cout, "error", "Could not get Instructor Mutex");
					cout << "\n}\n";
					return ( 0 );
				}
				iiLockTaken = 1;
				break;
			}
		}
		
		// Parse the submitted GET/POST elements
		for( iter = cgi.getElements().begin(); iter != cgi.getElements().end(); ++iter )
		{
			key = iter->getName();
			value = iter->getValue();
			if ( i > 0 )
			{
				cout << ",\n";
			}
			i++;
			if ( key.compare("qstat" ) == 0 )
			{
				// The Quick Status
				sendQuickStatus();
			}
			else if ( key.compare("check" ) == 0 )
			{
				makejson(cout, "check", "check is ok" );
				cout << ",\n";
				cp = do_command_read("/usr/bin/uptime", buffer, sizeof(buffer)-1 );
				if ( cp == NULL )
				{
					makejson(cout, "uptime", "no data");
				}
				else
				{
					makejson(cout, "uptime", buffer);
				}
				cout << ",\n";
				makejson(cout, "ip_addr", simmgr_shm->server.ip_addr );
				cout << ",\n";
				makejson(cout, "wifi_ip_addr", simmgr_shm->server.wifi_ip_addr );
			}
			else if ( key.compare("uptime" ) == 0 )
			{
				cp = do_command_read("/usr/bin/uptime", buffer, sizeof(buffer)-1 );
				if ( cp == NULL )
				{
					makejson(cout, "uptime", "no data");
				}
				else
				{
					makejson(cout, "uptime", buffer);
				}
			}
			else if ( key.compare("date" ) == 0 )
			{
				get_date(buffer );
				makejson(cout, "date", buffer );
				//makejson(cout, "date", simmgr_shm->server.server_time );
			}
			else if ( key.compare("ip" ) == 0 )
			{
				makejson(cout, "ip_addr", simmgr_shm->server.ip_addr );
			}
			else if ( key.compare("host" ) == 0 )
			{
				makejson(cout, "hostname", simmgr_shm->server.name );
			}
			else if ( key.compare("time" ) == 0 )
			{
				theTime = std::string(simmgr_shm->server.server_time );
	
				if (!theTime.empty() && theTime[theTime.length()-1] == '\n') 
				{
					theTime.erase(theTime.length()-1);
				}
				makejson(cout, "time", theTime.c_str() );
			}
			else if ( key.compare("status" ) == 0 )
			{
				// The meat of the task - Return the content of the SHM
				sendStatus();
			}
			else if ( key.compare("simctrldata" ) == 0 )
			{
				// Return abbreviated status for sim controller
				sendSimctrData();
			}
			else if ( key.compare(0, 4, "set:" ) == 0 )
			{
				set_count++;
				// set command: Split to segments to construct the reference
				v = explode(key, ':');
				cout << " \"set_" << set_count << "\" : {\n    ";
				makejson(cout, "class", v[1] );
				cout << ",\n    ";
				makejson(cout, "param", v[2] );
				cout << ",\n    ";
				makejson(cout, "value", value );
				cout << ",\n    ";
				sts = 0;
				
				
				if ( v[1].compare("cardiac" ) == 0 )
				{
					sts = cardiac_parse(v[2].c_str(), value.c_str(), &simmgr_shm->instructor.cardiac );
				}
				else if ( v[1].compare("scenario" ) == 0 )
				{
					if ( v[2].compare("active" ) == 0 )
					{
						sprintf(simmgr_shm->instructor.scenario.active, "%s", value.c_str() );
					}
					else if ( v[2].compare("state" ) == 0 )
					{
						sprintf(simmgr_shm->instructor.scenario.state, "%s", value.c_str() );
					}
					else if ( v[2].compare("record" ) == 0 )
					{
						simmgr_shm->instructor.scenario.record = atoi(value.c_str() );
						}
					else
					{
						sts = 1;
					}
				}
				else if ( v[1].compare("respiration" ) == 0 )
				{
					sts = respiration_parse(v[2].c_str(), value.c_str(), &simmgr_shm->instructor.respiration );
				}
				else if ( v[1].compare("general" ) == 0 )
				{
					sts = general_parse(v[2].c_str(), value.c_str(), &simmgr_shm->instructor.general );
				}
				else if ( v[1].compare("vocals" ) == 0 )
				{
					sts = vocals_parse(v[2].c_str(), value.c_str(), &simmgr_shm->instructor.vocals );
				}
				else if ( v[1].compare("media" ) == 0 )
				{
					sts = media_parse(v[2].c_str(), value.c_str(), &simmgr_shm->instructor.media );
				}
				else if ( v[1].compare("event" ) == 0 )
				{
					if ( v[2].compare("event_id" ) == 0 )
					{
						if ( value.length() != 0 )
						{
							addEvent((char *)value.c_str() );
							sts = 0;
						}
						else
						{
							sts = 4;
						}
					}
					else if ( v[2].compare("comment" ) == 0 )
					{
						if ( value.length() != 0 )
						{
							sprintf(buf, "Comment: %s", (char *)value.c_str() );
							if ( strcmp(simmgr_shm->status.scenario.state, "Running" ) == 0 ||
								  strcmp(simmgr_shm->status.scenario.state, "Paused" ) == 0 )
							{
								addComment(buf );
								sts = 0;
							}
							else
							{
								addComment(buf );
								sts = 5;
							}
						}
						else
						{
							sts = 4;
						}
					}
					else
					{
						sts = 2;
					}
				}
				else if ( v[1].compare("cpr" ) == 0 )
				{
					if ( v[2].compare("compression" ) == 0 )
					{
						simmgr_shm->instructor.cpr.compression = atoi(value.c_str() );
						sts = 0;
					}
					else if ( v[2].compare("release" ) == 0 )
					{
						simmgr_shm->instructor.cpr.release = atoi(value.c_str() );
						sts = 0;
					}
					else
					{
						sts = 2;
					}
				}
				else if ( v[1].compare("pulse" ) == 0 )
				{
					if ( v[2].compare("right_dorsal" ) == 0 )
					{
						simmgr_shm->status.pulse.right_dorsal = atoi(value.c_str() );
						sts = 0;
					}
					else if ( v[2].compare("left_dorsal" ) == 0 )
					{
						simmgr_shm->status.pulse.left_dorsal = atoi(value.c_str() );
						sts = 0;
					}
					else if ( v[2].compare("right_femoral" ) == 0 )
					{
						simmgr_shm->status.pulse.right_femoral = atoi(value.c_str() );
						sts = 0;
					}
					else if ( v[2].compare("left_femoral" ) == 0 )
					{
						simmgr_shm->status.pulse.left_femoral = atoi(value.c_str() );
						sts = 0;
					}
					else
					{
						sts = 2;
					}
				}
				else if ( v[1].compare("auscultation" ) == 0 )
				{
					if ( v[2].compare("side" ) == 0 )
					{
						simmgr_shm->status.auscultation.side = atoi(value.c_str() );
						sts = 0;
					}
					else if ( v[2].compare("row" ) == 0 )
					{
						simmgr_shm->status.auscultation.row = atoi(value.c_str() );
						sts = 0;
					}
					else if ( v[2].compare("col" ) == 0 )
					{
						simmgr_shm->status.auscultation.col = atoi(value.c_str() );
						sts = 0;
					}
					else
					{
						sts = 2;
					}
				}
				else
				{
					sts = 2;
				}
				if ( sts == 1 )
				{
					makejson(cout, "status", "invalid param" );
				}
				else if ( sts == 2 )
				{
					makejson(cout, "status", "invalid class" );
				}
				else if ( sts == 3 )
				{
					makejson(cout, "status", "invalid parameter" );
				}
				else if ( sts == 4 )
				{
					makejson(cout, "status", "Null string in parameter" );
				}
				else if ( sts == 5 )
				{
					makejson(cout, "status", "Scenario is not running" );
				}
				else
				{
					makejson(cout, "status", "ok" );
				}
				cout << "\n    }";
			}
			else
			{
				makejson(cout, "Invalid Command", cmd );
			}
		}
	}
	catch(exception& ex) 
	{
		// Caught a standard library exception
		makejson(cout, "status", "Fail");
		cout << ",\n    ";
		makejson(cout, "err", ex.what() );
		cout << "\n}\n";
		releaseLock( iiLockTaken );
		return ( 0 );
	}
	
	cout << "\n}\n";
	releaseLock( iiLockTaken );
	return ( 0 );
}
void
sendSimctrData(void )
{
    char buffer[256];
	
	
	cout << " \"cardiac\" : {\n";
	makejson(cout, "vpc", simmgr_shm->status.cardiac.vpc	);
	cout << ",\n";
	makejson(cout, "pea", itoa(simmgr_shm->status.cardiac.pea, buffer, 10 )	);
	cout << ",\n";
	makejson(cout, "vpc_freq", itoa(simmgr_shm->status.cardiac.vpc_freq, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "vpc_delay", itoa(simmgr_shm->status.cardiac.vpc_delay, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "rate", itoa(simmgr_shm->status.cardiac.rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "avg_rate", itoa(simmgr_shm->status.cardiac.avg_rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_rate", itoa(simmgr_shm->status.cardiac.nibp_rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_read", itoa(simmgr_shm->status.cardiac.nibp_read, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_linked_hr", itoa(simmgr_shm->status.cardiac.nibp_linked_hr, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_freq", itoa(simmgr_shm->status.cardiac.nibp_freq, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "pulseCount", itoa(simmgr_shm->status.cardiac.pulseCount, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "pulseCountVpc", itoa(simmgr_shm->status.cardiac.pulseCountVpc, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "pwave", simmgr_shm->status.cardiac.pwave );
	cout << ",\n";
	makejson(cout, "pr_interval", itoa(simmgr_shm->status.cardiac.pr_interval, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "qrs_interval", itoa(simmgr_shm->status.cardiac.qrs_interval, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "bps_sys", itoa(simmgr_shm->status.cardiac.bps_sys, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "bps_dia", itoa(simmgr_shm->status.cardiac.bps_dia, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "arrest", itoa(simmgr_shm->status.cardiac.arrest, buffer, 10 ) );
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.right_dorsal_pulse_strength )
	{
		case 0:
			makejson(cout, "right_dorsal_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "right_dorsal_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "right_dorsal_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "right_dorsal_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "right_dorsal_pulse_strength", itoa(simmgr_shm->status.cardiac.right_dorsal_pulse_strength, buffer, 10) );
			break;
	}
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.left_dorsal_pulse_strength )
	{
		case 0:
			makejson(cout, "left_dorsal_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "left_dorsal_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "left_dorsal_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "left_dorsal_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "left_dorsal_pulse_strength", itoa(simmgr_shm->status.cardiac.left_dorsal_pulse_strength, buffer, 10) );
			break;
	}
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.right_femoral_pulse_strength )
	{
		case 0:
			makejson(cout, "right_femoral_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "right_femoral_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "right_femoral_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "right_femoral_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "right_femoral_pulse_strength", itoa(simmgr_shm->status.cardiac.right_femoral_pulse_strength, buffer, 10) );
			break;
	}
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.left_femoral_pulse_strength )
	{
		case 0:
			makejson(cout, "left_femoral_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "left_femoral_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "left_femoral_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "left_femoral_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "left_femoral_pulse_strength", itoa(simmgr_shm->status.cardiac.left_femoral_pulse_strength, buffer, 10) );
			break;
	}
	cout << ",\n";
	makejson(cout, "heart_sound_volume", itoa(simmgr_shm->status.cardiac.heart_sound_volume, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "heart_sound_mute", itoa(simmgr_shm->status.cardiac.heart_sound_mute, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "heart_sound", simmgr_shm->status.cardiac.heart_sound );
	cout << ",\n";
	makejson(cout, "rhythm", simmgr_shm->status.cardiac.rhythm );
	cout << "\n},\n";
	
	cout << " \"defibrillation\" : {\n";
	makejson(cout, "shock", itoa(simmgr_shm->status.defibrillation.shock, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"cpr\" : {\n";
	makejson(cout, "running", itoa(simmgr_shm->status.cpr.running, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"respiration\" : {\n";
	makejson(cout, "left_lung_sound", simmgr_shm->status.respiration.left_lung_sound );
	cout << ",\n";
	makejson(cout, "left_lung_sound_volume", itoa(simmgr_shm->status.respiration.left_lung_sound_volume, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "left_lung_sound_mute", itoa(simmgr_shm->status.respiration.left_lung_sound_mute, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "right_lung_sound", simmgr_shm->status.respiration.right_lung_sound );
	cout << ",\n";
	makejson(cout, "right_lung_sound_volume", itoa(simmgr_shm->status.respiration.right_lung_sound_volume, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "right_lung_sound_mute", itoa(simmgr_shm->status.respiration.right_lung_sound_mute, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "inhalation_duration", itoa(simmgr_shm->status.respiration.inhalation_duration, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "exhalation_duration", itoa(simmgr_shm->status.respiration.exhalation_duration, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "rate", itoa(simmgr_shm->status.respiration.rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "awRR", itoa(simmgr_shm->status.respiration.awRR, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "chest_movement", itoa(simmgr_shm->status.respiration.chest_movement, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "manual_count", itoa(simmgr_shm->status.respiration.manual_count, buffer, 10 ) );
	cout << "\n}\n";

}

void
sendStatus(void )
{
    char buffer[256];
	
	cout << " \"scenario\" : {\n";
	makejson(cout, "active", simmgr_shm->status.scenario.active );
	cout << ",\n";
	makejson(cout, "start", simmgr_shm->status.scenario.start );
	cout << ",\n";
	makejson(cout, "runtime", simmgr_shm->status.scenario.runtimeAbsolute );
	cout << ",\n";
	makejson(cout, "runtimeScenario", simmgr_shm->status.scenario.runtimeScenario );
	cout << ",\n";
	makejson(cout, "runtimeScene", simmgr_shm->status.scenario.runtimeScene );
	cout << ",\n";
	makejson(cout, "scene_name", simmgr_shm->status.scenario.scene_name );
	cout << ",\n";
	makejson(cout, "scene_id", itoa(simmgr_shm->status.scenario.scene_id, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "record", itoa(simmgr_shm->status.scenario.record, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "state", simmgr_shm->status.scenario.state );
	cout << "\n},\n";
	cout << " \"logfile\" : {\n";
	makejson(cout, "active", itoa(simmgr_shm->logfile.active, buffer, 10 ) );
	cout << ",\n";
	
	makejson(cout, "filename", simmgr_shm->logfile.filename );
	cout << ",\n";
	makejson(cout, "lines_written", itoa(simmgr_shm->logfile.lines_written, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"cardiac\" : {\n";
	makejson(cout, "rhythm", simmgr_shm->status.cardiac.rhythm	);
	cout << ",\n";
	makejson(cout, "vpc", simmgr_shm->status.cardiac.vpc	);
	cout << ",\n";
	makejson(cout, "pea", itoa(simmgr_shm->status.cardiac.pea, buffer, 10 )	);
	cout << ",\n";
	makejson(cout, "vpc_freq", itoa(simmgr_shm->status.cardiac.vpc_freq, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "vpc_delay", itoa(simmgr_shm->status.cardiac.vpc_delay, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "vfib_amplitude", simmgr_shm->status.cardiac.vfib_amplitude	);
	cout << ",\n";
	makejson(cout, "rate", itoa(simmgr_shm->status.cardiac.rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "avg_rate", itoa(simmgr_shm->status.cardiac.avg_rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_rate", itoa(simmgr_shm->status.cardiac.nibp_rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_read", itoa(simmgr_shm->status.cardiac.nibp_read, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_linked_hr", itoa(simmgr_shm->status.cardiac.nibp_linked_hr, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "nibp_freq", itoa(simmgr_shm->status.cardiac.nibp_freq, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "pulseCount", itoa(simmgr_shm->status.cardiac.pulseCount, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "pulseCountVpc", itoa(simmgr_shm->status.cardiac.pulseCountVpc, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "pwave", simmgr_shm->status.cardiac.pwave );
	cout << ",\n";
	makejson(cout, "pr_interval", itoa(simmgr_shm->status.cardiac.pr_interval, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "qrs_interval", itoa(simmgr_shm->status.cardiac.qrs_interval, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "bps_sys", itoa(simmgr_shm->status.cardiac.bps_sys, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "bps_dia", itoa(simmgr_shm->status.cardiac.bps_dia, buffer, 10 ) );
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.right_dorsal_pulse_strength )
	{
		case 0:
			makejson(cout, "right_dorsal_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "right_dorsal_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "right_dorsal_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "right_dorsal_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "right_dorsal_pulse_strength", itoa(simmgr_shm->status.cardiac.right_dorsal_pulse_strength, buffer, 10) );
			break;
	}
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.left_dorsal_pulse_strength )
	{
		case 0:
			makejson(cout, "left_dorsal_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "left_dorsal_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "left_dorsal_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "left_dorsal_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "left_dorsal_pulse_strength", itoa(simmgr_shm->status.cardiac.left_dorsal_pulse_strength, buffer, 10) );
			break;
			}
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.right_femoral_pulse_strength )
	{
		case 0:
			makejson(cout, "right_femoral_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "right_femoral_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "right_femoral_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "right_femoral_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "right_femoral_pulse_strength", itoa(simmgr_shm->status.cardiac.right_femoral_pulse_strength, buffer, 10) );
			break;
	}
	cout << ",\n";
	switch ( simmgr_shm->status.cardiac.left_femoral_pulse_strength )
	{
		case 0:
			makejson(cout, "left_femoral_pulse_strength", "none" );
			break;
		case 1:
			makejson(cout, "left_femoral_pulse_strength", "weak" );
			break;
		case 2:
			makejson(cout, "left_femoral_pulse_strength", "medium" );
			break;
		case 3:
			makejson(cout, "left_femoral_pulse_strength", "strong" );
			break;
		default:	// Should never happen
			makejson(cout, "left_femoral_pulse_strength", itoa(simmgr_shm->status.cardiac.left_femoral_pulse_strength, buffer, 10) );
			break;
	}
	cout << ",\n";
	makejson(cout, "heart_sound_volume", itoa(simmgr_shm->status.cardiac.heart_sound_volume, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "heart_sound_mute", itoa(simmgr_shm->status.cardiac.heart_sound_mute, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "heart_sound", simmgr_shm->status.cardiac.heart_sound );
	cout << ",\n";
	makejson(cout, "ecg_indicator", itoa(simmgr_shm->status.cardiac.ecg_indicator, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "bp_cuff", itoa(simmgr_shm->status.cardiac.bp_cuff, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "arrest", itoa(simmgr_shm->status.cardiac.arrest, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"respiration\" : {\n";
	makejson(cout, "left_lung_sound", simmgr_shm->status.respiration.left_lung_sound );
	cout << ",\n";
	makejson(cout, "left_lung_sound_volume", itoa(simmgr_shm->status.respiration.left_lung_sound_volume, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "left_lung_sound_mute", itoa(simmgr_shm->status.respiration.left_lung_sound_mute, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "right_lung_sound", simmgr_shm->status.respiration.right_lung_sound );
	cout << ",\n";
	makejson(cout, "right_lung_sound_volume", itoa(simmgr_shm->status.respiration.right_lung_sound_volume, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "right_lung_sound_mute", itoa(simmgr_shm->status.respiration.right_lung_sound_mute, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "inhalation_duration", itoa(simmgr_shm->status.respiration.inhalation_duration, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "exhalation_duration", itoa(simmgr_shm->status.respiration.exhalation_duration, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "breathCount", itoa(simmgr_shm->status.respiration.breathCount, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "spo2", itoa(simmgr_shm->status.respiration.spo2, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "etco2", itoa(simmgr_shm->status.respiration.etco2, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "rate", itoa(simmgr_shm->status.respiration.rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "awRR", itoa(simmgr_shm->status.respiration.awRR, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "etco2_indicator", itoa(simmgr_shm->status.respiration.etco2_indicator, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "spo2_indicator", itoa(simmgr_shm->status.respiration.spo2_indicator, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "chest_movement", itoa(simmgr_shm->status.respiration.chest_movement, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "manual_count", itoa(simmgr_shm->status.respiration.manual_count, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"auscultation\" : {\n";
	makejson(cout, "side", itoa(simmgr_shm->status.auscultation.side, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "row", itoa(simmgr_shm->status.auscultation.row, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "col", itoa(simmgr_shm->status.auscultation.col, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"general\" : {\n";
	makejson(cout, "temperature", itoa(simmgr_shm->status.general.temperature, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "temperature_enable", itoa(simmgr_shm->status.general.temperature_enable, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"vocals\" : {\n";
	makejson(cout, "filename", simmgr_shm->status.vocals.filename );
	cout << ",\n";
	makejson(cout, "repeat", itoa(simmgr_shm->status.vocals.repeat, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "volume", itoa(simmgr_shm->status.vocals.volume, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "play", itoa(simmgr_shm->status.vocals.play, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "mute", itoa(simmgr_shm->status.vocals.mute, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"pulse\" : {\n";
	makejson(cout, "right_dorsal", itoa(simmgr_shm->status.pulse.right_dorsal, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "left_dorsal", itoa(simmgr_shm->status.pulse.left_dorsal, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "right_femoral", itoa(simmgr_shm->status.pulse.right_femoral, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "left_femoral", itoa(simmgr_shm->status.pulse.left_femoral, buffer, 10 ) );
	cout << "\n},\n";

	cout << " \"media\" : {\n";
	makejson(cout, "filename", simmgr_shm->status.media.filename );
	cout << ",\n";
	makejson(cout, "play", itoa(simmgr_shm->status.media.play, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"cpr\" : {\n";
	makejson(cout, "last", itoa(simmgr_shm->status.cpr.last, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "running", itoa(simmgr_shm->status.cpr.running, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "compression", itoa(simmgr_shm->status.cpr.compression, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "release", itoa(simmgr_shm->status.cpr.release, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"defibrillation\" : {\n";
	makejson(cout, "last", itoa(simmgr_shm->status.defibrillation.last, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "shock", itoa(simmgr_shm->status.defibrillation.shock, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "energy", itoa(simmgr_shm->status.defibrillation.energy, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"debug\" : {\n";
	makejson(cout, "msec", itoa(simmgr_shm->server.msec_time, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "avg_rate", itoa(simmgr_shm->status.cardiac.avg_rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "debug1", itoa(simmgr_shm->server.dbg1, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "debug2", itoa(simmgr_shm->server.dbg2, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "debug3", itoa(simmgr_shm->server.dbg3, buffer, 10 ) );
	cout << "\n}\n";
}

void
sendQuickStatus(void )
{
    char buffer[256];

	
	cout << " \"cardiac\" : {\n";
	makejson(cout, "pulseCount", itoa(simmgr_shm->status.cardiac.pulseCount, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "pulseCountVpc", itoa(simmgr_shm->status.cardiac.pulseCountVpc, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "rate", itoa(simmgr_shm->status.cardiac.rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "avg_rate", itoa(simmgr_shm->status.cardiac.avg_rate, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"respiration\" : {\n";
	makejson(cout, "breathCount", itoa(simmgr_shm->status.respiration.breathCount, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "inhalation_duration", itoa(simmgr_shm->status.respiration.inhalation_duration, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "exhalation_duration", itoa(simmgr_shm->status.respiration.exhalation_duration, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "rate", itoa(simmgr_shm->status.respiration.rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "awRR", itoa(simmgr_shm->status.respiration.awRR, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "manual_count", itoa(simmgr_shm->status.respiration.manual_count, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"defibrillation\" : {\n";
	makejson(cout, "shock", itoa(simmgr_shm->status.defibrillation.shock, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"cpr\" : {\n";
	makejson(cout, "running", itoa(simmgr_shm->status.cpr.running, buffer, 10 ) );
	cout << "\n},\n";
	
	cout << " \"debug\" : {\n";
	makejson(cout, "msec", itoa(simmgr_shm->server.msec_time, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "avg_rate", itoa(simmgr_shm->status.cardiac.avg_rate, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "debug1", itoa(simmgr_shm->server.dbg1, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "debug2", itoa(simmgr_shm->server.dbg2, buffer, 10 ) );
	cout << ",\n";
	makejson(cout, "debug3", itoa(simmgr_shm->server.dbg3, buffer, 10 ) );
	cout << "\n}\n";
}