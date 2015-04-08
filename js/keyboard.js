//https://github.com/grimmdude/Raphael-Keyboard

window.onload = function () {
	//--------------Editable Stuff-------------------------------
	
	// Keyboard Height
	var keyboard_height = 100;
	
	// Keyboard Width
	var keyboard_width = 624;
	
	// White Key Color
	var white_color = 'white';
	
	// Black Key Color
	var black_color = 'black';
	
	// Number of octaves
	var octaves = 6;
	
	// ID of containing Div
	var div_id = 'keyboard';
	
	//------------------------------------------------------------

	// Creates canvas 700 x 200 in above defined div
	var paper = Raphael(div_id, keyboard_width, keyboard_height);

	// Define white key specs
	var white_width = keyboard_width / 52;

	// Define black key specs
	var black_width = white_width/2;
	var black_height = keyboard_height/1.6;

	var repeat = 0;
	var keyboard_keys = [];
	
	//define white and black key names
	var wkn = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
	var bkn = ['Csharp', 'Dsharp', 'Fsharp', 'Gsharp', 'Asharp'];
	
	//create octave groups
	for (i=0;i<octaves;i++) {
		
		
		//create white keys first
		for (var w=0; w <= 6 ; w++) {
			keyboard_keys[wkn[w]+i] = paper.rect(white_width*(repeat + w), 0, white_width, keyboard_height).attr("fill", white_color);
		};
		
		//set multiplier for black key placement
		var bw_multiplier = 1.5;
		
		//then black keys on top
		for (var b=0; b <= 4 ; b++) {	
			keyboard_keys[bkn[b]+i] = paper.rect((white_width*repeat) + (black_width*bw_multiplier), 0, black_width, black_height).attr("fill", black_color);
			bw_multiplier = (b == 1) ? bw_multiplier + 4 : bw_multiplier + 2;
		};
		
		repeat = repeat + 7;
	}
	
	// Key highlighting example
	keyboard_keys.C1.attr("fill", "yellow");
	keyboard_keys.G1.attr("fill", "yellow");
	keyboard_keys.C2.attr("fill", "yellow");
	keyboard_keys.E2.attr("fill", "yellow");
};