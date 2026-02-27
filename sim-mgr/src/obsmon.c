/*
 * obsmon.c
 *
 * Monitor requests for Open Broadcaster Studio commands
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
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>

using namespace std;

int 	obsShmFp;					// The File for shared memory
struct	obsShmData *obsShm;			// Pointer to shared data
char	shmFileName[512];
int		mmapSize;
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
	int permission;

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
	obsShmFp = shm_open(shmFileName, permission , S_IRWXO | S_IRWXG | S_IRWXU );
	if ( obsShmFp < 0 )
	{
		fprintf(stderr, "%s %s", shmFileName, strerror(errno) );

		perror("shm_open" );
		//return ( -3 );
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
		//perror("mmap" );
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

time_t
getFileDate(char *fname )
{
	struct stat sbuf;
	int sts;
	time_t m_time = -1;
	
	sts = stat(fname, &sbuf );
	if ( sts == 0 )
	{
		m_time = sbuf.st_mtime;
	}
	return ( m_time );
}

off_t
getFileSize(char *fname )
{
	struct stat sbuf;
	int sts;
	off_t size = -1;
	
	sts = stat(fname, &sbuf );
	if ( sts == 0 )
	{
		size = sbuf.st_size;
	}
	return ( size );
}
int
getLatestFile(char *rname, char *dir )
{
	DIR *dirp;
	struct dirent *dp;
	time_t fileDate = 0;
	time_t latestDate = 0;
	
	char fname[512];
	char latestFname[512] = { 0, };
	
	dirp = opendir(dir );
	if ( dirp == NULL )
	{
		perror("opendir" );
		return -1;
	}
	while (( dp = readdir(dirp)) != NULL )
	{
		if ( strlen(dp->d_name ) > 5 )
		{
			sprintf(fname, "%s/%s", dir, dp->d_name );
			fileDate = getFileDate(fname );
			if ( fileDate > latestDate )
			{
				latestDate = fileDate;
				sprintf(latestFname, "%s/%s", dir, dp->d_name );
			}
		}
	}
	if ( strlen(latestFname ) != 0 )
	{
		sprintf(rname, "%s", latestFname );
		return ( 0 );
	}
	else
	{
		return ( -1 );
	}
}

int
renameVideoFile(char *filename )
{
	int sts;
	char *path; 
	char newFile[1024];
	char oldFile[1024];
	
	snprintf(oldFile, 1024, "%s", filename );
	path = dirname(filename );
	
	snprintf(newFile, 1024, "%s/%s", path, obsShm->newFilename );
	printf("Rename %s to %s\n", oldFile, newFile );
	sts = rename(oldFile, newFile );
	return ( sts );
}
#define MAX_FILE_WAIT_LOOPS 10

void
closeVideoCapture()
{
	int fsizeA = 0;
	int fsizeB = 0;
	char filename[512];
	char path[512];
	int loops = 0;
	int sts;

	// Wait for file to complete and then rename it.
	// 1 - Find the latest file in /var/www/html/simlogs/video
	sprintf(path, "%s", "/var/www/html/simlogs/video" );
	sts = getLatestFile(filename, path );
	if ( sts == 0 )
	{
		// Found file
		printf("Last Video File is: %s\n",  filename );
		
		// 2 - Wait until Stop has completed. Detected by no change in size for 2 seconds.
		fsizeA = getFileSize(filename );
		do {
			usleep ( 2000000 );
			fsizeB = getFileSize(filename );
			if ( fsizeA == fsizeB )
			{
				break;
			}
			fsizeA = fsizeB;
		} while ( loops++ < MAX_FILE_WAIT_LOOPS );
		
		// 3 - Rename the file
		if ( loops < MAX_FILE_WAIT_LOOPS )
		{
			sts = renameVideoFile(filename );
			if ( sts != 0 )
			{
				printf("renameVideoFile failed: %s",  strerror(errno ) );
			}
		}
		else
		{
			printf("File Size did not stop %d, %d", fsizeA, fsizeB );
		}
	}
}
int
main( int argc, char **argv )
{
	int cc;
	int sts;
	
	mmapSize = sizeof(struct obsShmData );
	sprintf(shmFileName, "%s", OBS_SHM_NAME );
	
	while ( 1 )
	{
		sts = initOBSSHM(OPEN_ACCESS );
		if ( sts < 0  )
		{
			// No SHM file
		}
		else
		{
			if ( obsShm->next_write != obsShm->next_read )
			{
				cc = obsShm->buff[obsShm->next_read];
				obsShm->next_read = ( obsShm->next_read + 1 ) MOD obsShm->buff_size;
				printf("%d\n", cc );
				switch ( cc )
				{
					case OBSMON_START:
						sts = system("obs_start.sh > /dev/null" );
						if ( sts == 0 )
						{
							obsShm->obsRunning = 1;
						}
						break;
					case OBSMON_STOP:
						sts = system("obs_stop.sh > /dev/null" );
						closeVideoCapture();
						break;
					case OBSMON_OPEN:
						sts = system("obs_open.sh > /dev/null" );
						break;
					case OBSMON_CLOSE:
						sts = system("obs_close.sh > /dev/null" );
						{
							obsShm->obsRunning = 0;
						}
						break;
					default:
						sts = -100;
						printf("obsmon: Unsupported Command %d\n", cc );
						break;
				}
			}
			close(obsShmFp );
		}
		usleep(500000 );
	}
	exit ( sts );
}