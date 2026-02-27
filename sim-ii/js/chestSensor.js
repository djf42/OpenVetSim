/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	var chestSensor = {
		status: {
			left: 0,
			right: 0
		},
		box_h: 10,
		box_l: 10,
		senseTimer: 0,
		updateInterval: 1000,
		
		// assume document is rendered before calling init.
		init: function() {
			senseTimer = setTimeout(get_sensor, updateInterval );
		},
		
		// Draw the sense indicator for the position indicated
		drawBox: function(row, col, set) {
			
		}
		
		get_sensor: function() {

			var dstr = "{";
				// Add any commands here
			dstr += "}";
	
			// Get the sensor readings from the chestMonitor
			$.ajax({
				url : "/cgi-bin/getChest.cgi",
				type : "POST",
				data : dstr,
				dataType : "json",
				timeout : 10000
				})
			.always(function() {
				senseTimer = setTimeout(get_sensor, updateInterval );
				$('.sensorMark').remove();
			})
			.done(function(json)
			{
				cs = json;
				var left = Number(cs.leftStatus);
				var i;
				var row = 0;
				var col = 0;
				var maxCol = 6;
				for ( i = 0 ; i < 24 ; i++ )
				{
					if ( left & (1<<i) )
					{
						console.log("Sensor "+i+" ("+col+" "+row+")");
					}
					col++;
					if ( col >= maxCol )
					{
						row++;
						col = 0;
					}
				}
			};			
		}
	}
