expect = require 'expect.js'
Paws = require '../Source/Paws.coffee'

describe 'the Paws object', ->
   it 'should be defined', ->
      expect(Paws).to.be.ok()
