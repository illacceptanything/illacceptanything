           require('coffee-script')
var Paws = require('../Source/Paws.coffee')

Object.keys(Paws).forEach(function(key){
   if (key.export === true)
      exports[key] = Paws[key] })
