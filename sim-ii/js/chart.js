/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/

	// routine to get max of an array
	Array.prototype.max = function () {
		return Math.max.apply(Math, this);
	};
	
	var chart = {
		status: {
			cardiac: {
				heartRate: 0,
				synch: false,
				vpcSynch: false,
			},
			
			resp: {
				synch: false,
				manual: false
			}
		},
		
		displayETCO2: {
			max: 0
		},
		
		// baseline params for introducing sinusoid amplitude into generated waveform
		// params are fixed for no oscillations
		baselineP1: 0,
		baselineP2: 0,
		baselineUnit: 0.1,
		
		// fibrillation parameters
		fibP1: 0,
		fibP2: 0,
		fibP3: 0,
		
		// following params are fixed for high frequency filtering for vfib
		fibUnit1: 12,
		fibUnit2: 12,
		fibP1Constant: 4.3,
		fibP2Constant: 2.7,
		// ------------------
		
		fibP3ListIndex: 0,
//		fibP3List: [ 10, 9, 8, 9, 10, 11, 12, 13, 14,14,15,16,16,15,14,13,12,11, 10, 9, 8, 7, 6, 5, 4, 5, 6, 7, 8, 9, 10, 11, 12, 11, 10, 11, 12, 9, 8, 9 ],
		fibP3List: [ 10, 9, 8, 9, 10, 11, 12, 13, 14,14,15,16,16,15,14,13,12,11],
		fibDivide: 6, // amplitude of ventricular bibrillation
						// 4 = fine
						// 3 - medium
						// 1 - coarse
		
		vfib: {
			base: 0
		},
		
		afib: {
			delay: new Array,
			delayCount: 100,
			delayPtr: 0
		},

		// cpr status constants
		// delay stop in msec
		CPR_DELAY_NONE: 0,			// no delay in progress for cpr display of HR '----' 
		CPR_DELAY_START: 1,			// start delay for cpr display of HR '----' 
		CPR_DELAY_STOP: 2,			// stop delay for cpr display of HR '----' 
		CPR_ACTIVE: 2,				// active cpr display of HR '----'
		CPR_DELAY_IN: 3000,			// delay start in msec
		CPR_DELAY_OUT: 3000,		// delay stop in msec
		cprDelayTimer: 0,			// timer for cpr delay
		
		MANUAL_RESP_IDLE: 0,		// no manual respiration
		MANUAL_RESP_START: 1,		// start of manual respiration cycle
		MANUAL_RESP_DISPLAY_ETCO2: 2,	// display etco2
		MANUAL_RESP_DISPLAY_START_INDEX: 35,	// manual breath index to start display	
		MANUAL_RESP_DISPLAY_END_COUNT: 300,	// duration of display count
		RESP_ETCO2_BLANK_DELAY: 15000,	// delay for blanking ETCO2 on vitals
		
		// ekg strip parameters
		ekg: {
			width: 0,				// width of strip in pixels
			height: 125,			// height of strip in pixles
			id: 'vs-trace-1',		// id of canvas for strip
			interval: 0,			// variable to hold interval instantiation
			color: 'green',			// color of trace (either hex or html color)
			rhythm: new Array,		// array of digitized rhythms
			yOffset: 0,				// yOffset of trace
			yDisplayOffset: 5,		// display y offset
			xOffsetLeft: 10,		// left xOffset of trace
			xOffsetRight: 0,		// right xOffset of trace
			rhythmIndex: '',		// index of current rhythm being displayed
			rateIndex: 0,			// index of pattern for current heart rate
			length: 0,				// variable to hold length of pattern
			patternIndex: 0,		// index of currently displayed pixel in pattern
			lastY: 0,				// variable to save last displayed Y coordinate of pattern
			xPos: 0,				// current x position on strip
			drawInterval: 15,		// interval in milli-sec to display pixels
			noiseMax: 2,			// max amplitude of background noise total +/-
			stopFlag: false,			// stop flag 
			beepValue: 0,			// value to beep at
			beepFlag: false,
			pixelCount: 0,			// count in pixel ticks (drawInterval) of current period (incrementing)
			periodCount: 0,			// number of pixel counts in current period
			cprHRDisplayStatus: 0, 	// status of hr display {CPR_DELAY_NONE || CPR_DELAY_START || CPR_DELAY_STOP || CPR_ACTIVE}
			cprwaveformIndex: 0,    // index of the current cpr artifact waveform
			
			// vpc params
			vpcRateIndex: 0,		// index for VPC pattern for current heart rate
			vpcLength: 0,			// length of current vpc pattern
			vpcPatternIndex: 0,		// index of currently displayed pixel in vpc pattern
			vpcCount: 0,			// count of how many vpc's have been generated
			vpcSynchDelayCount: 0,		
									// count of delay added in to synch if VPC is generated
			vpcSynchDelay: 0,		// calculated delay
			vpcAdvanceDelay: 700,	// advance delay of vpc pulse * 1000. (i.e. 700 = 70% of heart rate to advance pulse or 1.4X of base HR).
		},
		
		// respiration strip parameters
		resp: {
			width: 0,				// width of strip in pixels
			height: 125,			// height of strip in pixles
			id: 'vs-trace-2',		// id of canvas for strip
			interval: 0,			// variable to hold interval instantiation
			color: 'white',			// color of trace (either hex or html color)
			rhythm: new Array,		// array of digitized rhythms
			yOffset: 0,				// yOffset of trace
			yDisplayOffset: 5,		// display y offset
			xOffsetLeft: 10,		// left xOffset of trace
			xOffsetRight: 0,		// right xOffset of trace
			rhythmIndex: 'low',		// index of current rhythm being displayed
			length: 10,				// variable to hold length of pattern
			patternIndex: 0,		// index of currently displayed pixel in pattern
			lastY: 0,				// variable to save last Y coordinate of pattern
			lastDisplayedY: 0,		// variable to save last displayed Y coordinate of pattern (with display offsets)
			lastETCO2: 0,			// last ETCO2 used for calculating ETCO2 Max...used for vitals display of ETCO2
			xPos: 0,				// current x position on strip
			drawInterval: 50,		// interval in milli-sec to display pixels
			activeCount: 0,			// Count of updates since sync
			halfCount: 100,			// Count to middle of period, for start of Exhale
			stopFlag: false,		// stop flag
			phaseTimer: 0,			// timer hold,
			ETCO2MaxDuration: 2000,	// max duration of ETCO2 high in msec
			inhalationDuration: 0,	// duration of inhalation in msec
			exhalationDuration: 0,	// duration of exhalation in msec
			patternComplete: false,	// flag for resp pattern complete
			inhalationPatternIndex: 0,	
									// pattern index for resp low to high.
			exhalationPatternIndex: 4,	
									// pattern index for resp high to low.
			pixelCount: 0,			// count in pixel ticks (drawInterval) of current period (incrementing)
			periodCount: 0,			// number of pixel counts in current period
			risePatternIndex: 4,		// index of pattern to use for rise and fall times based on breathing rate
			manualStatus: this.MANUAL_RESP_IDLE,
			manualBreathDisplayCount: 0,			// count of where we are in the display delay for ETCO2
			breathStart: false,		// flag to indicate if a new breating waveform is starting.
			blankTimer: 0,			// timer to blank vitals ETCO2
			rrBlankCount: 2,		// count of breath waveforms before displaying valid awRR
			currentetCO2value: 0,		// variable to hold ETCO2 value at the start of a breath waveform
			maxInhalationDuration: 0	// max duration ofr inhalation
		},
		
		cursorWidth: 10,			// width of cursor in pixels

		// assume document is rendered before calling init.
		init: function() {
			/************************** EKG **********************************/
			// set initial pattern
			chart.ekg.rhythmIndex = 'asystole';	// Flatline
			chart.ekg.rateIndex = 0;	// lowest heart rate
			
			// init canvas for ekg
			chart.initStrip('ekg');
			
			// init rhythm patterns
			chart.ekg.rhythm.asystole = new Array;
			chart.ekg.rhythm.sinus = new Array;
			chart.ekg.rhythm.vfib = new Array;
			chart.ekg.rhythm.afib = new Array;
			chart.ekg.rhythm.vtach1 = new Array;
			chart.ekg.rhythm.vtach2 = new Array;
			chart.ekg.rhythm.vtach3 = new Array;  // place holder since vtach 3 is half sine
			chart.ekg.rhythm.vpc1 = new Array;
			chart.ekg.rhythm.vpc2 = new Array;
			chart.ekg.rhythm.cpr = new Array;  // place holder since cpr is similar to vtach3
			
			// init cpr waveform, assume rate will be 120 bpm and waveform is simple 1/2 sinusoidal
			//var cprXIncr = (120 * chart.ekg.drawInterval * Math.PI) / 60000;
			var cprAmplitude = chart.ekg.height / 2;
			//var cprIndex = 0;
			//for(var x = 0; x <= Math.PI; x += cprXIncr) {
			//	chart.ekg.rhythm.cpr[cprIndex] = (Math.sin(x) * -cprAmplitude);
			//	cprIndex++;
			//}
			chart.ekg.rhythm.cpr[0] = [
				0,0.007352941,0.014705882,0.022058824,0.029411765,0.036764706,0.044117647,0.095588235,
				0.147058824,0.198529412,0.25,0.272058824,0.294117647,0.529411765,0.764705882,
				0.838235294,0.911764706,0.941176471,1,0.970588235,0.941176471,0.970588235,1,
				0.941176471,0.926470588,0.911764706,0.75,0.588235294,0.485294118,0.382352941,
				0.279411765,0.176470588,0.220588235,0.264705882,0.205882353,0.147058824,0.132352941,
				0.117647059,0.073529412,0.029411765,0.014705882,-0.014705882
			];
			chart.ekg.rhythm.cpr[1] = [
				0,0.006410256,0.012820513,0.128205128,0.173076923,0.217948718,0.237179487,0.256410256,
				0.461538462,0.666666667,0.730769231,0.794871795,0.871794872,0.923076923,0.871794872,
				0.846153846,0.871794872,0.923076923,0.948717949,1,0.884615385,0.846153846,0.871794872,
				0.820512821,0.230769231,0.179487179,0.128205128,0.115384615,0.102564103,0.064102564,
				0.025641026,0.012820513,-0.012820513,0,0.012820513,-0.012820513,0,-0.012820513,0.038461538,
				0,0.012820513,-0.012820513
			];
			chart.ekg.rhythm.cpr[2] = [
				-0.013513514,0.013513514,0,0.040540541,-0.013513514,0,-0.013513514,0.013513514,0,0.081081081,
			0.162162162,0.621621622,0.648648649,0.675675676,0.648648649,0.540540541,0.621621622,0.702702703,
			0.864864865,0.918918919,0.918918919,0.932432432,0.972972973,1,0.972972973,0.986486486,
			0.986486486,0.972972973,0.972972973,0.918918919,0.837837838,0.77027027,0.702702703,0.486486486,
			0.27027027,0.25,0.22972973,0.182432432,0.135135135,0.013513514,0.006756757,0
			];

			for(var j = 0; j < chart.ekg.rhythm.cpr.length; j++)
			{
				for(var i = 0; i < chart.ekg.rhythm.cpr[j].length; i++)
				{
				 	chart.ekg.rhythm.cpr[j][i] *= -cprAmplitude;
				}
			}
			
			// ekg
			chart.ekg.rhythm['defib'] = [
//				32, -64, 64, 64, 64, 64, 64, 64, 64, 64,
//				64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
//				32, 25, 16, 12, 10, 8, 6, 4, 2, 1, 0
-32,-32,32,32,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-32,-25,-16,-12,-10,-8,-6,-5,-4,-2,-1,0,0
			];
			
			// Atrial Fibrillation
			chart.ekg.rhythm['afib'][0] = [
				0, 1, 2, 3, 10, 17, 20, 52, 64, 40, 26, 10, 0, -10, -20, -15, -10, -1 // Up to 150
			];
			chart.ekg.rhythm['afib'][1] = [
				0, 2, 3, 10, 20, 52, 64, 26, 10, 0, -20, -15, -10, -1 // Up to 300
			];

			// Ventricular Tachycardia
			// BPM 0 - 80
			chart.ekg.rhythm['vtach1'][0] = [
				8, 8, 11, 21, 40, 56, 63, 67, 55, 37,
				17, -7, -13, -16, -21, -23, -24, -25, -26, -26,
				-24, -18, -11, -3, 5, 11, 14, 16, 15, 15,
				13, 12, 12, 13, 13, 17, 16, 15, 11, 9,
				9, 8, 8
			];
			// BPM 81 - 160
			chart.ekg.rhythm['vtach1'][1] = [
				8, 11, 21, 40, 56, 63, 67, 55, 37, 17,
				-7, -13, -16, -23, -26, -24, -18, -11, -3, 5, 
				11, 9, 8
			];
			// BPM 161 - 240
			chart.ekg.rhythm['vtach1'][2] = [
				8, 21, 40, 56, 63, 67, 37, 17,
				-7, -13, -26, -18, -11, 
				11, 8
			]; 
			// BPM 241 - 300
			chart.ekg.rhythm['vtach1'][3] = [
				8, 21, 40, 67, 37, 
				-7, -13, -26, -11, 
				11
			];

			// BPM 0 - 80
			chart.ekg.rhythm['vtach2'][0] = [
				0, 0, 0, 0, 0, 1, 2, 3, 3, 4,
				5, 3, -25, -52, -51, -49, -30, -19, -9, 11, 
				24, 25, 27, 28, 31, 35, 39, 42, 43, 40, 
				33, 25, 16, 9, 4, 0, 0, 0, 0, 0 
			];
			// BPM 81 - 160
			chart.ekg.rhythm['vtach2'][1] = [
				0, 1, 2, 3, 4, 5, 3, -25, -52, -30, 
				-19, -9, 11, 25, 35, 42, 33, 25, 16, 4
			];
			// BPM 161- 240
			chart.ekg.rhythm['vtach2'][2] = [
				1, 3, 5, -25, -52, -30, 
				-19, -9, 11, 25, 35, 42, 33, 25, 16
			]; 
			// BPM 241 - 300
			chart.ekg.rhythm['vtach2'][3] = [
				1, 5, 3, -25, -52, 
				-19, 11, 42, 33, 16
			];
			chart.ekg.rhythm['vtach3'][0] = [
				0, 1, 2, 3
			];
			
			// VPC
			chart.ekg.rhythm['vpc1'][0] = [
				8, 8, 11, 21, 40, 56, 63, 67, 55, 37,
				17, -7, -13, -16, -21, -23, -24, -25, -26, -26,
				-24, -18, -11, -3, 5, 11, 14, 16, 15, 15,
				13, 12, 12, 13, 13, 17, 16, 15, 11, 9,
				9, 8, 8
			];
			chart.ekg.rhythm['vpc1'][1] = [
				8, 11, 21, 40, 56, 63, 67, 55, 37, 17,
				-7, -13, -16, -23, -26, -24, -18, -11, -3, 5, 
				11, 9, 8
			];
			chart.ekg.rhythm['vpc1'][2] = [
				8, 21, 40, 56, 63, 67, 37, 17,
				-7, -13, -26, -18, -11, 
				11, 8
			];
			chart.ekg.rhythm['vpc2'][0] = [
				0, 0, 0, 0, 0, 1, 2, 3, 3, 4,
				5, 3, -25, -52, -51, -49, -30, -19, -9, 11, 
				24, 25, 27, 28, 31, 35, 39, 42, 43, 40, 
				33, 25, 16, 9, 4, 0, 0, 0, 0, 0 
			];
			chart.ekg.rhythm['vpc2'][1] = [
				0, 1, 2, 3, 4, 5, 3, -25, -52, -30, 
				-19, -9, 11, 25, 35, 42, 33, 25, 16, 4
			];
			chart.ekg.rhythm['vpc2'][2] = [
				1, 5, 3, -25, -52, -30, 
				-19, 11, 35, 42, 33, 16, 4
			];
			
			// asystole
			chart.ekg.rhythm['asystole'][0] = [
				0, 0, 0, 0, 0, 0, 0		// Flatline
			];
			
			// sinus
			chart.ekg.rhythm['sinus'][0] = [
				4, 3, 4, 6, 7, 7, 6, 4, 2, 1, 
				1, 1, 2, 2, 2, 3, 17, 52, 64, 26,
				-3, -5, -2, 0, 1, 2, 3, 4, 4, 5, 
				6, 7, 8, 10, 11, 13, 15, 16, 17, 17, 
				16, 14, 10, 7, 4, 2, 1, 0, 0, 1, 
				1 
			];
			chart.ekg.rhythm['sinus'][1] = [
				4, 3, 6, 7, 4, 2, 1, 2, 3, 17, 
				64, 26, -5, -2, 0, 2, 4, 5, 6,  10, 
				11, 15, 16, 17, 16, 10, 4, 1, 0, 1 
			];
			chart.ekg.rhythm['sinus'][2] = [
				4, 3, 7, 1, 3, 35, 64, -5, -2, 4, 
				6, 11, 15, 10, 4, 1 
			];
			chart.ekg.rhythm['sinus'][3] = [
				3, 7, 1, 35, 64, -5, 4, 17, 
				4, 1 
			];
			
			
			// vfib
			chart.ekg.rhythm['vfib'][0] = [
				2, 3, 17, 52, 64, 26, -3, -5, -2, 0, 1, 2, 3, 4, 4, 5, 6, 7, 
				8, 10, 11, 13, 15, 16, 17, 17, 16, 14, 10, 7, 4, 2, 1, 0, 0, 1, 1 // Up to 75
			];
			chart.ekg.rhythm['vfib'][1] = [
				2, 3, 17, 64, 26, -5, -2, 0, 2, 4, 5, 6,  10, 11, 15, 16, 17, 16, 10, 4, 1, 0, 1 // Up to 140
			];
			chart.ekg.rhythm['vfib'][2] = [
				3, 35, 64, -5, -2, 4, 6, 11, 15, 17, 10, 4, 1 // Up to 230
			];
			chart.ekg.rhythm['vfib'][3] = [
				3, 35, 64, -5, 4, 11, 17, 4, 1 // Up to 300
			];
			
			// setup pattern length
			chart.ekg.length = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex].length;
			
			// setup beep value
			chart.ekg.beepValue = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex].max() * -1;
			
			// start the pattern
			chart.ekg.interval = setInterval(chart.drawEkgPixel, chart.ekg.drawInterval);
						
			/************************** Respiration **********************************/
			// init respiration
			chart.initStrip('resp');
			
			// init rhythm patterns
			chart.resp.rhythm['high-to-low'] = new Array;
			chart.resp.rhythm['low-to-high'] = new Array;
			chart.resp.rhythm['high'] = new Array;
			chart.resp.rhythm['high-to-low'][0] = [	
				61.5493449,60.72807351,58.85943707,50.48641877,
				36.93296859,24.22481363,11.00608487,2.468408594,0.877075091,
				0.334116028,0.292495125,0
//				60,52,44,36,28,24,20,15,8
			];
			chart.resp.rhythm['high-to-low'][1] = [	
				61.5493449,60.72807351,58.85943707,50.48641877,
				36.93296859,24.22481363,11.00608487,2.468408594,0.877075091,
				0.334116028,0.292495125,0
//				60,52,44,36,28,24,20,15,8
			];
			chart.resp.rhythm['high-to-low'][2] = [	
				61.5493449,60.72807351,58.85943707,50.48641877,
				36.93296859,24.22481363,11.00608487,2.468408594,0.877075091,
				0.334116028,0.292495125,0			
			];
			chart.resp.rhythm['high-to-low'][3] = [	
				60.72807351, 50.48641877, 24.22481363, 2.468408594, 0.334116028
			];
			chart.resp.rhythm['high-to-low'][4] = [	
				30
			];
			chart.resp.rhythm['low'] = [	
				0
			];
			chart.resp.rhythm['rest'] = [	
				0,0,0
			];
			chart.resp.rhythm['low-to-high'][0] = [	
				0.110304316,0.204757313,0.444808642,1.440832926,3.047279566,
				5.909726892,12.58774301,24.91583508,37.12211491,44.30213386,
				46.80024109,48.49503321,49.89641132,50.9500952,51.83944314,
			];
			chart.resp.rhythm['low-to-high'][1] = [	
				0.204757313,0.444808642,1.440832926,3.047279566,
				5.909726892,12.58774301,24.91583508,37.12211491,44.30213386,
				46.80024109,48.49503321,49.89641132,50.9500952
			];
			chart.resp.rhythm['low-to-high'][2] = [	
				1.440832926,
				5.909726892,24.91583508,44.30213386,
				48.49503321,50.9500952
			];
			chart.resp.rhythm['low-to-high'][3] = [	
				12.58774301,// 37.12211491,
				46.80024109,// 49.89641132,
				50.9500952
			];
			chart.resp.rhythm['low-to-high'][4] = [	
				30
			];

			chart.resp.rhythm['high'][0] = [	
				52,62
			];
			chart.resp.rhythm['high'][1] = [	
				54,62
			];
			chart.resp.rhythm['high'][2] = [	
				54,62
			];
			chart.resp.rhythm['high'][3] = [	
				56,62
			];
			chart.resp.rhythm['high'][4] = [	
				56,62
			];
			
			chart.resp.manualBreathPattern = [	// approximate 300 msec waveform
				0.110304233,0.110304233,0.110304233,0.110304233,0.110304233,
				0.110304233,0.110304233,0.110304233,0.110304233,0.110304233,
				0.110304316,0.204757313,0.444808642,1.440832926,3.047279566,
				5.909726892,12.58774301,24.91583508,37.12211491,44.30213386,
				46.80024109,48.49503321,49.89641132,50.9500952,51.83944314,
				52.67463999,53.16772702,53.59644724,53.8281222,54.26381362,
				54.49252225,54.93136691,55.1664858,55.59261397,56.04765394,
				56.25591856,56.46471241,56.92293194,57.36708998,57.80026169,
				58.01810818,58.40573427,58.67420683,59.01728293,59.37400285,
				59.78660762,60.2297822,60.4456987,60.89843097,61.10002117,
				61.55049417,61.5493449,60.72807351,58.85943707,50.48641877,
				36.93296859,24.22481363,11.00608487,2.468408594,0.877075091,
				0.334116028,0.292495125,0
			];
			
			// get max value
			chart.resp.max = chart.resp.rhythm['high'].max();
			
			// max inhalation duration
			chart.resp.maxInhalationDuration = Math.floor( 1500 / chart.resp.drawInterval );
			
			// get max displayed value
			chart.getETC02MaxDisplay();

			// beep indicator
			if(chart.ekg.beepFlag == true){
				$('#ekg-sound').html('Turn EKG Sound OFF!').removeClass('play').addClass('pause')
			} else {
				$('#ekg-sound').html('Turn EKG Sound ON!').removeClass('pause').addClass('play')			
			}
			
			// setup pattern length
			chart.resp.length = chart.resp.rhythm[chart.resp.rhythmIndex].length;

			// start the pattern
			chart.resp.interval = setInterval(chart.drawRespPixel, chart.resp.drawInterval, "resp");

//console.log("chart.resp.interval: " + chart.resp.interval);
//console.log("chart.resp.drawInterval: " + chart.resp.drawInterval);

			// init respiration
			chart.updateRespRate();
			controls.awRR.setSynch();
		},
		
		// Passed the cardiac data from simmgr status
		updateCardiac: function( cardiac) {
			if(controls.cpr.inProgress == true) {
				chart.ekg.rateIndex = 0;				
			} else if ( cardiac.rate <= 0  ) {
//				chart.ekg.rhythmIndex = 'asystole';	// Flatline
			} else if(chart.ekg.rhythmIndex == 'sinus') {
				if( cardiac.rate <= 65 ) {
					chart.ekg.rateIndex = 0;
				}
				else if( cardiac.rate <= 100 ) {
					chart.ekg.rateIndex = 1;
				}
				else if( cardiac.rate <= 220 ) {
					chart.ekg.rateIndex = 2;
				}
				else {
					chart.ekg.rateIndex = 3;
				}

				if(controls.heartRhythm.vpc != 'none') {
					if( cardiac.rate <= 65 ) {
						chart.ekg.vpcRateIndex = 0;
					}
					else if( cardiac.rate <= 115 ) {
						chart.ekg.vpcRateIndex = 1;
					}
					else {
						chart.ekg.vpcRateIndex = 2;
					}

					// calculate length of VPC
					chart.ekg.vpcLength = chart.ekg.rhythm[controls.heartRhythm.vpc][chart.ekg.vpcRateIndex].length;
					
					// calculate vpc synch delay 1.4X of heart rate (or 70%) minus width of sinus pulse
					chart.ekg.vpcSynchDelay = Math.floor(((60 / cardiac.rate) * chart.ekg.vpcAdvanceDelay) / chart.ekg.drawInterval);

					// set these 2 params to kick off a series of VPC's.
					chart.ekg.vpcCount = -1;
					chart.ekg.vpcSynchDelayCount = 0;
					
					chart.ekg.vpcPatternIndex = 0;
				}
			} else if(chart.ekg.rhythmIndex == 'vfib') {
				if( cardiac.rate <= 75 ) {
					chart.ekg.rateIndex = 0;
				}
				else if( cardiac.rate <= 140 ) {
					chart.ekg.rateIndex = 1;
				}
				else if( cardiac.rate <= 230 ) {
					chart.ekg.rateIndex = 2;
				}
				else {
					chart.ekg.rateIndex = 3;
				}			
			} else if(chart.ekg.rhythmIndex == 'afib') {
				if( cardiac.rate <= 80 ) {
					chart.ekg.rateIndex = 0;
				} else {
					chart.ekg.rateIndex = 1;
				}			
			} else if(chart.ekg.rhythmIndex == 'vtach1') {
				if( cardiac.rate <= 80 ) {
					chart.ekg.rateIndex = 0;
				} else if( cardiac.rate <= 160 ) {
					chart.ekg.rateIndex = 1;
				} else if( cardiac.rate <= 240 ) {
					chart.ekg.rateIndex = 2;				
				} else {
					chart.ekg.rateIndex = 3;				
				}		
			} else if(chart.ekg.rhythmIndex == 'vtach2') {
				if( cardiac.rate <= 80 ) {
					chart.ekg.rateIndex = 0;
				} else if( cardiac.rate <= 160 ) {
					chart.ekg.rateIndex = 1;
				} else if( cardiac.rate <= 240 ) {
					chart.ekg.rateIndex = 2;				
				} else {
					chart.ekg.rateIndex = 3;				
				}		
			}  else if(chart.ekg.rhythmIndex == 'vtach3') {
				chart.ekg.rateIndex = 0;
			}
			
			if ( typeof ( chart.ekg.rhythm[chart.ekg.rhythmIndex] ) === 'undefined' || typeof chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex] == 'undefined')
			{
//				console.log("No EKG Rhythm "+chart.ekg.rhythmIndex );
				chart.ekg.rhythmIndex = 'asystole';	// Flatline
				chart.ekg.rateIndex = 0;
			}
			
			if(chart.ekg.rhythmIndex != 'dfib') {
				chart.ekg.length = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex].length;
			}
			
			if(chart.ekg.patternIndex >= chart.ekg.length) {
				// Advance to the next CPR artifact waveform if cpr is happening
				//chart.cpr.waveformIndex++;
				chart.ekg.patternIndex = 0;
			}

			chart.heartRate = cardiac.rate;
			controls.heartRate.value = cardiac.rate;
			if ( typeof simsound !== 'undefined' )
			{
				simsound.lookupHeartSound();
			}
//console.log(cardiac );
//console.log(cardiac.rate);
//console.log(chart.ekg.rhythmIndex);
		},
		initStrip: function(stripType) {
			chart[stripType].canvas = document.getElementById(chart[stripType].id);
			chart[stripType].ctx = chart[stripType].canvas.getContext("2d");
			chart[stripType].xPos = chart[stripType].xOffsetLeft;
			chart[stripType].yOffset = chart[stripType].lastY = Math.floor(chart[stripType].height / 2);
			
			// set width of strip dynamically
			chart[stripType].width = $('#' + chart[stripType].id).width() - chart[stripType].xOffsetLeft - chart[stripType].xOffsetRight;
			return;
		},
		
		// routine to initialize vtach 3 R on T values based on heart rate sinusoidal
		// updated amplitude setting per Dan F - 2024-03-04
		initVtach3: function() {
			chart.ekg.rhythm.vtach3[0] = new Array;	
			xIncr = (controls.heartRate.value * chart.ekg.drawInterval * Math.PI) / 60000;
//			var amplitude = chart.ekg.height / 2;
			var amplitude = chart.ekg.height / 2.5;
			var offset = chart.ekg.height / 2;
			var index = 0;
			for(var x = 0; x <= Math.PI; x += xIncr) {
//				chart.ekg.rhythm.vtach3[0][index] = (Math.sin(x) * -amplitude) + 10;
				chart.ekg.rhythm.vtach3[0][index] = (Math.sin(x*2) * -amplitude) + 10;
				index++;
			}
		},
		
		drawEkgPixel: function() {
			if(scenario.currentScenarioState == scenario.scenarioState.PAUSED && profile.isVitalsMonitor) {
				return;
			}
			
			var y;

			// Create the 'cursor' by clearing out a 10px wide section in front of the pixel
			chart.drawCursor('ekg');
	
//console.log(chart.ekg.patternIndex)

			if ( ( profile.isVitalsMonitor == false ) || ( controls.ekg.leadsConnected == true ) || simmgr.isTeleSim() == true ) {
				// see if we need to draw waveform or if we are in background
				if(chart.ekg.stopFlag == true) {
					y = 0;
					controls.heartRate.audio.pause();
				} else if(controls.cpr.inProgress == true) {
					y = chart.ekg.rhythm.cpr[chart.ekg.cprwaveformIndex][chart.ekg.patternIndex];
					controls.heartRate.value = 120;
					chart.ekg.length = chart.ekg.rhythm.cpr[chart.ekg.cprwaveformIndex].length;
					
					// increment pointers
					chart.ekg.patternIndex++;
//				} else if( controls.cpr.running == 1) {
					// if we get here then we are in the 2 second runout of the cpr waveform.
					// generate noise value.
//					y = Math.floor((Math.random() * chart.ekg.noiseMax));
//					if(y > (chart.ekg.noiseMax / 2)) {
//						y -= (chart.ekg.noiseMax / 2);
//					}
//					chart.ekg.patternIndex = 0;					
				} else if(chart.ekg.rhythmIndex == 'defib') {
					y = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.patternIndex];
					
					// increment pointers
					chart.ekg.patternIndex++;				
				} else if(chart.ekg.rhythmIndex == 'sinus' || chart.ekg.rhythmIndex == 'vtach1' || chart.ekg.rhythmIndex == 'vtach2') {
					// check if we are doing a vpc.  VPC synch will only get set when the vpc needs to be generated
					if(chart.status.cardiac.vpcSynch == true && chart.ekg.patternIndex == 0 && chart.status.cardiac.synch == false) {
						// are there vpc's to generate?
						if(chart.ekg.vpcCount > 0) {
							// see if we need to generate the delay
							if(chart.ekg.vpcSynchDelayCount > 0) {
								// generate noise
								y = chart.getEKGNoisePixel();
// y = -30;
								chart.ekg.vpcSynchDelayCount--;
							} else {
								// generate the pattern
								y = chart.ekg.rhythm[controls.heartRhythm.vpc][chart.ekg.vpcRateIndex][chart.ekg.vpcPatternIndex] * -1;
								chart.ekg.vpcPatternIndex++;
								
								// are we done with the pattern?
								if(chart.ekg.vpcPatternIndex >= chart.ekg.vpcLength) {
									chart.ekg.vpcPatternIndex = 0;
									chart.ekg.vpcCount--;
									
									// reset synch delay minus width of vpc pattern
									chart.ekg.vpcSynchDelayCount = chart.ekg.vpcSynchDelay - chart.ekg.length;
								}
							}
						} else {
							chart.status.cardiac.vpcSynch = false;						
							y = chart.getEKGNoisePixel();
						}

					} else if((chart.status.cardiac.synch == false && chart.ekg.patternIndex == 0) || controls.heartRate.value == 0) {
						// see if we are doing a vpc...here is where we would generate noise or the pre-vpc delay
						y = chart.getEKGNoisePixel();						
					} else if(chart.status.cardiac.synch == true || chart.ekg.patternIndex > 0) {
						y = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex][chart.ekg.patternIndex] * -1;
						if ( typeof simsound !== 'undefined' && chart.status.cardiac.synch == true )
						{
							simsound.playHeartSound();
						}
						
						// beep?
						if(y == chart.ekg.beepValue && chart.ekg.beepFlag == true && chart.ekg.stopFlag == false) {
							// controls.heartRate.audio.load();  // Don't do this!!
							controls.heartRate.audio.play();
						}
						
						// increment pointers
						chart.ekg.patternIndex++;
					}
										
				} else if(chart.ekg.rhythmIndex == 'afib') {
					if(chart.status.cardiac.synch == false && chart.ekg.patternIndex == 0) {
						// generate slow noise between range
						y = chart.vfib.base + chart.getafibBase();
						
					} else if(chart.status.cardiac.synch == true || chart.ekg.patternIndex > 0) {
						y = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex][chart.ekg.patternIndex] * -1;
						
						if ( typeof simsound !== 'undefined' )
						{
							//simsound.playHeartSound();
						}
						// beep?
						if(y == chart.ekg.beepValue && chart.ekg.beepFlag == true && chart.ekg.stopFlag == false) {
							// controls.heartRate.audio.load();  // Don't do this!!
							controls.heartRate.audio.play();
						}
						
						// increment pointers
						chart.ekg.patternIndex++;
					}
				} else if(chart.ekg.rhythmIndex == 'asystole') {
					y = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex][chart.ekg.patternIndex] * -1;
					
					// generate random noise between range
					y += Math.floor((Math.random() * chart.ekg.noiseMax));
					if(y > (chart.ekg.noiseMax / 2)) {
						y -= (chart.ekg.noiseMax / 2);
					}
					
					// increment pointers
					chart.ekg.patternIndex++;
				} else if(chart.ekg.rhythmIndex == 'vtach3') {
					y = chart.ekg.rhythm[chart.ekg.rhythmIndex][chart.ekg.rateIndex][chart.ekg.patternIndex];
					
					// increment pointers
					chart.ekg.patternIndex++;
				} else if(chart.ekg.rhythmIndex == 'vfib') {
					chart.vfib.base = chart.getBaseline();
					y = chart.vfib.base + chart.getfib() - 6;
				}
				
				// clear out sync flag
				if(chart.status.cardiac.synch == true) {
					if( (chart.ekg.periodCount > 0) && (parseInt(controls.heartRate.value) > parseInt(simmgr.cardiacResponse.rate)) ) {
						chart.updateCardiacRate();
					} else {
						chart.status.cardiac.synch = false;
					}
					
					// reset tick count
					chart.ekg.pixelCount = 0;
				} else {
					chart.ekg.pixelCount++;
					if( (chart.ekg.periodCount > 0) && (chart.ekg.pixelCount >= (chart.ekg.periodCount)) ) {
						chart.updateCardiacRate();
					}
				}
				
				// are we beyond pattern?
				if(chart.ekg.patternIndex >= chart.ekg.length) {
					if(controls.cpr.inProgress == true) {
						//if(chart.ekg.cprwaveformIndex==2){
						//	chart.ekg.cprwaveformIndex=0;
						//} else {
						chart.ekg.cprwaveformIndex = Math.floor((Math.random() * 3));
						//}
					}
					
					if(controls.defib.shock == 1) {
						// keep patternindex on 0
//						chart.ekg.patternIndex--;
						// generate random noise between range
						y = Math.floor((Math.random() * chart.ekg.noiseMax));
						if(y > (chart.ekg.noiseMax / 2)) {
							y -= (chart.ekg.noiseMax / 2);
						}
					} else {
						chart.ekg.patternIndex = 0;
					}
				}
			} else {
				y = 0;
			}
			
			y += chart.ekg.yOffset + chart.ekg.yDisplayOffset;
			
			// create stroke
			chart.ekg.ctx.lineWidth = 2;
			if ( ( profile.isVitalsMonitor == false ) || ( controls.ekg.leadsConnected == true ) )
			{
				chart.ekg.ctx.strokeStyle = chart.ekg.color;
			}
			else
			{
				chart.ekg.ctx.strokeStyle = 'black';
			}
			chart.ekg.ctx.beginPath();
			chart.ekg.ctx.moveTo(chart.ekg.xPos, chart.ekg.lastY);
			
			// increment xpos
			chart.ekg.xPos++;
			
			chart.ekg.ctx.lineTo(chart.ekg.xPos, y);
			chart.ekg.ctx.stroke();
						
			// save last values for next segment
			chart.ekg.lastY = y;
			
			// see if we are beyond end of chart
			if((chart.ekg.xPos + chart.ekg.xOffsetRight) > chart.ekg.width) {
				chart.ekg.xPos = chart.ekg.xOffsetLeft;
				chart.ekg.ctx.fillRect(0, 0, chart.ekg.xOffsetLeft, chart.ekg.height);
			}
		},
		
		drawCursor: function(stripType) {
			// Create the 'cursor' by clearing out section in front of the pixel
			chart[stripType].ctx.fillStyle="black";
			chart[stripType].ctx.clearRect(chart[stripType].xPos, 0, chart.cursorWidth, chart[stripType].height );
		},
		
		
		drawRespPixel: function() {
			if(scenario.currentScenarioState == scenario.scenarioState.PAUSED && profile.isVitalsMonitor) {
				return;
			}

			var y;

			// Create the 'cursor' by clearing out a 10px wide section in front of the pixel
			chart.drawCursor('resp');
			
			
			if(controls.manualRespiration.inProgress == true) {
				if(controls.manualRespiration.manualBreathIndex >= chart.resp.manualBreathPattern.length) {
					controls.manualRespiration.inProgress = false;
					y = 0;
				} else {
					//scale the y value to the current ETCO2
					chart.resp.currentetCO2value = controls.etCO2.value
					y = chart.resp.manualBreathPattern[controls.manualRespiration.manualBreathIndex] * -1 * chart.resp.currentetCO2value / controls.etCO2.maxValue;
//console.log("manual breath: " + y);
					// advance to the next point in the waveform
					controls.manualRespiration.manualBreathIndex++;
				}
				
				// check for vitals display of new ETCO2, manual breath index = 35 is transition to low.
				if( profile.isVitalsMonitor && controls.manualRespiration.manualBreathIndex == (chart.resp.manualBreathPattern.length - 1) ) {
					controls.etCO2.displayValue();					
				}
				
//			} else if (controls.heartRhythm.pea == true) {
//				y = 0;
			} else if(simmgr.respResponse.rate == 0) {
				chart.resp.rhythmIndex = 'low';	// start pattern with pattern low...start of inhalation
				y = 0;
			} else if ( ( profile.isVitalsMonitor == false ) || ( controls.CO2.leadsConnected == true ) ) {
				if(chart.status.resp.synch == true ) {	// Restart Cycle
					if ( typeof simsound !== 'undefined' )
					{
						simsound.playLungSound();
					}
// console.log("Periodcount: " + chart.resp.periodCount);
// console.log("rate: " + simmgr.respResponse.rate);
					chart.updateRespRate();
					//store the current etCO2 value for this breath in case it is changed mid-breath
					chart.resp.currentetCO2value = controls.etCO2.value
					
					// clear out synch bit
					chart.status.resp.synch = false;
					
					// flag start of synch
					chart.resp.breathStart = true;
					
					// pixel count is used to track total number of pixels rendered in waveform
					chart.resp.pixelCount = 0;
					
					// index of current pattern pixel being displayed
					chart.resp.patternIndex = 0;
					
					// pattern being displayed
					chart.resp.rhythmIndex = 'low';	// start pattern with pattern low...start of inhalation
					
					// length of current pattern segment being displayed
					chart.resp.length = chart.resp.inhalationDuration-1;
//console.log("chart.resp.inhalationDuration: " + chart.resp.inhalationDuration);
//console.log("chart.resp.exhalationDuration: " + chart.resp.exhalationDuration);
					
					// max value
					if(chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.patternIndex] > chart.displayETCO2.max) {
						y = chart.displayETCO2.max * -1;
					} else {
						y = chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.patternIndex] * -1;
					}
				}
				else {
					if(chart.resp.rhythmIndex == 'low-to-high') {
						y = chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.risePatternIndex][chart.resp.patternIndex] * -1 * chart.resp.rhythm['high'][chart.resp.risePatternIndex][0]/52;
					} else if(chart.resp.rhythmIndex == 'high-to-low'){
						y = chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.risePatternIndex][chart.resp.patternIndex] * -1;
					} else if(chart.resp.rhythmIndex == 'low' || chart.resp.rhythmIndex == 'rest'){
						y = chart.resp.rhythm[chart.resp.rhythmIndex][0] * -1;
					} else if (chart.resp.rhythmIndex == 'high'){
						y = -1* (chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.risePatternIndex][0] + ((chart.resp.patternIndex/(chart.resp.length-1)) * (chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.risePatternIndex][1]-chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.risePatternIndex][0])))
//console.log("y: " + y);
//console.log("chart.displayETCO2.max * -1: " + chart.displayETCO2.max * -1);

					}

					//scale the y value to the current ETCO2
					y = y * chart.resp.currentetCO2value / controls.etCO2.maxValue
//console.log("y: " + y);
					
					// check that y is not over max value
					// if(y < (chart.displayETCO2.max * -1)) {
					//	y = chart.displayETCO2.max * -1;
					// }
					
					// increment pixel count and index into pattern
					chart.resp.pixelCount++;
					chart.resp.patternIndex++;

					if(chart.resp.patternIndex >= chart.resp.length) {						
						chart.resp.patternIndex = 0;
						switch ( chart.resp.rhythmIndex ) {
							// inhalation
							case 'low': // Hold In (pattern low)
								// breathing rate is greater than zero than advance to next waveform, else stay in low and reset pattern
								if(simmgr.respResponse.rate > 0) {
									chart.resp.rhythmIndex = 'low-to-high';
									chart.resp.length = chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.risePatternIndex].length-1;
								}
								chart.resp.patternIndex = 0;								
								break;
							
							case 'low-to-high': // Exhalation (low to high)
								chart.resp.rhythmIndex = 'high';
								chart.resp.length = chart.resp.exhalationDuration - chart.resp.length - chart.resp.rhythm['high-to-low'][chart.resp.risePatternIndex].length-1;
//console.log("length of high: " + chart.resp.length);
								chart.resp.patternIndex = 0;								
								break;

							case 'high': // Hold Out (hold high)
								chart.resp.rhythmIndex = 'high-to-low';
								chart.resp.length = chart.resp.rhythm[chart.resp.rhythmIndex][chart.resp.risePatternIndex].length-1;
								chart.resp.patternIndex = 0;
								controls.etCO2.changeInProgressStatus = ETCO2_NEW_WAVEFORM_COMPLETED;
									if ( profile.isVitalsMonitor ) {
										controls.etCO2.displayValue();
									}								
								break;

							case 'high-to-low':	// Depletion of CO2 (high to low)
//console.log("got to end of high to low");
								chart.resp.rhythmIndex = 'rest';
								chart.resp.length = chart.resp.rhythm[chart.resp.rhythmIndex].length-1;
								chart.resp.patternIndex = 0;
//								controls.etCO2.changeInProgressStatus = ETCO2_NEW_WAVEFORM_COMPLETED;
//									if ( profile.isVitalsMonitor ) {
//										controls.etCO2.displayValue();
//									}
								break;

							case 'rest':	// rest between breaths...stay in cycle until synch pulse
								chart.resp.rhythmIndex = 'rest';
								chart.resp.length = chart.resp.rhythm[chart.resp.rhythmIndex].length-1;
								chart.resp.patternIndex = 0;
								if(controls.etCO2.changeInProgressStatus == ETCO2_NEW_WAVEFORM_COMPLETED) {
									controls.etCO2.changeInProgressStatus = ETCO2_OK;
									
//									if ( profile.isVitalsMonitor ) {
//										controls.etCO2.displayValue();
//									}								
								}							
								break;

						}
					}
				}
			} else if ( ( profile.isVitalsMonitor == true ) || ( controls.CO2.leadsConnected == false ) ) {
				if(chart.status.resp.synch == true ) {	// Restart Cycle
					if ( typeof simsound !== 'undefined' )
					{
						simsound.playLungSound();
					}
					chart.status.resp.synch = false;
				}
			}
			else {
				y = 0;
			}
			
			// save last y before offsets are added in
			chart.resp.lastY = y;
			
			y += chart.resp.yOffset + chart.resp.yDisplayOffset;
			// create stroke
			chart.resp.ctx.lineWidth = 2;
			if ( ( profile.isVitalsMonitor == false ) || ( controls.CO2.leadsConnected == true ) )
			{
				chart.resp.ctx.strokeStyle = chart.resp.color;
			}
			else
			{
				chart.resp.ctx.strokeStyle = 'black';
			}
			chart.resp.ctx.beginPath();
			chart.resp.ctx.moveTo(chart.resp.xPos, chart.resp.lastDisplayedY);
			
			// increment xpos
			chart.resp.xPos++;
			
			chart.resp.ctx.lineTo(chart.resp.xPos, y);
			chart.resp.ctx.stroke();
						
			// save last values for next segment
			chart.resp.lastDisplayedY = y;
			
			// see if we are beyond end of chart
			if((chart.resp.xPos + chart.resp.xOffsetRight) > chart.resp.width) {
				chart.resp.xPos = chart.resp.xOffsetLeft;
				chart.resp.ctx.fillRect(0, 0, chart.resp.xOffsetLeft, chart.resp.height);
			}
			
			// are we at the start of a new pattern?
			// clear out bit and recalculate amplitude of ETCO2 waveform.
			if( chart.resp.breathStart ) {
				chart.resp.breathStart = false;
				chart.getETC02MaxDisplay();
//console.log("New ETCO2: " + controls.etCO2.value);
//console.log("New ETCO2 max: " + chart.displayETCO2.max);
			}
		},
		
		getETC02MaxDisplay: function() {
			// calculate maximum displayed for ETCO2
			chart.displayETCO2.max = Math.floor(chart.resp.max * (controls.etCO2.value / controls.etCO2.maxValue));
			
			// save value of ETCO2 used for last max calculation (for vitals use...)
			chart.resp.lastETCO2 = controls.etCO2.value;
		},

		getBaseline: function() {
			x1 = chart.baselineP1 / chart.baselineUnit;
			y1 = Math.sin(x1);
			
			x2 = chart.baselineP2 / chart.baselineUnit;
			y2 = Math.sin(x2);
			chart.baselineP1 += 0.1;
			chart.baselineP2 += 0.25;
			return ( chart.baselineUnit*(y1 + y2) );
		},
		
		getfib: function() {
			if ( chart.fibUnit1 == 0 ) {
				return ( 0 );
			}
			else {	
				if ( ( chart.fibP3 % 4 ) == 1 )
				{
					chart.fibMultiply = chart.fibP3List[chart.fibP3ListIndex];
					chart.fibP3ListIndex++;
					if ( chart.fibP3ListIndex >= chart.fibP3List.length ) {
						chart.fibP3ListIndex = 0;
					}
//console.log("fib Multiply: " + fibMultiply);
				}
			
				y1 = Math.sin(chart.fibP1 / chart.fibUnit1 );
				y2 = Math.sin(chart.fibP2 / chart.fibUnit2 );
				
				chart.fibP1 += chart.fibP1Constant;
				chart.fibP2 += chart.fibP2Constant;
				chart.fibP3 += 1;
				
				return ( (chart.fibMultiply/chart.fibDivide)*(y1 + y2) );
			}
		},
		
		getafibBase2: function() {
			if ( chart.fibUnit1 == 0 ) {
				return ( 0 );
			}
			else {	
				if ( ( chart.fibP3 % 2 ) == 1 )
				{
					chart.fibMultiply = chart.fibP3List[chart.fibP3ListIndex];
					chart.fibP3ListIndex++;
					if ( chart.fibP3ListIndex >= chart.fibP3List.length ) {
						chart.fibP3ListIndex = 0;
					}
//console.log("fib Multiply: " + fibMultiply);
				}
			
				y1 = Math.sin(chart.fibP1 / chart.fibUnit1 );
				y2 = Math.sin(chart.fibP2 / chart.fibUnit2 );
				
				chart.fibP1 += chart.fibP1Constant;
				chart.fibP2 += chart.fibP2Constant;
				chart.fibP3 += 1;
				
				return ( (chart.fibMultiply/8)*(y1 + y2) );
//				return ( (chart.fibMultiply/chart.fibDivide)*(y1 + y2) );
			}
		},
		
		getafibBase: function() {
			if ( chart.fibUnit1 == 0 ) {
				return ( 0 );
			}
			else {	
				if ( ( chart.fibP3 % 2 ) == 1 )
				{
					chart.fibMultiply = chart.fibP3List[chart.fibP3ListIndex];
					chart.fibP3ListIndex++;
					if ( chart.fibP3ListIndex >= chart.fibP3List.length ) {
						chart.fibP3ListIndex = 0;
					}
//console.log("fib Multiply: " + fibMultiply);
				}
			
				y1 = Math.sin(chart.fibP1 / chart.fibUnit1 );
				y2 = Math.sin(chart.fibP2 / chart.fibUnit2 );
				
				chart.fibP1 += 6;
				chart.fibP2 += 4;
//				chart.fibP1 += chart.fibP1Constant;
//				chart.fibP2 += chart.fibP2Constant;
				chart.fibP3 += 1;
				
				return ( (chart.fibMultiply/4)*(y1 + y2) );
			}
		},

		updateCardiacRate: function() {
			controls.heartRate.setHeartRateValue(simmgr.cardiacResponse.rate );
			if(simmgr.cardiacResponse.rhythm == 'vtach3') {
				// pre calculate R on T based on heart rate
				chart.initVtach3();
			}
			chart.updateCardiac(simmgr.cardiacResponse);
			chart.status.cardiac.synch == false;
			chart.ekg.patternIndex = 0;
			clearTimeout(controls.heartRate.beatTimeout);
			controls.heartRate.setSynch();
		},
		
		updateRespRate: function() {
			// Calculate the inhalation time
			if ( simmgr.respResponse.rate > 0 )
			{
				// calculate total length of breathing pattern
				chart.resp.periodCount = Math.round(((60 / simmgr.respResponse.rate) * 1000) / chart.resp.drawInterval);
//console.log("periodCount: " + chart.resp.periodCount);
//console.log("simmgr.respResponse.rate: " + simmgr.respResponse.rate);

				// calculate exhalation duration
				// Maximum length of expiration is 3 seconds, so truncate at cycle length > 4.5sec
				if( (60/simmgr.respResponse.rate) > 4.5 )
					{
						chart.resp.exhalationDuration = Math.floor((3 * 1000) / chart.resp.drawInterval);
					} else {
						//chart.resp.exhalationDuration = Math.floor(simmgr.respResponse.exhalation_duration / chart.resp.drawInterval);
						chart.resp.exhalationDuration = Math.floor(chart.resp.periodCount * 2 / 3);
					}
				
				// calculate inhalation duration
				chart.resp.inhalationDuration = chart.resp.periodCount - chart.resp.exhalationDuration;

				

//console.log("exhalation_duration: " + chart.resp.exhalationDuration)
//console.log("chart.resp.inhalationDuration: " + chart.resp.inhalationDuration)

				
				// rise / fall pattern used fo rising and falling edge patterns
				if(simmgr.respResponse.rate < 10) {
					chart.resp.risePatternIndex = 0;
				} else if(simmgr.respResponse.rate <= 20) {
					chart.resp.risePatternIndex = 1;								
				} else if(simmgr.respResponse.rate <= 30) {
					chart.resp.risePatternIndex = 2;								
				} else if(simmgr.respResponse.rate <= 50) {
					chart.resp.risePatternIndex = 3;
					//chart.resp.exhalationDuration = Math.floor(chart.resp.periodCount / 2);	
					//chart.resp.inhalationDuration = chart.resp.periodCount - chart.resp.exhalationDuration;							
				} else {
					chart.resp.risePatternIndex = 4;
					chart.resp.exhalationDuration = Math.floor(chart.resp.periodCount / 2);	
					chart.resp.inhalationDuration = chart.resp.periodCount - chart.resp.exhalationDuration;							

				}
				
				// check for maximum inhalation duration
				if( chart.resp.inhalationDuration > chart.resp.maxInhalationDuration ) {
					chart.resp.inhalationDuration = chart.resp.maxInhalationDuration;
				}
			}
			else
			{
				// Default to avoid divide by zero
				controls.inhalation_duration.value = 400; 
				
				// assign any value to generate low value
				chart.resp.periodCount = 50;
			}
		},
		
		getEKGNoisePixel: function() {
			// generate random noise between range
			var y = Math.floor((Math.random() * chart.ekg.noiseMax));
			if(y > (chart.ekg.noiseMax / 2)) {
				y -= (chart.ekg.noiseMax / 2);
			}
			return y;
		}
	}
