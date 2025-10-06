class Solution {
    public int maxBottlesDrunk(int numBottles, int numExchange) {
        if(numExchange>numBottles)return numBottles;
        int totalBottles = numBottles,emptyBottles = numBottles;
        while(emptyBottles/numExchange>0){
            totalBottles++;
            emptyBottles = emptyBottles - numExchange + 1;
            numExchange++;
        }
        return totalBottles;
    }
}
