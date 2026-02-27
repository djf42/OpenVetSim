/*
 * simmgrVideo.cpp
 *
 * Video Recording Support.
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

#include "../include/obsmon.h"
#include "../include/simmgr.h"

using namespace std;

extern char msgbuf[];

/*
 * FUNCTION: initOBSSHM
 *
 * Open the shared memory space
 *
 * Parameters: create 0 = Open only, 1 = Create
 *
 * Returns: 0 for success
 */
int 	obsShmFp;					// The File for shared memory
struct	obsShmData *obsShm;			// Pointer to shared data

int
initOBSSHM(int create )
{
	void *space;
	int mmapSize;
	int permission;
	char shmFileName[512];
	
	mmapSize = sizeof(struct obsShmData );

	sprintf(shmFileName, "%s", OBS_SHM_NAME );
	
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
	obsShmFp = shm_open(shmFileName, permission , 0777 );
	if ( obsShmFp < 0 )
	{
		sprintf(msgbuf, "Could not open OBS SHM file(%d) \"%s\": Error  \"%s\"", obsShmFp, shmFileName, strerror(errno) );
		log_message("", msgbuf ); 
	}
	else
	{
		if ( mmapSize < 4096 )
		{
			mmapSize = 4096;
		}
		
		if (  create == OPEN_WITH_CREATE )
		{
			lseek(obsShmFp, mmapSize, SEEK_SET );
			write(obsShmFp, "", 1 );
			sprintf(shmFileName, "/dev/shm/%s", OBS_SHM_NAME );
			chmod(shmFileName, 0777 );
		}
		space = mmap((caddr_t)0,
					mmapSize, 
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					obsShmFp,
					0 );
		if ( space == MAP_FAILED )
		{
			sprintf(msgbuf, "mmap failed on OBS SHM file \"%s\": Error  \"%s\"", shmFileName, strerror(errno) );
			log_message("", msgbuf ); 
		}
		else
		{
			obsShm = (struct obsShmData *)space;
			if (  create == OPEN_WITH_CREATE )
			{
				obsShm->buff_size = MSG_BUFF_MAX;
				obsShm->next_write = 0;
				obsShm->next_read = 0;
				obsShm->obsRunning = 0;
				sprintf(msgbuf, "OBS SHM file(%d) \"%s\": Create Success  ", obsShmFp, shmFileName );
			}
			else
			{
				sprintf(msgbuf, "OBS SHM file(%d) \"%s\": Open Success  ", obsShmFp, shmFileName );
			}
			log_message("", msgbuf ); 
		}
	}
	
	return ( 0 );
}

/**
 * recordStartStop
 * @record - Start if 1, Stop if 0
 *
 * 
 */

void
recordStartStop(int record )
{
	
	if ( record )
	{
		if ( obsShm->obsRunning == 0 )
		{
			obsShm->buff[obsShm->next_write] = OBSMON_OPEN;
			obsShm->next_write = ( obsShm->next_write + 1 ) MOD obsShm->buff_size;
		}
		obsShm->buff[obsShm->next_write] = OBSMON_START;
		obsShm->next_write = ( obsShm->next_write + 1 ) MOD obsShm->buff_size;
	}
	else
	{
		sprintf(obsShm->newFilename, "%s", simmgr_shm->logfile.vfilename );
		
		obsShm->buff[obsShm->next_write] = OBSMON_STOP;
		obsShm->next_write = ( obsShm->next_write + 1 ) MOD obsShm->buff_size;
		
		
		obsShm->buff[obsShm->next_write] = OBSMON_CLOSE;
		obsShm->next_write = ( obsShm->next_write + 1 ) MOD obsShm->buff_size;
	}
}
int
getVideoFileCount(void )
{
	int file_count = 0;
	DIR * dirp;
	struct dirent * entry;

	dirp = opendir("/var/www/html/simlogs/video"); /* There should be error handling after this */
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_type == DT_REG) { /* If the entry is a regular file */
			 file_count++;
		}
	}
	closedir(dirp);
	return ( file_count );
}