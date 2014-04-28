`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`


module.exports = Paws = new Object

require('./additional.coffee') Paws

Paws.utilities =
   require('./utilities.coffee')

require './data.coffee'
Paws.parser =
   require './parser.coffee'
