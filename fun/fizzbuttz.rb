# Fizzbuttz by Aspleenic

(1..100).each do |x|
	if x % 3 == 0 && x % 5 != 0
		puts 'fizz'
	elsif x % 5 == 0 && x % 3 != 0
		puts 'buttz'
	elsif x % 5 == 0 && x % 3 == 0
		puts 'fizzbuttz'
	else 
		puts x
	end
end