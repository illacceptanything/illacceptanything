function bitwiseOR(stringA, stringB){
    var i;
    var newString = "";
    
    for (i = 0; i < stringA.length; i++){
        if(stringA.charAt(i) === '1' || stringB.charAt(i) === '1'){
            newString += '1';
        }else{
            newString += '0';
        }
    }
    var numTopics = (newString.match(/1/g) || []).length;
    return numTopics;
}

function processData(input) {
    //Enter your code here
    var temp = input.split("\n");
    var tempA, tempB, i, k;
    var topics = 0;
    var maxTopics = 0;
    var maxTeams = 0;

    for (i = 1; i < temp.length; i++) {
        for (k = i + 1; k < temp.length; k++){
            topics = bitwiseOR(temp[i], temp[k]);
            if (topics > maxTopics){
                maxTopics = topics;
                maxTeams = 1;
            }else if (topics == maxTopics){
                maxTeams += 1
            }
        }
    }
    console.log(maxTopics);
    console.log(maxTeams);
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
