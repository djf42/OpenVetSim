# OpenVetSim — C++ Simulation Engine
## Cross-platform simulation engine (macOS / Windows)
All projects within the archive are licensed under GPL v3.0. See LICENSE.

Originally developed as WinVetSim using Microsoft Visual Studio Community 2022; now cross-platform via CMake.


## Release Notes
### OpenVetSim SIMMGR Version 2.2 5/1/2025
* Add improved Group Trigger support.<br>
	&nbsp;&nbsp;&nbsp;&nbsp;Move xml parse functions from scenario.cpp to new file, scenario_xml.cpp
	
* Add new trigger targets to allow trigger on calculated rates rather than programmed rates:<br>
	&nbsp;&nbsp;&nbsp;&nbsp;cardiac:avg_rate<br>
	&nbsp;&nbsp;&nbsp;&nbsp;respiration:awRR
	
* Change SimManager version from Major.Minor.BuildNumber to Major.Minor.DateCode<br>
	&nbsp;&nbsp;&nbsp;&nbsp;Date code is YYYYMMDDHH

* When registry key is not found, locate the html directories via OPENVETSIM_HTML_PATH environment variable or fallback paths.

* Update PHP executable search path 
