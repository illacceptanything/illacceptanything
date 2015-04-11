function cycle(input){
    var growth = 2;
    var numCycle = input;
    var height = 1;
    
    while(numCycle > 0){
        numCycle -= 1;
        
        if(growth == 1){ 
            height += 1;      
            growth = 2;
        }else if(growth == 2){
            height = height * 2;
            growth = 1;
        } 
         
    }
    console.log(height);
   
}
function processData(input) {
    //Enter your code here
    var temp = input.split("\n");

    for (i = 0; i < temp.length; i++) {
        if(i != 0){
            cycle(temp[i]);
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
