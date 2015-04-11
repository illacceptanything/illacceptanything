function processData(input) {
    //Enter your code here
    var i, k, range;
    var lowerRange = new Array();
    var temp = input.split('\n');
    
    for (i = 1; i < temp.length; i++){
        range = temp[i].split(" ");
        var numSquare = 0;
        
        lowerRange[0] = Math.floor(Math.sqrt(range[0]));
        lowerRange[1] = Math.floor(Math.sqrt(range[1]));
        
        for (k = lowerRange[0]; k <= lowerRange[1]; k++){
            var square = k * k;
            if (range[0] <= square & square <= range[1]){
                numSquare += 1;
            }
        }
        console.log(numSquare);
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
