function sortNumber(a,b) {
    return a - b;
}

function processData(input) {
    //Enter your code here
    var inputs = input.split("\n");
    var i, j, a, b, numStones, secondStone;
    var firstStone = 0;
    var list;
    
    for (i = 1; i < inputs.length; i += 3){
        numStones = parseInt(inputs[i]);
        a = parseInt(inputs[i+1]);
        b = parseInt(inputs[i+2]);
        list = [];

        for (j = 0; j < numStones; j++){
            // All possible combination of a and b
            // If numStones = 4, then we have 
            // 3a, 2a 1b, 1a 2b, 0a 3b. 
            list.push(((numStones - j - 1) * a) + (j * b));
        }
        
        // Remove duplicate element
		// The index stays the same if the value of the element is not unique
		// eg: 1800 0, and 1800 1, list.indexOf(1800) == 1 will return false
		// Since 0 is before 1
        list = list.filter(function(elem, index) {
            return list.indexOf(elem) == index;
        }); 
        
        list = list.sort(sortNumber);
        console.log(list.join(" "));
    }
} 

process.stdin.resume();
process.stdin.setEncoding("ascii");
_input = "";
process.stdin.on("data", function (input) {
    _input += input;
});

process.stdin.on("end", function () {
   processData(_input);
});
