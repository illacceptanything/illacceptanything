function processData(input) {
    //Enter your code here
    var i, k;
    var tempArray = input.split('\n');
    var length = tempArray.length;
    var tempChar;
    
    for(i = 1; i < length ; i++){
        var numDel = 0;
        tempChar = 'C';
        
        for(k = 0; k < tempArray[i].length; k++){
            if(tempArray[i].charAt(k) === tempChar){
                numDel++;
            }
            tempChar = tempArray[i].charAt(k);
        }
        console.log(numDel);
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
