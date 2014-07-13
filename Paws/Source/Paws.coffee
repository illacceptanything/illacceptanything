`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`

process.title = 'paws.js'

Paws = require './data.coffee'

Paws.debugging = require('./additional.coffee').debugging
Paws.utilities = require './utilities.coffee'

Paws.parser  = require './parser.coffee'
Paws.reactor = require './reactor.coffee'


Paws.primitives = (bag)->
   require("./primitives/#{bag}.coffee")

Paws.generateRoot = (code = '')->
   code = Paws.parser.parse code, root: true if typeof code == 'string'
   code = new Execution code
   
   code.locals.inject Thing.with(names: yes).construct Paws.primitives 'infrastructure'
   code.locals.inject Thing.with(names: yes).construct Paws.primitives 'implementation'
   
   return code

Paws.start =
Paws.js = (code)->
   root = Paws.generateRoot code
   
   here = new Paws.reactor.Unit
   here.stage root
   
   here.start()


Paws.infect = (globals)-> @utilities.infect globals, this
module.exports = Paws
