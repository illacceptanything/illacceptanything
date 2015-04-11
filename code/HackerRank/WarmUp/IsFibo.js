function isPerfectSqaure(num1, num2){
    var temp1 = Math.floor(Math.sqrt(num1));
    var temp2 = Math.floor(Math.sqrt(num2));
    var bool = false;
   
    if(temp1 * temp1 == num1){
        bool = true;
    }
    if(temp2 * temp2 == num2){
        bool = true;
    }
    if(bool){
        console.log("IsFibo");
    }else{
        console.log("IsNotFibo");
    }
}
function isFab(input){

    var num1 = (5*input*input + 4);
    var num2 = (5*input*input - 4);
    isPerfectSqaure(num1, num2);
}
function processData(input) {
    //Enter your code here
    var temp = input.split("\n");

    for (i = 0; i < temp.length; i++) {
        if(i != 0){
            isFab(temp[i]);
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
