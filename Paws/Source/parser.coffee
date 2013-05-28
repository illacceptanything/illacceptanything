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

# A simple recursive descent parser with no backtracking. No lexing is needed here.
class Parser
   labelCharacters = /[^(){} \n]/ # Not currently supporting quote-delimited labels

   constructor: (@text) ->
      # Keep track of the current position into the text
      @i = 0

   # Accept a single character. If the given +char+ is at the
   # current position, proceed and return true.
   accept: (char) ->
      @text[@i] is char && ++@i

   expect: (char) ->
      # TODO: This should raise an exception
      @accept(char)

   # Swallow all whitespace
   whitespace: ->
      true while @accept(' ') || @accept('\n')
      true

   # Sets a SourceRange on a expression
   with_range: (expr, begin, end) ->
      # Copy the source range of the contents if possible
      if expr.contents?.source_range?
         expr.source_range = expr.contents.source_range
      else
         expr.source_range = new SourceRange(@text, begin, end || @i)
         # Copy the source range to the contents if possible
         expr.contents.source_range = expr.source_range if expr.contents?
      expr

   # Parses a single label
   label: ->
      start = @i
      res = ''
      while @text[@i] && labelCharacters.test(@text[@i])
         res += @text[@i]
         @i++
      res && @with_range(new Paws.Label(res), start)

   # Parses an expression delimited by some characters
   braces: (delim, constructor) ->
      start = @i
      if @accept(delim[0]) &&
            (it = @expr()) &&
            @whitespace() &&
            @expect(delim[1])
         @with_range(new constructor(it), start)

   # Subexpression
   paren: -> @braces('()', (it) -> it)
   # Execution
   scope: -> @braces('{}', Paws.Native)

   # Parses an expression
   expr: ->
      # Strip leading whitespace
      @whitespace()
      # The whole expression starts at this position
      start = @i
      # and ends here
      end = @i
      # The subexpression starts here
      substart = @i

      res = new Expression
      while sub = (@label() || @paren() || @scope())
         res.append(@with_range(new Expression(sub), substart))
         # Expand the expression range (exclude trailing whitespace)
         end = @i
         @whitespace()
         # Set the position of the next expression (exclude leading whitespace)
         substart = @i

      @with_range(res, start, end)

   parse: ->
      @expr()

module.exports =
   parse: (text) ->
      parser = new Parser(text)
      parser.parse()
   
   Expression: Expression
   SourceRange: SourceRange

