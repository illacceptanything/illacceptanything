`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
Paws = require './Paws.coffee'
Paws.infect global

PrettyError = require('pretty-error')

readline = require 'readline'
readline.vim = try require 'readline-vim' catch error
                  throw error unless error.code == 'MODULE_NOT_FOUND'


module.exports = Interactive =
parameterizable class Interactive
   
   constructor: ->
      # XXX: Not sure if I can create the readline instance early, before I use it. This may need to
      #      be moved to start().
      @readline = readline.createInterface
         input: @_?.input ? process.stdin, output: @_?.output ? process.stdout
      
      @readline.setPrompt @_?.prompt ? ':: '
      
      @error_renderer = @_?.error_renderer
      unless @error_renderer?
         @error_renderer = new PrettyError()
         @error_renderer.skipNodeFiles()
      
      # TODO: Inject aliens.
      @here = new reactor.Unit
      @shared_locals = (new Execution).locals
      
      inspector = new Alien (result)->
         process.stdout.write Paws.inspect(result) + "\n"
      .rename '<interact: inspect result>'
      @shared_locals.push Thing.pair '<inspect>', inspector
   
   prompt: -> @readline.prompt()
   
   
   start: ->
      #process.title = 'paws.js: interact'
      @here.start()
      
      shortcircuit = undefined # ???
      @readline.on 'line', (line)=>
         return shortcircuit = false if shortcircuit # ???
         return @readline.prompt() unless line.length
         
         # FIXME: Input during the processing is currently all processed immediately after a prompt
         #        is next shown. This is rather icky when the user ^C's a ton, and then externally
         #        emits a `SIGINT`.
         @readline.pause()
         
         try
            @evaluate line
         catch err
            # TODO: ‘theme’ this to be a bit less verbose
            Paws.error @error_renderer.render(err)
            @prompt()
      
      process.removeAllListeners('SIGINT') # FIXME: This is a bad idea.
      SIGINT = => process.nextTick =>
         if @mutex then @here.table.remove mask: new reactor.Mask @mutex
         else
            shortcircuit = true # ???
            @readline.write "\n"
            @prompt()
      @readline.on 'SIGINT', SIGINT
      process  .on 'SIGINT', SIGINT
      
      SIGTERM = =>
         @here.stop()
         @readline.write "\x1b[2K\x1b[0G" # Zero cursor.
         process.stdin.destroy()
      @readline.on 'close', SIGTERM
      process.on 'SIGTERM', SIGTERM
      
      t = Paws.debugging.tput
      Paws.alert "Successive lines will be evaluated as executions, with shared `locals`."
      Paws.alert "   (#{t.bold '⌃d'} to close the input-stream; "+
                     "#{t.bold '⌃c'} to synchronously force new prompt)"
      @prompt()
   
   
   evaluate: (code)->
      @mutex = new Thing
      
      # We generate a wrapper-Expression for the input, turning it into:
      # 
      #     <inspector> (expr)
      if code instanceof parser.Expression
         inspector = new parser.Expression new Label '<inspector>'
         superexpr = new parser.Expression code
         inspector.append superexpr
         expr = inspector
      else
         expr = parser.parse '<inspect> ('+code+')'
      
      Paws.info "-- Generated expression to evaluate: " +
         expr.with(context: yes, tag: no).toString()
      
      # Now, we put both those in the queue, giving the first ownership of the mutex. This prevents
      # the resumer from realizing until the interact-line has become complete(), and thus had its
      # ownership invalidated.
      execution = new Execution expr
      execution.locals = @shared_locals
      execution.rename '<interact: interactive input>'
      
      @here.stage execution, null, new reactor.Mask @mutex
      @here.stage @generateResumer(), null, new reactor.Mask @mutex
      
      return execution
   
   
   # Generates an `Execution` that will clean up the `mutex` and then print the next prompt.
   generateResumer: -> new Alien =>
      @mutex = undefined
      @prompt()
   .rename '<interact: resume prompt>'
   
   # Generates an `Execution` that will print information about the `Thing` passed to it
   # (presumably, the final result of a line of code typed into the `Interactive`.)
