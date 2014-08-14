`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
expect = require 'expect.js'

Paws  = require '../Source/Paws.coffee'
parse = require "../Source/parser.coffee"

describe 'Parser', ->
   it 'exists', ->
      expect(parse).to.be.ok()
      expect(parse).to.be.a 'function'
   
   Sequence    = parse.Sequence
   Expression  = parse.Expression
   Context     = parse.Context
   
   describe 'Context', ->
      it 'exists', ->
         expect(Context).to.be.ok()
         expect(Context).to.be.a 'function'
         
         expect(-> new Context).to.not.throwException()
         expect(   new Context).to.be.a Context
      
      it 'can associate an instance of itself with any object', ->
         an_object = new Object; another_object = new Object
         
         expect(-> Context.on an_object, 'abc').to.not.throwException()
         expect(   Context.on an_object, 'abc').to.be.a Context
      
      it 'can be retreived from an object', ->
         an_object = new Object; some_text = 'abc'
         
         Context.on an_object, some_text
         expect(-> Context.for an_object).to.not.throwException()
         expect(   Context.for an_object).to.be.a Context
         expect(   Context.for(an_object).text).to.be some_text
      
      it 'can store a range within the source-text', ->
         an_object = new Object; some_text = 'abc def ghi'
         
         Context.on an_object, some_text, 4, 7
         expect(Context.for(an_object).source()).to.be 'def'
      
      it 'can retreive the text *before* the source', ->
         an_object = new Object; some_text = 'abc def ghi'
         
         Context.on an_object, some_text, 4, 7
         expect(Context.for(an_object).before()).to.be 'abc '
      
      it 'can retreive the text *after* the source', ->
         an_object = new Object; some_text = 'abc def ghi'
         
         Context.on an_object, some_text, 4, 7
         expect(Context.for(an_object).after()).to.be ' ghi'
   
   # This is a bare-minimum test of the moving-parts *between* the PEG and the API.
   # I need to write much more in-depth parser tests; preferably something that doesn't require five
   # lines of code to check a single word. (Some Stack Overflow genius suggests an intermediate-form
   # XML parse-structure exclusive to your test-base?)
   describe 'parses ...', ->
      it 'nothing', ->
         structure = parse('')
         expect(structure).to.be.ok()
         expect(structure).to.be.a(parse.Sequence)
         
         expr = structure.at 0
         expect(expr).to.be.ok()
         expect(expr).to.be.a(parse.Expression)
