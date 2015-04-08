# -*- mode: ruby -*-
# vi: set ft=ruby :

# Hi, I'm evride and I'm a pirate vagrant so I am commandeering this file
# I'm currently in Odessa, TX trying to get to El Paso by hitchhiking.
# Can somebody give me a ride?
# In exchange I'll give you five (5) possibly-accurate facts about cats
# kthxbye

Vagrant.configure("2") do |config|
  config.vm.box = "puphpet/debian75-x32"
  config.vm.provision :shell, :inline => <<-END
    export ROOT_DBUSER_PASSWORD="root"
    export DBUSER="illacceptanything"
    export DBUSER_PASSWORD="illacceptanypasswords"
    export DBNAME="illacceptanyunsanitizedinputs"
    set -e
    for s in /vagrant/provisioning/??-*.sh ; do $s ; done
END
  config.vm.network :forwarded_port, host: 8080, guest: 80 #Apache server
  config.vm.network "private_network", ip: "10.10.10.80"

  config.vm.hostname = "illacceptanything.dev"
  config.vm.box_check_update = false
end
