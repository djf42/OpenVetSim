/*
 * obsmon.h
 *
 * Signals for Open Broadcast Studio control
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

#ifndef _OBSMON_H
#define _OBSMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
	
#include <iostream>

#define MSG_BUFF_MAX	32
#define MOD		%


struct obsShmData
{
	int buff_size;
	int next_write;
	int next_read;
	int buff[MSG_BUFF_MAX];
	int obsRunning;
	char newFilename[512];
};
	
#define OBSMON_START	100
#define OBSMON_STOP		101
#define OBSMON_OPEN		102
#define OBSMON_CLOSE	103

#define OBS_SHM_NAME	"obs_shm"

#define	OPEN_WITH_CREATE		1
#define OPEN_ACCESS				0

#endif // _OBSMON_H