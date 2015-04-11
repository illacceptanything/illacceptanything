function processData(input) {
    //Enter your code here
    var i, numChoco, numHori, numVert; 
    var temp = input.split("\n");
	
    for (i = 1; i < temp.length; i++){
        if (parseInt(temp[i]) % 2 == 0){
            numHori = parseInt(temp[i]) / 2;
            numVert = parseInt(temp[i]) / 2;
        }else {
            numHori = (parseInt(temp[i]) + 1) / 2;
            numVert = (parseInt(temp[i]) - 1) / 2;
        }
        numChoco = numHori * numVert;
        console.log(numChoco);
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
