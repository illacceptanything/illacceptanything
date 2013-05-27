`require = require('./cov_require.js')(require)`
Paws = require './Paws.coffee'

class SourceRange
   constructor: (@source, @begin, @end) ->

   slice: -> @source.slice(@begin, @end)

class Expression
    constructor: (@contents, @next) ->
    
    append: (expr) ->
       curr = this
       curr = curr.next while curr.next
       curr.next = expr

class Parser
   labelCharacters = /[^(){} \n]/ # Not currently supporting quote-delimited labels

   constructor: (@text) ->
      @i = 0

   with_range: (expr, begin, end) ->
      if expr.contents?.source_range?
         expr.source_range = expr.contents.source_range
      else
         expr.source_range = new SourceRange(@text, begin, end || @i)
         expr.contents.source_range = expr.source_range if expr.contents?
      expr

   character: (char) ->
      @text[@i] is char && ++@i

   whitespace: ->
      true while @character(' ') || @character('\n')
      true

   label: ->
      @whitespace()
      start = @i
      res = ''
      while @text[@i] && labelCharacters.test(@text[@i])
         res += @text[@i++]
      res && @with_range(new Paws.Label(res), start)

   braces: (delim, constructor) ->
      start = @i
      if @whitespace() &&
            @character(delim[0]) &&
            (it = @expr()) &&
            @whitespace() &&
            @character(delim[1])
         @with_range(new constructor(it), start)

   paren: -> @braces('()', (it) -> it)
   scope: -> @braces('{}', Paws.Native)

   expr: ->
      start = @i
      substart = @i
      res = new Expression
      while sub = (@label() || @paren() || @scope())
         res.append(@with_range(new Expression(sub), substart))
         substart = @i
      @with_range(res, start)

   parse: ->
    @expr()

module.exports =
   parse: (text) ->
      parser = new Parser(text)
      parser.parse()
   
   Expression: Expression
   SourceRange: SourceRange

