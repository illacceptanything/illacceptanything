`require = require('./cov_require.js')(require)`

require('./additional.coffee') module.exports =
   paws = new Object

paws.utilities       = require('./utilities.coffee').infect global
paws.Unit   = Unit   = require './Unit.coffee'
paws.Script = Script = require './Script.coffee'

paws.Thing = Thing = parameterizable class Thing
   constructor: ->
      return this
   
paws.Label = Label = class Label extends Thing
   constructor: (@alien) ->

paws.Execution = Execution = class Execution extends Thing

paws.Native = Native = class Native extends Execution
   constructor: (@position) ->
