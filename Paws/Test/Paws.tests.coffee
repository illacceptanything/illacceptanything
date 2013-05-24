`require = require('../Source/cov_require.js')(require)`
expect = require 'expect.js'

describe 'the Paws object', ->
   Paws = require "../Source/Paws.coffee"
   it 'should be defined', ->
      expect(Paws).to.be.ok()
