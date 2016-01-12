
content = File.read("data.txt")
sanitized_content = content.split("\n").map{|i| i.split("-")[1].strip.downcase unless i.split("-")[1].nil?}

def most_repeated_word(content)
	word_map = Hash.new(0)
	content.map{|i| i.split(":")[1].strip.delete(".!'")}.map{|i| i.split(" ").map{|word| word_map[word] += 1 if word.length >= 6 && word != "<media" && word != "omitted>"}}
	10.times do 
		max_t = word_map.max_by{|k,v| v}
		p "#{max_t[0]} - #{max_t[1]} times"
		word_map.delete(max_t[0])
	end
end

headcount = Hash.new(0)
message_array = Hash.new
message_array["prasanna"], message_array["viji"] = [],[]

sanitized_content.each do |reply|
	unless reply.nil?
		headcount["prasanna"] += 1 if reply.include? "prasanna:"
		message_array["prasanna"] << reply if reply.include? "prasanna:"
			
		headcount["viji"] += 1 if reply.include? "viji:"
		message_array["viji"] << reply if reply.include? "viji:"

	end
end

puts "Total messages in DB: #{headcount.values.inject(:+)}"

headcount.each do |k,v|
	puts "#{k} sent #{v} messages"
end

message_array.each do |k,v|
	puts "\n *** Most Repeated Words by #{k} "
	most_repeated_word(v)
end

puts "\n *** Most Repeated Overall ***"
most_repeated_word(message_array.values.inject(:+))


	
