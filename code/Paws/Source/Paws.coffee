# Hello! I am Paws! Come read about me, and find out how I work.
#
#                        ,d88b.d88b,
#                        88888888888
#                        `Y8888888Y'
#                          `Y888Y'
#                            `Y'

process.title = 'paws.js'

Paws = require './data.coffee'

Paws.utilities = require './utilities.coffee'

Paws.parse   = require './parser.coffee'
Paws.reactor = require './reactor.coffee'


Paws.primitives = (bag)->
   require("./primitives/#{bag}.coffee")()

Paws.generateRoot = (code = '')->
   code = Paws.parse Paws.parse.prepare code if typeof code == 'string'
   code = new Execution code
   
   code.locals.inject Paws.primitives 'infrastructure'
   code.locals.inject Paws.primitives 'implementation'
   
   return code

Paws.start =
Paws.js = (code)->
   root = Paws.generateRoot code
   
   here = new Paws.reactor.Unit
   here.stage root
   
   here.start()


Paws.infect = (globals)-> @utilities.infect globals, this
module.exports = Paws
