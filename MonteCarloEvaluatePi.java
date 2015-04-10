import java.util.Random;

/**
 * Created by kenny on 5/29/14.
 */
public class MonteCarloEvaluatePi {

    private static final Random RANDOM = new Random();
    /*
        A_circle = πr^2
        A_square = (2r)^2

        circle transcribed in a square, r_c = r_s = r

        A_square / A_circle (r = 1) = ratio of areas between square and circle
        = 4 / π
      */
    public static void main(String[] args) {
        monteCarlo(10);
        monteCarlo(100);
        monteCarlo(1000);
        monteCarlo(10000);
        monteCarlo(100000);
        monteCarlo(1000000);
        monteCarlo(10000000);
        monteCarlo(100000000);
    }

    /*
        make n random guesses, take the ratio of guesses in the circle,
        and divide that by the total guesses (all of which will be in the square)
        this will give you the A_square / A_circle ratio, 4/π
    */
    public static void monteCarlo(final int randomSamples) {
        int inCircle = 0;
        for(int i = 0; i < randomSamples; i++) {
            // generate random x/y variables between -1, 1 (which are guaranteed to be within the square
            final double x = 2 * RANDOM.nextDouble() - 1;
            final double y = 2 * RANDOM.nextDouble() - 1;
            if(inCircle(x, y)) {
                inCircle++;
            }
        }
        final double ratio = (double) inCircle / randomSamples;
        final double piApprox = ratio * 4; // ratio is 4/π, so multiply by 4 to get π approximation

        System.out.println("Random Samples: " + randomSamples);
        System.out.println("In circle: " + inCircle + ", In square: " + randomSamples);
        System.out.println("PI Approximation: " + piApprox + ", Error: " + Math.abs(Math.PI - piApprox));
    }

    /*
        eq of circle centered at origin: x^2 + y^2 = r^2
        if (x)^2 + (y)^2 < r^2 then is within circle where r = 1
     */
    private static boolean inCircle(final double x, final double y) {
        // System.out.println(x + ", " + y);
        return x * x + y * y < 1.0;
    }

}
