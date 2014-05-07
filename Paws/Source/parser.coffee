`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
Paws = require './data.coffee'

class SourceRange
   constructor: (@text, @begin, @end)->

   before:     -> @text.substring 0, @begin
   contents:   -> @text.substring @begin, @end
   after:      -> @text.substring @end

Expression = parameterizable class Expression
   constructor: (@contents, @next)->
   
   append: (expr)->
      curr = this
      curr = curr.next while curr.next
      curr.next = expr

# A simple recursive descent parser with no backtracking. No lexing is needed here.
class Parser
   labelCharacters = /[^(){} \n]/ # Not currently supporting quote-delimited labels

   constructor: (@text)->
      # Keep track of the current position into the text
      @i = 0

   # Accept a single character. If the given +char+ is at the
   # current position, proceed and return true.
   accept: (char)->
      @text[@i] is char && ++@i

   expect: (char)->
      # TODO: This should raise an exception
      @accept(char)

   # Swallow all whitespace
   whitespace: ->
      true while @accept(' ') || @accept('\n')
      true

   # Sets a SourceRange on a expression
   with_range: (expr, begin, end)->
      # Copy the source range of the contents if possible
      if expr.contents?.source?
         expr.source = expr.contents.source
      else
         expr.source = new SourceRange(@text, begin, end || @i)
         # Copy the source range to the contents if possible
         expr.contents.source = expr.source if expr.contents?
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
   braces: (delim, constructor)->
      start = @i
      if @accept(delim[0]) &&
            (it = @expr()) &&
            @whitespace() &&
            @expect(delim[1])
         @with_range(new constructor(it), start)

   # Subexpression
   paren: -> @braces('()', (it)-> it)
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


# @option {boolean=false} context:  Include the entire parse-time Script, instead of just the
#                                      current Expression
# @option {boolean=true} colour:    Convey the boundaries of context, if included, with ANSI
#                                      colour-codes instead of Unicode delimiters.
# @option {boolean=true} tag:       Include the [Type ...] tag around the output.
Expression::toString = ->
   if not @source?
      return new Serializer(this).serialize()
   
   output = if @_?.context
      use_colour = Paws.use_colour and (@_?.colour ? @_?.color ? true)
      
      magenta = '\x1b[' + '95m'
      reset   = '\x1b[' + '39m'
      [before, after] = if use_colour then [magenta, reset] else ['|', '|']
      
      '{ ' + @source.before() + before + @source.contents() + after + @source.after() + ' }'
      
   else
      @source.contents()
   
   if @_?.tag == no then output else '['+@constructor.name+' '+output+']'


module.exports =
   parse: (text)->
      parser = new Parser(text)
      parser.parse()
   
   Expression: Expression
   SourceRange: SourceRange
