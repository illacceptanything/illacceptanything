`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
assert = require 'assert'
expect = require 'expect.js'

Paws = require "../Source/Paws.coffee"
Paws.utilities.infect global, Paws

describe 'The Paws reactor:', ->
   reactor = Paws.reactor
   it 'should exist', ->
      expect(reactor).to.be.ok()
