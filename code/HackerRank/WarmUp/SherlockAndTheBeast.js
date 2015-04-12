function processData(input) {
    //Enter your code here
    var inputs = input.split("\n");
    var i, n, num;
    
    for (i = 1; i < inputs.length; i++){
        num = parseInt(inputs[i]);
        var str = "";
        for (n = num; n >= 0; n--){
            if (n % 3 == 0 & (num - n) % 5 == 0){
                str = Array(n + 1).join("5") + Array(num - n + 1).join("3");
                console.log(str);
                break;
            }
        }
        if (str === ""){
            console.log(-1);
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
