function processData(input) {
    //Enter your code here
    var temp = input.split('\n');
    var i;
    
    for(i = 1; i < temp.length; i++){
        var count = 0;
        var last = temp[i].length - 1;
        for(k = 0; k < temp[i].length/2; k++){
            var a = temp[i].charCodeAt(k);
            var b = temp[i].charCodeAt(last);
            count += Math.abs(a - b);
            last -= 1;
        }
        console.log(count);
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
