function processData(input) {
    //Enter your code here
    var i, k, j, line, diff;
    var temp = input.split('\n');
    var n_m = temp[0].split(" ");
    var total = 0;
    
    for (i = 1; i < temp.length; i++){
        line = temp[i].split(" ");
        diff = parseInt(line[1]) - parseInt(line[0]) + 1;
        total += parseInt(line[2]) * diff;
    }
    
    var average = Math.floor(total / parseInt(n_m[0]));

    console.log(average);
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
