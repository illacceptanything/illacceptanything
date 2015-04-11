function processData(input) {
    //Enter your code here
    var i, NCM;
    var numChoco = 0;
    var numWrapper = 0;
    var temp = input.split('\n');
    
    for (i = 1; i < temp.length; i++){
        NCM = temp[i].split(" ");
        numChoco = Math.floor(NCM[0] / NCM[1])
        numWrapper = numChoco;
        while(numWrapper - NCM[2] >= 0){
            numChoco += 1;
            numWrapper += 1;
            numWrapper -= NCM[2];
        }
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
