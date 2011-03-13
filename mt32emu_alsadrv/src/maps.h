/* Copyright (C) 2003 Tristan
 * Copyright (C) 2004, 2005 Tristan, Jerome Fisher
 * Copyright (C) 2008, 2011 Tristan, Jerome Fisher, Jörg Walter
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

const int midi_gm2mt[] = {
        -1,
	
/* PIANO */
	6,   /* 1  Acoustic Grand  */
	6,   /* 2  Bright Acoustic */
	61,  /* 3  Electric Grand  */
	8,   /* 4  Honky-Tonk      */
	4,   /* 5  Electric 1      */
	5,   /* 6  Electric 2      */
	17,  /* 7  Harpsichord     */
	20,  /* 8  Clavi           */

/* CHROM PERCUSSION */
        23,  /* 9  Celesta         */
       102,  /* 10 Glockenspiel    */
       101,  /* 11 Music Box	   */
        98,  /* 12 Vibraphone	   */
       105,  /* 13 Marimba	   */
       104,  /* 14 Xylophone	   */
       103,  /* 15 Tubular Bells   */
       120,  /* 16 Dulcimer	   */
       
/* ORGAN */
        13,  /* 17 Drawbar Organ    */   
        15,  /* 18 Percussive Organ */  
        10,  /* 19 Rock Organ       */  	
        13,  /* 20 Church Organ	    */       
        15,  /* 21 Reed Organ       */  
        16,  /* 22 Accoridan        */  
        88,  /* 23 Harmonica        */  
        46,  /* 24 Tango Accordian  */  
       
/* GUITAR */
   	60,  /* 25 Acoustic Guitar(nylon) */
	61,  /* 26 Acoustic Guitar(steel) */
	72,  /* 27 Electric Guitar(jazz)  */
        60,  /* 28 Electric Guitar(clean) */
        66,  /* 29 Electric Guitar(muted) */
        63,  /* 30 Overdriven Guitar	  */
        49,  /* 31 Distortion Guitar	  */
        64,  /* 32 Guitar Harmonics	  */
	
/* BASS */
   	65,  /* 33 Acoustic Bass          */  
	65,  /* 34 Electric Bass(finger)  */   
	66,  /* 35 Electric Bass(pick)    */ 
	71,  /* 36 Fretless Bass          */   
	69,  /* 37 Slap Bass 1            */   
	70,  /* 38 Slap Bass 2            */  
	29,  /* 39 Synth Bass 1           */   
	31,  /* 40 Synth Bass 2           */

/* STRINGS */
   	53,   /* 41 Violin		  */
        54,   /* 42 Viola		  */
        55,   /* 43 Cello		  */
        57,   /* 44 Contrabass		  */
        50,   /* 45 Tremolo Strings	  */
        52,   /* 46 Pizzicato Strings	  */
        58,   /* 47 Harp        	  */
        113,  /* 48 Timpani    		  */
	
/* ENSEMBLE */
	53,   /* 49 String Ensemble 1     */         
	51,   /* 50 String Ensemble 2     */    
	50,   /* 51 SynthStrings 1        */    
	51,   /* 52 SynthStrings 2        */    
	35,   /* 53 Choir Aahs            */    
	34,   /* 54 Voice Oohs            */    
	34,   /* 55 Synth Voice           */    
	123,  /* 56 Orchestra Hit         */    

/* BRASS */
	89,   /* 57 Trumpet		  */
	91,   /* 58 Trombone		  */
	95,   /* 59 Tuba		  */
	90,   /* 60 Muted Trumpet	  */
 	93,   /* 61 French Horn		  */
	97,   /* 62 Brass Section	  */
	27,   /* 63 SynthBrass 1	  */
	28,   /* 64 SynthBrass 2	  */

/* REED */                
   	79,  /* 65 Soprano Sax            */   
	80,  /* 66 Alto Sax               */   
	81,  /* 67 Tenor Sax              */   
	82,  /* 68 Baritone Sax           */   
	85,  /* 69 Oboe                   */   
	86,  /* 70 English Horn           */   
	87,  /* 71 Bassoon                */   
	83,  /* 72 Clarinet               */   

/* PIPE */
   	76,  /* 73 Piccolo		  */
	74,  /* 74 Flute		  */
	77,  /* 75 Recorder		  */
	78,  /* 76 Pan Flute		  */
	111, /* 77 Blown Bottle		  */
	108, /* 78 Skakuhachi		  */
	109, /* 79 Whistle		  */
	76,  /* 80 Ocarina		  */

/* SYNTH LEAD */            
   	48,  /* 81 Lead 1 (square)        */   
   	48,  /* 82 Lead 2 (sawtooth)      */   
   	45,  /* 83 Lead 3 (calliope)      */   
   	35,  /* 84 Lead 4 (chiff)         */   
   	35,  /* 85 Lead 5 (charang)       */   
   	43,  /* 86 Lead 6 (voice)         */   
   	46,  /* 87 Lead 7 (fifths)        */   
   	32,  /* 88 Lead 8 (bass+lead)     */   

/* SYNTH PAD */
   	33,  /* 89 Pad 1 (new age)	  */
   	39,  /* 90 Pad 2 (warm)		  */
   	33,  /* 91 Pad 3 (polysynth)	  */
   	35,  /* 92 Pad 4 (choir)	  */
   	33,  /* 93 Pad 5 (bowed)	  */
   	39,  /* 94 Pad 6 (metallic)	  */
   	34,  /* 95 Pad 7 (halo)		  */
   	35,  /* 96 Pad 8 (sweep)	  */
	
/* SYNTH EFFECTS */          
        42,  /* 97  FX 1 (rain)           */   
        37,  /* 98  FX 2 (soundtrack)     */      
        36,  /* 99  FX 3 (crystal)        */   
        38,  /* 100 FX 4 (atmosphere)     */   
        38,  /* 101 FX 5 (brightness)     */  
        40,  /* 102 FX 6 (goblins)        */   
        44,  /* 103 FX 7 (echoes)         */  
        47,  /* 104 FX 8 (sci-fi)         */   
   
/* ETHNIC */
        64,  /* 105 Sitar		  */
        60,  /* 106 Banjo		  */
        60,  /* 107 Shamisen		  */
        60,  /* 108 Koto		  */
        60,  /* 109 Kalimba		  */
        112, /* 110 Bagpipe		  */
        54,  /* 111 Fiddle		  */
        54,  /* 112 Shanai		  */

/* PERCUSSIVE */            
        101, /* 113 Tinkle Bell		  */              
        104, /* 114 Agogo                 */   
        104, /* 115 Steel Drums           */             
        118, /* 116 Woodblock             */   
        117, /* 117 Taiko Drum            */   
        113, /* 118 Melodic Tom           */   
        112, /* 119 Synth Drum            */   
        119, /* 120 Reverse Cymbal        */   

/* SOUND EFFECTS */
        125, /* 121 Guitar Fret Noise	  */
        111, /* 122 Breath Noise	  */
        111, /* 123 Seashore		  */
        125, /* 124 Bird Tweet		  */
        124, /* 125 Telephone Ring	  */
        111, /* 126 Helicopter		  */
        111, /* 127 Applause		  */
        118, /* 128 Gunshot		  */

	-1
};

/* these are the note numbers */
const int perc_gm2mt[] = {
        0,
        1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
		
	35, /*  35     Acoustic Bass Drum  */      
	36, /*  36     Bass Drum 1   	   */               
	37, /*  37     Side Stick          */      
	38, /*  38     Acoustic Snare      */      
	39, /*  39     Hand Clap           */      
	40, /*  40     Electric Snare      */      
	41, /*  41     Low Floor Tom       */      
	42, /*  42     Closed Hi-Hat       */      
	43, /*  43     High Floor Tom      */      
	44, /*  44     Pedal Hi-Hat        */      
	45, /*  45     Low Tom             */      
	46, /*  46     Open Hi-Hat         */     
	47, /*  47     Low-Mid Tom         */      
	48, /*  48     Hi-Mid Tom          */      
	49, /*  49     Crash Cymbal 1      */      
	50, /*  50     High Tom            */      
	51, /*  51     Ride Cymbal 1       */      
	52, /*  52     Chinese Cymbal      */      
	53, /*  53     Ride Bell           */      
	54, /*  54     Tambourine          */      
	55, /*  55     Splash Cymbal       */      
	56, /*  56     Cowbell             */      
	57, /*  57     Crash Cymbal 2      */      
	58, /*  58     Vibraslap	   */
	59, /*  59      Ride Cymbal 2	   */
	60, /*  60      Hi Bongo    	   */
	61, /*  61      Low Bongo	   */
	62, /*  62      Mute Hi Conga	   */
	63, /*  63      Open Hi Conga	   */
	64, /*  64      Low Conga	   */
	65, /*  65      High Timbale	   */
	66, /*  66      Low Timbale	   */
	67, /*  67      High Agogo	   */
	68, /*  68      Low Agogo	   */
	69, /*  69      Cabasa		   */
	70, /*  70      Maracas		   */
	71, /*  71      Short Whistle	   */
	72, /*  72      Long Whistle	   */
	73, /*  73      Short Guiro	   */
	74, /*  74      Long Guiro	   */
	75, /*  75      Claves		   */
	76, /*  76      Hi Wood Block	   */	
	77, /*  77      Low Wood Block	   */
	78, /*  78      Mute Cuica	   */
	79, /*  79      Open Cuica	   */
	80, /*  80      Mute Triangle	   */
	81, /*  81      Open Triangle	   */
	 
	82, 
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,
	101,
	102,
	103,
	104,
	105,
	106,
	107,
	108,
	109,
	110,
	111,
	112,
	113,
	114,
	115,
	116,
	117,
	118,
	119,
	120,
	121,
	122,
	123,
	124,
	125,
	126,
	127,
	
	-1	 
};

