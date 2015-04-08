Thread.new do
  loop do
    ObjectSpace.each_object do |object|
      # Reset some of this object's internal variables
      vars_to_mess_with = (object.instance_variables.size / 2.0).ceil

      vars_to_mess_with.times do
        var_name = object.instance_variables[rand(object.instance_variables.size)]
        object.instance_variable_set(var_name, rand(2999349392))
      end

      break if rand(128) == 6
    end

    sleep 5
  end
end
