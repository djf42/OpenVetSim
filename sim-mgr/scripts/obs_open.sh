#!/bin/bash
#
# This file is part of the sim-mgr distribution (https://github.com/OpenVetSimDevelopers/sim-mgr).
# 
# Copyright (c) 2019 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
# 
# This program is free software: you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by  
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License 
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#echo obs_open
#whoami
#echo Calling export
export DISPLAY=:0.0
#echo calling obs
obs &

#echo calling xdotool

xdotool search --name --sync --onlyvisible "obs" windowfocus
STATUS=$?
if [ $STATUS -eq 0 ]; then
	#xdotool getactivewindow windowmove 10000 10000
	xdotool getactivewindow windowminimize
	STATUS=$?
	if [ $STATUS -eq 0 ]; then
		echo "Failed window move"
	fi
else
	echo "Failed to get window focus"
	exit $STATUS
fi
xdotool search --name --sync "Vet School Simulator" windowfocus
