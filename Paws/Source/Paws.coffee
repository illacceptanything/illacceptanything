utilities = require('./utilities').infect global

Unit   = require './Unit'
Script = require './Script'

parameterizable class Thing
   constructor: ->
      return this

module.exports = Paws =
   Thing: Thing
   
   Unit: Unit
   Script: Script
