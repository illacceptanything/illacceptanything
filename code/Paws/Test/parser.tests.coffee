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
   
   describe 'Expression', ->
      it 'exists', ->
         expect(Expression).to.be.ok()
         expect(Expression).to.be.a 'function'
         
         expect(-> new Expression).to.not.throwException()
         expect(   new Expression).to.be.a Expression
      
      it "contains Things as 'words'", ->
         a_thing = new Thing
         
         expr = new Expression.from [a_thing]
         expect(expr).to.be.a Expression
         expect(expr.words).to.have.length 1
         expect(expr.words).to.eql [a_thing]
      
      it 'constructs Strings into Label-words', ->
         a_label = 'foo'
         
         expr = new Expression.from [a_label]
         expect(expr).to.be.a Expression
         expect(expr.words).to.have.length 1
         expect(expr.at 0       ).to.be.a Label
         expect(expr.at(0).alien).to.be a_label
      
      it 'can create sub-expressions', ->
         expect(-> new Expression.from ['foo', ['bar'], 'baz']).to.not.throwException()
   
      it 'creates a Sequence to wrap sub-expressions', ->
         expr = new Expression.from [['bar']]
         expect(expr).to.be.a Expression
         expect(expr.words).to.have.length 1
         expect(expr.at 0).to.be.a Sequence
   
      it 'recurses to create contents of sub-expressions', ->
         a_thing = new Thing
         
         expr = new Expression.from ['foo', [a_thing], 'baz']
         expect(expr).to.be.a Expression
         expect(expr.words).to.have.length 3
         expect(expr.at 1).to.be.a Sequence
         
         sub = expr.at(1).at(0)
         expect(sub).to.be.an Expression
         expect(sub.at 0).to.be a_thing
   
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
      
      it 'a label expression', ->
         seq = parse('foo')
         expect(seq).to.be.a(parse.Sequence)
         
         expr = seq.at 0
         expect(expr).to.be.a(parse.Expression)
         
         label = expr.at 0
         expect(label).to.be.ok()
         expect(label).to.be.a Label
         expect(label.alien).to.be 'foo'
      
      it 'multiple sequential labels in an expression', ->
         seq = parse('foo bar')
         expect(seq).to.be.a(parse.Sequence)
         
         expr = seq.at 0
         expect(expr).to.be.a(parse.Expression)
         
         foo = expr.at 0
         expect(foo).to.be.ok()
         expect(foo).to.be.a Label
         expect(foo.alien).to.be 'foo'
         bar = expr.at 1
         expect(bar).to.be.ok()
         expect(bar).to.be.a Label
         expect(bar.alien).to.be 'bar'
   
   it '... while retaining knowledge of the source-code context', ->
                 # 0123456789A #
      seq = parse('hello world')
      expect(Context.for seq).to.be.ok()
      expect(Context.for(seq).begin).to.be 0
      expect(Context.for(seq).end).to.be 10

      expr = seq.at 0
      expect(Context.for expr).to.be.ok()
      expect(Context.for(expr).begin).to.be 0
      expect(Context.for(expr).end).to.be 10

      hello = expr.at 0
      expect(Context.for hello).to.be.ok()
      expect(Context.for(hello).begin).to.be 0
      expect(Context.for(hello).end).to.be 4

      world = expr.at 1
      expect(Context.for world).to.be.ok()
      expect(Context.for(world).begin).to.be 6
      expect(Context.for(world).end).to.be 10
