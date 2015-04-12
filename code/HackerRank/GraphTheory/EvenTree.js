function processData(input) {
    //Enter your code here
    var inputs = input.split("\n");
    var NM = inputs[0].split(" ");
    var numVert = parseInt(NM[0]);
    var numChild = [];
    var ancestor = [];
    var i, k, parent, child;
    var counter = -1;
    
    for (i = 0; i < numVert; i++){
        numChild[i] = 1;
        ancestor[i] = 0;
    }
    
    for (i = 1; i < inputs.length; i++){
        var temp = inputs[i].split(" ");
        parent = parseInt(temp[1]) - 1;
        child = parseInt(temp[0]) - 1;
        numChild[parent] += numChild[child];
        ancestor[child] = parent + 1;
        var root = ancestor[parent];

        // Explain
        // parent + 1 because root != 0
        // So the ancestor of 3 is 1, not 0
        // This traverse through a single path buttom up
        while(root != 0){
            numChild[root - 1] += numChild[child];
            root = ancestor[root - 1];
        }
        
        //console.log(numChild, ancestor);
    }
    
    for (k = 0; k < numChild.length; k++){
        if (numChild[k] % 2 == 0){
            counter += 1;
        }
    }
    console.log(counter);
    
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
