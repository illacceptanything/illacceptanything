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
   
   serialize: -> new Serializer(this).serialize()

# A simple recursive descent parser with no backtracking. No lexing is needed here.
class Parser
   labelCharacters = /[^\[\]{} \r\n]/ # Not currently supporting quote-delimited labels

   constructor: (@text, opts = {})->
      # Keep track of the current position into the text
      @i = 0
      
      if opts.root and @text.slice(0,2) == '#!'
         @text = @text.split("\n").slice(1).join("\n")

   # Accept a single character. If the given +char+ is at the
   # current position, proceed and return true.
   accept: (char)->
      @text[@i] is char && ++@i

   expect: (char)->
      # TODO: This should raise an exception
      @accept(char)

   # Swallow all whitespace
   whitespace: ->
      true while @accept(' ') || @accept('\r') || @accept('\n')
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
   paren: -> @braces('[]', (it)-> it)
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

#---
# XXX: Why are we doing this disgusting Java-esque “everything is a class” form? o_O - ELLIOTTCABLE
class Serializer
   constructor: (@root)->
   
   serialize: (node, text)->
      return @serialize @root, "" unless node?
      {contents, next} = node
      
      if contents instanceof Label
         text += '"'+contents.alien+'"'
         text += ' ' if next
      
      if contents instanceof Expression
         text += '['+@serialize(contents, '')+']'
         text += ' ' if next
      
      return text unless next?
      return @serialize next, text


# @option {boolean=false} context:  Include the entire parse-time Script, instead of just the
#                                      current Expression
# @option {boolean=true} colour:    Convey the boundaries of context, if included, with ANSI
#                                      colour-codes instead of Unicode delimiters.
# @option {boolean=true} tag:       Include the [Type ...] tag around the output.
Expression::toString = ->
   contents = if not @source?
      new Serializer(this).serialize()
   
   else if @_?.context
      use_colour = Paws.use_colour and (@_?.colour ? @_?.color ? true)
      
      magenta = '\x1b[' + '95m'
      reset   = '\x1b[' + '39m'
      [before_char, after_char] = if use_colour then [magenta, reset] else ['|', '|']
      
      before = @source.before().split("\n").slice(-3).join("\n") + before_char
      after  = after_char + @source.after().split("\n").slice(0, 3).join("\n")
      
      (before + @source.contents() + after).trim()
   
   else
      @source.contents()
   
   contents = '{'+contents+'}'
   
   if @_?.tag == no then contents else '['+(@constructor.__name__ or @constructor.name)+' '+contents+']'


module.exports =
   parse: (text, opts)->
      parser = new Parser(text, opts)
      parser.parse()
   
   serialize: (expr, opts)->
      serializer = new Serializer(expr, opts)
      serializer.serialize()
   
   Expression: Expression
   SourceRange: SourceRange
