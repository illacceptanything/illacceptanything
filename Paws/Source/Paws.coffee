`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`

Paws = require './data.coffee'
require('./additional.coffee') Paws

Paws.utilities = require './utilities.coffee'

Paws.parser  = require './parser.coffee'
Paws.reactor = require './reactor.coffee'

Paws.infect = (globals)-> @utilities.infect globals, this
module.exports = Paws
