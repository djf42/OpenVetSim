/*
sim-ii: Copyright (C) 2020  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
/* 
	simulation sounds for TeleSim 
*/


var simsound = {
	currentHeartSound : 0,
	currentLungSound : 0,
	currentHeartLevel : 0,
	currentLungLevel : 0,
	heartSoundElement : 0,
	lungSoundElement : 0,
	heartCount : 0,
	
	init : function() {
	},
	
	playHeartSound : function() {
		if ( simsound.currentHeartSound > 0 && 
			simsound.heartSoundElement != 0 &&
			typeof(controls.auscultation) !== 'undefined' &&  
			controls.auscultation.side > 0 )
		{
			var level = 0;
			if ( typeof(simsound.tags[controls.auscultation.side] ) == 'undefined' )
				console.log("Side not in table" );
			else if ( typeof(simsound.tags[controls.auscultation.side][controls.auscultation.col] ) == 'undefined' )
				console.log("Col (X) not in table" ); 
			else if ( typeof(simsound.tags[controls.auscultation.side][controls.auscultation.col][controls.auscultation.row] ) == 'undefined' )
				console.log("Row (Y) not in table" ); 
			else
			{
				level = simsound.tags[controls.auscultation.side][controls.auscultation.col][controls.auscultation.row].heartStrength;
			}
	
			
			simsound.heartSoundElement.pause();
			simsound.heartSoundElement.currentTime = 0;
			if ( level > 0 )
			{
				level = level / 10;
			
				console.log("Heart Level", level, "Count", simsound.heartCount );
				simsound.heartCount ++;
				
				simsound.heartSoundElement.volume = level;
				playPromise = simsound.heartSoundElement.play();
				if (playPromise !== undefined) {
					playPromise.then(_ => {
					// Automatic playback started!
					// Show playing UI.
					})
					.catch(error => {
						console.log("Promise Error", error );
						var errorWord = error.substr(0, error.indexOf(" ") );
						if ( errorWord == "NotAllowedError" )
						{
							// Do something to tell user that he needs to click on a sound entry in the Vitals Screen
						}
					});
				}
			}
		}
	},
	
	playLungSound : function() {
		if ( simsound.currentLungSound > 0 && 
			simsound.lungSoundElement != 0 && 
			typeof(controls.auscultation) !== 'undefined' &&  
			controls.auscultation.side > 0 )
		{
			var level = 0;
			
			simsound.lungSoundElement.pause();
			simsound.lungSoundElement.currentTime = 0;
			if ( typeof(simsound.tags[controls.auscultation.side] ) == 'undefined' )
			{
				console.log("Side not in table" );
			}
			else if ( typeof(simsound.tags[controls.auscultation.side][controls.auscultation.col] ) == 'undefined' )
			{
				console.log("Col (X) not in table" );
			}
			else if ( typeof(simsound.tags[controls.auscultation.side][controls.auscultation.col][controls.auscultation.row] ) == 'undefined' )
			{
				console.log("Row (Y) not in table" );
			}
			else
			{
				level = simsound.tags[controls.auscultation.side][controls.auscultation.col][controls.auscultation.row].lungStrength;
			}
			if ( level > 0 )
			{
				level = level / 10;
			
				console.log("Lung Sound", simsound.currentLungSound , "Level", level, simsound.currentLungLevel );
				simsound.lungSoundElement.volume = level;
				playPromise = simsound.lungSoundElement.play();
				
				if (playPromise !== undefined) 
				{
					playPromise.then(_ => {
						// Automatic playback started!
					})
					.catch(error => {
						console.log("Promise Error", error );
						var errorWord = error.substr(0, error.indexOf(" ") );
						if ( errorWord == "NotAllowedError" )
						{
							// Do something to tell user that he needs to click on a sound entry in the Vitals Screen
						}
						// Auto-play was prevented
					});
				}
			}
		}
	},
	
	lookupLungSound : function() {
		console.log ("Lookup Lung sound is", simmgr.respResponse.left_lung_sound, simmgr.respResponse.rate  );
		$.each(soundPlayList, function(idx, snd) {
			console.log ( snd );
			if ( snd.type == 'lung' && 
				snd.name == simmgr.respResponse.left_lung_sound &&
				simmgr.respResponse.rate >= snd.low_limit &&
				simmgr.respResponse.rate <= snd.high_limit)
			{
				var index = snd.index;
				simsound.currentLungSound = index;
				simsound.lungSoundElement =  $('#snd_'+index).get(0);
				console.log ("Lung sound is " + index );
				return false;
			}
		});
	},
	
	lookupHeartSound : function() {
		console.log ("Lookup Heart sound is", controls.heartSound.soundName, controls.heartRate.value  );
		$.each(soundPlayList, function(idx, snd) {
			if ( snd.type == 'heart' && 
				snd.name == controls.heartSound.soundName &&  
				controls.heartRate.value >= snd.low_limit &&
				controls.heartRate.value <= snd.high_limit )
			{
				var index = snd.index;
				simsound.currentHeartSound = index;
				simsound.heartSoundElement = $('#snd_'+index).get(0);
				console.log ("Heart sound is " + index );
				return false;
			}
		});
	},
	
	stopSounds : function() {
		if ( currentLungSound > 0 && lungSoundElement != 0 )
		{
			simsound.currentLungSound = 0;
			lungSoundElement.pause();
		}
		if ( currentHeartSound > 0 && heartSoundElement != 0 )
		{
			simsound.currentHeartSound = 0;
			lungSoundElement.pause();
		}
	},
	parseTags : function() {
		// Tags are loaded to the scenario section and processed here
		simsound.tags = new Array();
		simsound.tags[1] = new Array();	// First dimension is the sides, 
		simsound.tags[2] = new Array();
		simsound.tags[3] = new Array();
		//console.log(scenario.soundtags.tag );
		$.each(scenario.soundtags.tag, function(idx, tag ) {
			console.log(idx, tag );
			if ( typeof(simsound.tags[tag.side][tag.xPosition] ) == 'undefined' )
			{
				simsound.tags[tag.side][tag.xPosition] = new Array();
			}
			simsound.tags[tag.side][tag.xPosition][tag.yPosition] = tag;
		});
		console.log(simsound.tags);
	}
	
}