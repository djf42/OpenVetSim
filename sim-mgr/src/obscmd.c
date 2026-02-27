/*
 * obscmd.c
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

#include "../include/obsmon.h"

using namespace std;

int 	obsShmFp;					// The File for shared memory
struct	obsShmData *obsShm;			// Pointer to shared data

char getArgList[] = "sSoc";

/*
 * FUNCTION: initOBSSHM
 *
 * Open the shared memory space
 *
 * Parameters: create 0 = Open only, 1 = Create
 *
 * Returns: 0 for success
 */

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
	obsShmFp = shm_open(shmFileName, permission , 0755 );
	if ( obsShmFp < 0 )
	{
		fprintf(stderr, "%s %s", shmFileName, strerror(errno) );

		perror("shm_open" );
		return ( -3 );
	}
	if ( mmapSize < 4096 )
	{
		mmapSize = 4096;
	}
	
	if (  create == OPEN_WITH_CREATE )
	{
		lseek(obsShmFp, mmapSize, SEEK_SET );
		write(obsShmFp, "", 1 );
	}

	space = mmap((caddr_t)0,
				mmapSize, 
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				obsShmFp,
				0 );
	if ( space == MAP_FAILED )
	{
		perror("mmap" );
		return ( -4 );
	}
	
	obsShm = (struct obsShmData *)space;
	if (  create == OPEN_WITH_CREATE )
	{
		obsShm->buff_size = MSG_BUFF_MAX;
		obsShm->next_write = 0;
		obsShm->next_read = 0;
	}
	return ( 0 );
}


int
main( int argc, char **argv )
{	
	opterr = 0;
	int c;
	int sts ;
	
	sts = initOBSSHM(OPEN_ACCESS );
	if ( sts < 0  )
	{
		exit ( -1 );
	}
	
	c = getopt(argc, argv, getArgList );
	sts = 0;
	switch (c)
	{
		case 's': // start
			printf("Start\n");
			obsShm->buff[obsShm->next_write] = OBSMON_START;
			break;
		case 'S': // stop
			printf("Stop\n" );
			obsShm->buff[obsShm->next_write] = OBSMON_STOP;
			break;
		case 'o': // open
			printf("Open\n");
			obsShm->buff[obsShm->next_write] = OBSMON_OPEN;
			break;
		case 'c': // close
			printf("Close\n");
			obsShm->buff[obsShm->next_write] = OBSMON_CLOSE;
			break;
		default:
			if ( c == -1 )
			{
				printf("No option found\n" );
			}
			else if ( c == '?' )
			{
				printf("Invalid Option\n" );
			}
			sts = 1;
			break;
	}
	if ( sts == 0 )
	{
		obsShm->next_write = ( obsShm->next_write + 1 ) MOD obsShm->buff_size;
	}
	return ( sts );
}
