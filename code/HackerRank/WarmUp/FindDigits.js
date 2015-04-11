function divide(input){
    var counter = 0;
    var num = input;

    for (k = 0; k < input.toString().length; k++) {
        var lastDigit = num % 10;
        if(input % lastDigit == 0 && lastDigit != 0){
            counter ++ ;
        }
        num = Math.floor(num / 10);
    }
    console.log(counter);
}

function processData(input) {
    //Enter your code here
    var temp = input.split("\n");
    for (i = 0; i < temp.length; i++) {
        if(i != 0){
            divide(temp[i]);
        }
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
