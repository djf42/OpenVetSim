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
RELEASE=`lsb_release -rs`

sudo apt-get -y install g++
# wget ftp://ftp.gnu.org/gnu/cgicc/cgicc-3.2.19.tar.gz
# tar xvf cgicc-3.2.19.tar.gz
# sudo mv cgicc /usr/include
if [[ ${RELEASE} == "18.04" ]]; 
then
	sudo apt-get install -y libcgicc-dev
else
	sudo apt-get install -y build-essential libcgicc5v5 libcgicc5-dev libcgicc-doc
fi

sudo apt-get install -y libxml2 libxml2-dev

## Should add check here and skip if /var/lib/cgi-bin already exists
sudo mkdir /var/lib/cgi-bin
sudo chown www-data:www-data /var/lib/cgi-bin
sudo chmod 2777 /var/lib/cgi-bin

## Should add check here and skip if user simmgr already exists
sudo adduser simmgr
sudo cp simmgr_sudo /etc/sudoers.d
sudo chown root:root /etc/sudoers.d/simmgr_sudo
sudo chmod 0440 /etc/sudoers.d/simmgr_sudo
