package WarmUp;

import java.util.Scanner;

/**
 * Created by zuhayrelahi1 on 4/11/15.
 */
public class UtopianTree {

    /*
     * Calculate the height of the tree
     *
     * @param numOfGrowthCycles - # of growth cycles
     */
    public static int calcTreeHeight(int numOfGrowthCycles){
        //calculate the height of the tree
        int treeHeight = 1;

        //no growth cycles
        if(numOfGrowthCycles == 0){
            return treeHeight;
        }

        for(int i = 1; i <= numOfGrowthCycles; i++){
            //double the height if we are on an odd growth cycle
            if(i % 2 != 0){
                treeHeight = treeHeight * 2;
            }else {
                //increment the height
                treeHeight += 1;
            }
        }
        return treeHeight;
    }

    //test function
    public static void main(String args[]){
        Scanner sc = new Scanner(System.in);
        int numOfTestCases = sc.nextInt();

        for(int i = 0; i < numOfTestCases; i++){
            int numOfTrees = sc.nextInt();
            System.out.println(calcTreeHeight(numOfTrees));
        }
        sc.close();
    }
}
