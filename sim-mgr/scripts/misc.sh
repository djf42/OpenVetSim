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

wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | sudo apt-key add -
sudo sh -c 'echo "deb http://dl.google.com/linux/chrome/deb/ stable main" >> /etc/apt/sources.list.d/google-chrome.list'
sudo apt-get update
sudo apt-get install -y google-chrome-stable

# sudo apt-get install -y vsftpd
sudo apt-get install -y openssh-server
sudo apt-get install -y xdotool
sudo apt-get install -y unclutter
sudo apt-get install -y tinc
sudo apt-get install -y net-tools

sudo add-apt-repository -y ppa:obsproject/obs-studio
sudo apt-get -y update
sudo apt-get install -y obs-studio

./updateVitals.sh

# update permissions
chown www-data:www-data /var/www
chmod 0755 /var/www/html
chown -R www-data:www-data /var/www/html

sudo mkdir /var/www/html/simlogs
sudo chown www-data:www-data /var/www/html/simlogs
sudo chmod 2777 /var/www/html/simlogs
chmod g+s /var/www/html/simlogs

# create video directory for OBS
mkdir /var/www/html/simlogs/video
chmod g+sw /var/www/html/simlogs/video

# clone the sim-ii
cd /var/www/html
git clone https://github.com/tkelleher/sim-ii.git

cd /var/www/html
git clone https://github.com/tkelleher/scenarios.git

# clone sim-player
cd /var/www/html/
git clone https://github.com/tkelleher/sim-player.git

# update permissions for scenarios
cd /var/www/html
chmod -R 777 ./scenarios
