function processData(input) {
    //Enter your code here
    var inputs = input.split("\n");
    var i, k, numTowns, numRoutes, routes, totalRoutes;
    
    for (i = 1; i < inputs.length; i += 2){
        numTowns = parseInt(inputs[i]);
        numRoutes = inputs[i+1].split(" ");
        totalRoutes = 1;
        
        for (k = 0; k < numRoutes.length; k++){
            routes = parseInt(numRoutes[k]);
            totalRoutes *= routes;
            totalRoutes %= 1234567;
        }
		// For some reason, I can' put
		// console.log(totalRoutes % 1234567);
        console.log(totalRoutes);
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
