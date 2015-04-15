import java.util.Scanner; // program uses Scanner
public class SalaryCalculator {
	// begin execution
	public static void main(String[] args) {
		
		// create Scanner object
		Scanner input = new Scanner(System.in);
		
		String name;
		int counter = 1;
		double hours;
		double salary;
		double grossPay = 0;
		
		// create while loop
		while (counter <= 3) {
			// prompt and obtain user input
			System.out.printf("Please enter employee name: ");
			name = input.nextLine();
			System.out.printf("Please enter hours worked: ");
			hours = input.nextDouble();
			System.out.printf("Please enter hourly wage: $");
			salary = input.nextDouble();
			
			if (hours <= 40) {
				grossPay = salary * hours;
			} else {
				grossPay = salary * (hours + ((hours - 40)/2));
			}
			System.out.printf("Employee name: %s\n", name);
			System.out.printf("Hours worked: %.2f\n", hours);
			System.out.printf("Hourly wage: $%.2f\n", salary);
			System.out.printf("Net pay: $%.2f\n", grossPay);
			counter++;
		} // end while loop	
	} // end main
} // end SalaryCalculator
