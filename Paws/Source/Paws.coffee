`require = require('./cov_require.js')(require)`
utilities = require('./utilities.coffee').infect global

Unit   = require './Unit.coffee'
Script = require './Script.coffee'

parameterizable class Thing
   constructor: ->
      return this

module.exports = Paws =
   Thing: Thing
   
   Unit: Unit
   Script: Script
