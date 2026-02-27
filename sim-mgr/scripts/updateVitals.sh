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
# 

# If Vitals user does not exist, create it.
getent passwd vitals  > /dev/null
if [ $? -eq 0 ]; then
	echo "Updating Vitals user"
else
	echo "Creating Vitals user."
	sudo adduser vitals
	sudo usermod -a -G www-data vitals
	
	## Add call to vitals.sh in the .profile for vitals.
	echo -e 'if [[ $XDG_VTNR == "1" ]];\n    then\n    $HOME/vitals.sh &\n    obsmon &\nfi\n' >> ~vitals/.profile
fi

sudo cp vitals.sh /home/vitals/
chown vitals:vitals /home/vitals/vitals.sh
sudo cp grabScreen.sh /home/vitals/
chown vitals:vitals /home/vitals/grabScreen.sh


