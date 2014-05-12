`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`

process.title = 'paws.js'

Paws = require './data.coffee'

Paws.debugging = require('./additional.coffee').debugging
Paws.utilities = require './utilities.coffee'

Paws.parser  = require './parser.coffee'
Paws.reactor = require './reactor.coffee'

Paws.infect = (globals)-> @utilities.infect globals, this
module.exports = Paws
