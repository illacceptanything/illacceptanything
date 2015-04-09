# Attached Below is an updated, remastered, interpretation of 
# TROOOOOOGGGGDOOOOOOR

class Trogdor
	def burninate(peasants)
		for p in peasants
			burn(p)
		end
	end

	def burn(peasant)
		puts 
		"""
		TROOOOOOGGGGDOOOOOOR
		TROOOOOOGGGGDOOOOOOR 
		Burninating the peasants
		All across the land
		There was this #{peasant.sex} named was #{peasant.name}
		THEN THEY DIIIIIEEEEDDD
		THEN THEY DIIIIIEEEEDDD
		FROM THE FLAMING FLAMES OF TROGDOR.
		YEAH.\n
		"""
	end
end

class Peasant
	attr_accessor :name, :sex

	def initialize(name, sex)
		@name = name
		@sex = sex
	end
end
