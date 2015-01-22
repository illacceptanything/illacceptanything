expect = require 'expect.js'

Paws   = require '../Source/Paws.coffee'
parser = require "../Source/parser.coffee"

describe 'Parser', ->
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
      range = expr.source
      expect(range.end - range.begin).to.be 0 # because there's nothing *in* it
      
      expr = parser.parse '  abc  '
      range = expr.source
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
   
   # FIXME: This doesn't even *begin* to be well-exercised.
   it 'should parse quoted labels', ->
      expr = parser.parse('"hello, world!"').next
      expect(expr.contents).to.be.a(Paws.Label)
      expect(expr.contents.alien.toString()).to.be('hello, world!')
      
      expr = parser.parse('hello “elliott cable”').next
      expect(expr.contents).to.be.a(Paws.Label)
      expect(expr.contents.alien.toString()).to.be('hello')
      expect(expr.next.contents).to.be.a(Paws.Label)
      expect(expr.next.contents.alien.toString()).to.be('elliott cable')

   it 'should parse subexpressions', ->
      expr = parser.parse('[hello] [world]').next
      expect(expr.contents).to.be.a(parser.Expression)
      expect(expr.contents.next.contents).to.be.a(Paws.Label)
      expect(expr.next.contents).to.be.a(parser.Expression)
      expect(expr.next.contents.next.contents).to.be.a(Paws.Label)

   it 'should parse Execution', ->
      expr = parser.parse('{ hello world }').next
      expect(expr.contents).to.be.a(Paws.Execution)
      expect(expr.contents).to.not.be.a(Paws.Native)

   it 'should keep track of locations', ->
      expr = parser.parse('hello world')
      expect(expr.source).to.be.a(parser.SourceRange)
      expect(expr.source.begin).to.be(0)
      expect(expr.source.end).to.be(11)

      hello = expr.next
      expect(hello.source).to.be.a(parser.SourceRange)
      expect(hello.source.begin).to.be(0)
      expect(hello.source.end).to.be(5)

      hello_label = hello.contents
      expect(hello_label.source).to.be.a(parser.SourceRange)
      expect(hello_label.source.begin).to.be(0)
      expect(hello_label.source.end).to.be(5)

      world = expr.next.next
      expect(world.source).to.be.a(parser.SourceRange)
      expect(world.source.begin).to.be(6)
      expect(world.source.end).to.be(11)

      world_label = world.contents
      expect(world_label.source).to.be.a(parser.SourceRange)
      expect(world_label.source.begin).to.be(6)
      expect(world_label.source.end).to.be(11)

   it 'should keep track of tricky locations', ->
      expr = parser.parse(' h[  a{b  } ]')

      contains_same = (expr)->
         expect(expr.source.contents()).to.be(expr.contents.source.contents())

      hello = expr.next
      contains_same(hello)
      expect(hello.source.contents()).to.be('h')

      list = hello.next
      contains_same(list)
      expect(list.source.contents()).to.be('[  a{b  } ]')

      a = list.contents.next
      contains_same(a)
      expect(a.source.contents()).to.be('a')

      exe = a.next
      contains_same(exe)
      expect(exe.source.contents()).to.be('{b  }')

      expect(exe.contents.position.source.contents()).to.be('b')


# FIXME: This is currently tested *in terms of the `Parser`'s results*. I dislike that; any problem
#        in the `Parser` could shadow problems in the `Serializer`. Howevr, it's mildly necessary
#        now; because I don't have another remotely expident way to create Expressions right now.
# TODO: In that spirit ... write a Expression.construct() method. (JSON-to-Expression?)
describe 'Serializer', ->
   it 'generates an empty string for an empty expression', ->
      expr = parser.parse('')
      expect(expr.serialize()).to.be ''
   
   it 'generates no whitespace at the ends', ->
      expr = parser.parse('foo [bar] baz')
      expect(expr.serialize()).to.not.match /^\s+|\s+$/
      
      expr = parser.parse('foo [bar] [baz]')
      expect(expr.serialize()).to.not.match /^\s+|\s+$/
   
   it 'generates quotes around Labels', ->
      expr = parser.parse('foo')
      expect(expr.serialize()).to.be '“foo”'
   
   it 'generates parenthesis around sub-expressions', ->
      expr = parser.parse('[foo]')
      expect(expr.serialize()).to.be '[“foo”]'
   
   it 'puts spaces between adjacent Labels', ->
      expr = parser.parse('foo bar')
      expect(expr.serialize()).to.be '“foo” “bar”'
   
   it 'puts no space inside the start or end of expressions', ->
      expr = parser.parse('abc [def] ghi')
      expect(expr.serialize()).to.be '“abc” [“def”] “ghi”'
   
   it 'handles complex nested expressions', ->
      expr = parser.parse('[bar [[foo]]] baz')
      expect(expr.serialize()).to.be '[“bar” [[“foo”]]] “baz”'
