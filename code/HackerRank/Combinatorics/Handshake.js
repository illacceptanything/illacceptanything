function processData(input) {
    //Enter your code here
    var inputs = input.split("\n");
    var numHankShakes = 0;
    var i;
    
    for (i = 1; i < inputs.length; i++){
        numHankShakes = parseInt(inputs[i]) - 1;
        console.log((numHankShakes *  (numHankShakes + 1) ) / 2);    
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
