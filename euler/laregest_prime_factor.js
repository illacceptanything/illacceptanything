/* Problem specs
   * The prime factors of 13195 are 5, 7, 13 and 29.
 * What is the largest prime factor of the number 600851475143 ?
 */

 (function(largestPrime) {

   /*
    * checks whether the passed in number is prime
    * fails if the passed in number is a string, I believe
    * an ascii conversion is perfomred
    * ToDo: check the type of the passed in number (use typescript as instructor)
    */
   largestPrime.isPrime = function(number) {

     // negative numbers can not be prime
     if (number < 0) return false;

     // two is the smallest prime number
     if (number === 2) {
       return true;
     }

     // even numbers are not prime
     if (number % 2 == 0) {
       return false;
     }

     var root = Math.ceil(Math.sqrt(number));

     for (var i = root; i > 2; i--){
         if (number % i === 0){
           return false;
         }
     }

     return true;
   };


   largestPrime.largestPrimeFactor = function(number) {

     var root = Math.ceil(Math.sqrt(number));
     console.log(root);
     // todo: remove the var before i and test, then try to
     // understand why it broke
     for (var i = root; i > 2; i--) {
       if (number % i === 0) {
         if (largestPrime.isPrime(i))  return i;
       }
     }

     return 0;
   };

 })(window.largestPrime = window.largestPrime || {});