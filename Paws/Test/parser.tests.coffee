`require = require('../Source/cov_require.js')(require)`
expect = require 'expect.js'
Paws = require '../Source/Paws.coffee'

describe 'Parser', ->
   parser = require "../Source/parser.coffee"

   it 'should be defined', ->
      expect(parser).to.be.ok()
      expect(parser.parse).to.be.a('function')
      expect(parser.Expression).to.be.ok()

   it 'should parse nothing', ->
      expr = parser.parse('')
      expect(expr).to.be.ok()
      expect(expr).to.be.a(parser.Expression)
      expect(expr.contents).to.be(undefined)
      expect(expr.next).to.be(undefined)
   
   it 'should ignore leading/trailing whitespace', ->
      expr = parser.parse '   '
      range = expr.source_range
      expect(range.end - range.begin).to.be 0 # because there's nothing *in* it
      
      expr = parser.parse '  abc  '
      range = expr.source_range
      expect(range.end - range.begin).to.be 3 # because the *code*'s length is 3 characters

   it 'should parse a label expression', ->
      expr = parser.parse('hello').next
      expect(expr.contents).to.be.a(Paws.Label)
      expect(expr.contents.alien.toString()).to.be('hello')

   it 'should parse multiple labels', ->
      expr = parser.parse('hello world').next
      expect(expr.contents).to.be.a(Paws.Label)
      expect(expr.contents.alien.toString()).to.be('hello')
      expect(expr.next.contents).to.be.a(Paws.Label)
      expect(expr.next.contents.alien.toString()).to.be('world')

   it 'should parse subexpressions', ->
      expr = parser.parse('(hello) (world)').next
      expect(expr.contents).to.be.a(parser.Expression)
      expect(expr.contents.next.contents).to.be.a(Paws.Label)
      expect(expr.next.contents).to.be.a(parser.Expression)
      expect(expr.next.contents.next.contents).to.be.a(Paws.Label)

   it 'should parse Execution', ->
      expr = parser.parse('{hello world}').next
      expect(expr.contents).to.be.a(Paws.Native)

   it 'should keep track of locations', ->
      expr = parser.parse('hello world')
      expect(expr.source_range).to.be.a(parser.SourceRange)
      expect(expr.source_range.begin).to.be(0)
      expect(expr.source_range.end).to.be(11)

      hello = expr.next
      expect(hello.source_range).to.be.a(parser.SourceRange)
      expect(hello.source_range.begin).to.be(0)
      expect(hello.source_range.end).to.be(5)

      hello_label = hello.contents
      expect(hello_label.source_range).to.be.a(parser.SourceRange)
      expect(hello_label.source_range.begin).to.be(0)
      expect(hello_label.source_range.end).to.be(5)

      world = expr.next.next
      expect(world.source_range).to.be.a(parser.SourceRange)
      expect(world.source_range.begin).to.be(6)
      expect(world.source_range.end).to.be(11)

      world_label = world.contents
      expect(world_label.source_range).to.be.a(parser.SourceRange)
      expect(world_label.source_range.begin).to.be(6)
      expect(world_label.source_range.end).to.be(11)

   it 'should keep track of tricky locations', ->
      expr = parser.parse(' h(  a{b  } )')

      contains_same = (expr) ->
         expect(expr.source_range.slice()).to.be(expr.contents.source_range.slice())

      hello = expr.next
      contains_same(hello)
      expect(hello.source_range.slice()).to.be('h')

      list = hello.next
      contains_same(list)
      expect(list.source_range.slice()).to.be('(  a{b  } )')

      a = list.contents.next
      contains_same(a)
      expect(a.source_range.slice()).to.be('a')

      exe = a.next
      contains_same(exe)
      expect(exe.source_range.slice()).to.be('{b  }')

      expect(exe.contents.position.source_range.slice()).to.be('b')

