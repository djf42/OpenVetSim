/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	var buttons = {
		disconnectColor: "#B31B1B",
		connectColor: "green",
		
		init: function() {
			// init colors and label
			buttons.setVSButton("ekg");
			buttons.setVSButton("SpO2");
			buttons.setVSButton("Tperi");
			buttons.setVSButton("CO2");
			buttons.setVSButton("bpcuff");
			buttons.setCPRButton();
			
			// bind button events
			$('#button-ekg').click(function() {
				buttons.bindVSButton("ekg");
			});
			$('#button-SpO2').click(function() {
				buttons.bindVSButton("SpO2");
			});
			$('#button-Tperi').click(function() {
				buttons.bindVSButton("Tperi");
			});
			$('#button-CO2').click(function() {
				buttons.bindVSButton("CO2");
			});
			$('#button-bpcuff').click(function() {
				buttons.bindVSButton("bpcuff");
			});
			
			// remove border from nibp button and bind action
			$('#button-nbp').css('border', 'none');
			$('#button-nbp').click(function() {
				if(controls.nbp.nibp_read != 1) {
					simmgr.sendChange({'set:cardiac:nibp_read': 1});					
				}
			});
			
		},
		
		setVSButton: function(buttonType) {
			if(controls[buttonType].leadsConnected == false) {
				$('#button-' + buttonType).css({
					'background-color': buttons.disconnectColor
					}).prop('title', controls[buttonType].disconnectHTML);
			} else {
				$('#button-' + buttonType).css({
					'background-color': buttons.connectColor
					}).prop('title', controls[buttonType].connectHTML);			
			}
		},
		
		setCPRButton: function() {
			if(controls.cpr.inProgress == false) {
				$('#button-cpr').attr('src', BROWSER_IMAGES + 'empty.png');
			} else {
				$('#button-cpr').attr('src', BROWSER_IMAGES + 'heart.png');
			}
		},
		
		bindVSButton: function(buttonType) {
				var value = 0;
				if(controls[buttonType].leadsConnected == false) {
					value = 1;
				}
				
				// send command based on button type
				if(buttonType == 'ekg') {
					simmgr.sendChange({'set:cardiac:ecg_indicator': value});
				} else if(buttonType == 'SpO2') {
					simmgr.sendChange({'set:respiration:spo2_indicator': value});					
				} else if(buttonType == 'CO2') {
					simmgr.sendChange({'set:respiration:etco2_indicator': value});										
				} else if(buttonType == 'bpcuff') {
					simmgr.sendChange({'set:cardiac:bp_cuff': value});										
				} else if(buttonType == 'Tperi') {
					simmgr.sendChange({'set:general:temperature_enable': value});										
				}
		}
	}