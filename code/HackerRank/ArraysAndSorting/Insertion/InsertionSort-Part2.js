function processData(input) {
    //Enter your code here
    var temp = input.split("\n");
    var list = temp[1].split(" ");
    var i, k, store;
    
    for (i = 1; i < list.length; i++){
        k = i;
        while (k > 0 & parseInt(list[k-1]) > parseInt(list[k])){
            store = list[k];
            list[k] = list[k-1];
            list[k-1] = store;
            k -= 1;
        }
        console.log(list.join(" "));
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
