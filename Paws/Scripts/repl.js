#!/usr/bin/env node

// Usage:
// ------
// This opens a CoffeeScript REPL, with the Paws.js API loaded in for use:
// 
//     $ npm run-script repl
//     Paws.js> Paws.something 123
//     Paws.js> .exit
// 
// All of the Paws.js APIs are exposed both under `Paws.*`, as well as made directly accessible.

           require('coffee-script/register')
var repl = require('coffee-script/lib/coffee-script/repl')
  , path = require('path')
    
  , Paws = require('../Source/Paws.coffee')

r = repl.start({
   prompt: 'Paws.js> '
,  historyFile: path.resolve(__dirname + '/../.repl_history')
})

require('../Source/utilities.coffee').infect(r.context, Paws)
                                             r.context.Paws = Paws
                                             r.context.P    = Paws
