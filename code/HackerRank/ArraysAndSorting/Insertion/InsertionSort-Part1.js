function processData(input) {
    //Enter your code here
    var temp = input.split("\n");
    var array = temp[1].split(" ");
    var i = array.length - 1;
    var tempStore;
    var exit = true;
    
    while(i > 0 & exit){
        if (parseInt(array[i-1]) > parseInt(array[i])){
            tempStore = parseInt(array[i]);
            array[i] = array[i-1];
        }else{
            if (tempStore > parseInt(array[i-1])){
                array[i] = tempStore;
                exit = false;
            }else{
                array[i] = array[i-1];
            }
        }
        console.log(array.join(" "));
        i -= 1;
    }
    if (exit){
        array[0] = tempStore;
        console.log(array.join(" "));
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
