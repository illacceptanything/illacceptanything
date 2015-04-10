# coding: utf-8
# Bibliothèque simplifiant les appels à l'API Salesforce
# Library simplifying Slesforce API calls
require 'restforce'

# Initialisation du client
# Client initialization
client = Restforce.new :username => '***********',
                       :password => '******',
                       :security_token => '******',
                       :client_id => '********',
                       :client_secret => '********'

puts client.authenticate!

# Liste dans laquelle sont stockés les identifiants des objets
# List in which we'll store identifiers
ids = []
puts 
puts "Liste des équipements enregistrés : / Listing all the stored equipments : "
puts
# On souhaite afficher l'ID, le nom d'instance et le status des équipements enregistrés dans la CMDB
# Here we want to print the ID, the instance name and the status of the equipments stored in the CMDB
client.query("select Id, BMCServiceDesk__Name__c, BMCServiceDesk__CI_Status__c from BMCServiceDesk__BMC_BaseElement__c where BMCServiceDesk__ClassName__C = 'BMC_Equipment'").each do |e|
  puts "Id : #{e.Id} | Equipement : #{e.BMCServiceDesk__Name__c} | Status : #{e.BMCServiceDesk__CI_Status__c}"
  ids << e.Id
end

puts
puts "Mise à jour du status à 'In Repair'... / Updating to 'In Repair' status ..."
puts
# On change le status de tous les équipements
# Updating all the equipments status
ids.each do |id|
  client.update('BMCServiceDesk__BMC_BaseElement__c', Id:id, BMCServiceDesk__CI_Status__c: 'In Repair')
end

# On affiche les équipements après modification
# Print again, showing the modifications
puts
puts "Liste des équipements mis-à-jour : / Listing updated equipments :"
puts
client.query("select Id, BMCServiceDesk__Name__c, BMCServiceDesk__CI_Status__c from BMCServiceDesk__BMC_BaseElement__c where BMCServiceDesk__ClassName__C = 'BMC_Equipment'").each do |e|
  puts "Id : #{e.Id} | Equipement : #{e.BMCServiceDesk__Name__c} | Status : #{e.BMCServiceDesk__CI_Status__c}"
  ids << e.Id
end
