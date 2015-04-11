function processData(input) {
    //Enter your code here
    var inputs = input.split("\n");
    var i, numPair;
    
    for (i = 1; i < inputs.length; i++){
        numPair = parseInt(inputs[i]);
        console.log(numPair + 1);
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
