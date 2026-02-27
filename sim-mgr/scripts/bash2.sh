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

sudo apt-get install -y phpmyadmin apache2-utils

if [[ ${RELEASE} == "18.04" ]]; 
then
	## echo Running 18.04.
	sudo apt install -y php-dev libmcrypt-dev php-pear
	sudo pecl channel-update pecl.php.net
	sudo pecl install mcrypt-1.0.1
else
	## echo Not Running 18.04.
	sudo apt-get install -y php-mbstring php7.0 php-gettext
	sudo apt-get install -y php7.0-zip
	sudo phpenmod mcrypt
fi

sudo phpenmod mbstring
sudo ln -s /usr/share/phpmyadmin /var/www/html
sudo service apache2 restart

#if [[ ${RELEASE} == "18.04" ]]; 
#then
#	echo "You must edit /etc/php/7.2/cli/php.ini and /etc/php/7.2/apache2/php.ini"
#	echo "to add 'extension=mcrypt.so'"
#fi
