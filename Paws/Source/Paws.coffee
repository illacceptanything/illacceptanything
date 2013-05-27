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
   
   Label: class Label extends Thing
      constructor: (@alien) ->
   
   Execution: class Execution extends Thing
   
   Native: class Native extends Execution
      constructor: (@position) ->
