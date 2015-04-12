Paws = require './Paws.coffee'
Paws.infect global
T = Paws.debugging.tput

PrettyError = require('pretty-error')

{EventEmitter} = require 'events'
readline = require 'readline'
readline.vim = try require 'readline-vim' catch error
                  throw error unless error.code == 'MODULE_NOT_FOUND'


module.exports = Interactive =
parameterizable class Interactive extends EventEmitter
   
   constructor: ->
      # XXX: Not sure if I can create the readline instance early, before I use it. This may need to
      #      be moved to start().
      @readline = readline.createInterface
         input: @_?.input ? process.stdin, output: @_?.output ? process.stdout
      @readline.setPrompt @_?.prompt ? ':: '
      @readline.line_style = T.sgr 7
      @readline.clear_style = T.sgr 27
      @hackReadline()
      
      @error_renderer = @_?.error_renderer
      unless @error_renderer?
         @error_renderer = new PrettyError()
         @error_renderer.skipNodeFiles()
      
      # TODO: Inject aliens.
      @here = new reactor.Unit
      @shared_locals = (Paws.generateRoot()).locals
      
      inspector = new Native (result)->
         process.stdout.write Paws.inspect(result) + "\n"
      .rename '<interact: inspect result>'
      @shared_locals.push Thing.pair '_INTERACTIVE_INSPECT', inspector
   
   prompt: -> @readline.prompt()
   
   start: ->
      process.title = 'paws.js (interact)'
      @here.start()
      
      shortcircuit = undefined # ???
      @readline.on 'line', (line)=>
         return shortcircuit = false if shortcircuit # ???
         return @readline.prompt() unless line.length
         @readline.output.write @readline.clear_style
         
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
            @readline.output.write "\n"
            @prompt()
      @readline.on 'SIGINT', SIGINT
      process  .on 'SIGINT', SIGINT
      
      SIGTERM = =>
         @here.stop()
         @readline.output.write @readline.clear_style
         @readline.close()
         process.stdin.destroy()
         @emit 'close'
      @readline.on 'close', SIGTERM
      process.on 'SIGTERM', SIGTERM
      
      SIGTSTP = =>
         continued = false
         
         SIGCONT = =>
            return if continued
            continued = true
            @readline.input.pause()
            @readline.input.resume()
            @readline._setRawMode true
            @readline._refreshLine()
         
         process.once 'SIGCONT', SIGCONT
         process.nextTick SIGCONT
         
         @readline.output.write @readline.clear_style
         @readline._setRawMode false
         process.kill process.pid, 'SIGTSTP'
         
      @readline.on 'SIGTSTP', SIGTSTP
      
      Paws.alert "Successive lines will be evaluated as executions, with shared `locals`."
      Paws.alert "   (#{T.bold '⌃d'} to close the input-stream; "+
                     "#{T.bold '⌃c'} to synchronously force new prompt)"
      @prompt()
   
   
   evaluate: (code)->
      @mutex = new Thing
      
      # We generate a wrapper-Expression for the input, turning it into:
      # 
      #     _INTERACTIVE_INSPECT (expr)
      if code instanceof parse.Expression
         expr = parse.Sequence.from [new Label '_INTERACTIVE_INSPECT', code]
      else
         expr = parse '_INTERACTIVE_INSPECT ['+code+']'
      
     #Paws.info "-- Generated expression to evaluate: " +
     #   expr.with(context: yes, tag: no).toString()
      
      # Now, we put both those in the queue, giving the first responsibility for the mutex. This
      # prevents the resumer from realizing until the interact-line has become complete(), and thus
      # had its responsibility invalidated.
      execution = Paws.generateRoot expr
      execution.locals = @shared_locals
      execution.rename '<interact: interactive input>'
      
      @here.stage execution, undefined, new reactor.Mask @mutex
      @here.stage @generateResumer(), undefined, new reactor.Mask @mutex
      
      return execution
   
   
   # Generates an `Execution` that will clean up the `mutex` and then print the next prompt.
   generateResumer: -> new Native =>
      @mutex = undefined
      @prompt()
   .rename '<interact: resume prompt>'
   
   
   # --- ---- --- /!\ --- ---- --- #
   
   # This is all a huge, fragile, horrible, monkey-patching hack.
   hackReadline: ->
      exportz = readline
      
      _interface = @readline
      _interface._refreshLine = ->
         input = @_prompt + @line
         lastLineLength = input.length % @columns
         rows = (input.length - lastLineLength) / @columns
         
         cursorPos = @_getCursorPos()
         
         # No idea what function this is supposed to preform.
         prevRows = @prevRows || 0;
         if prevRows > 0
            exportz.moveCursor @output, 0, -prevRows
         
         exportz.cursorTo @output, 0
         exportz.clearScreenDown @output
         
         printable = input + new Array(@columns - lastLineLength).join ' '
         @output.write T.block printable, (line)->
            _interface.line_style + line
         
         if lastLineLength < @columns
            @output.write ' '
         
         exportz.cursorTo @output, cursorPos.cols
         
         diff = rows - cursorPos.rows
         if diff > 0
            exportz.moveCursor @output, 0, -diff
         
         @prevRows = cursorPos.rows
      
      
      # These replace the usual behavior of pausing the input-stream (which means all input while
      # paused is buffered, and will eventually be dumped back out), and simply *ignores* all input
      # until unpaused. No buffering.
      _interface.pause = ->
         return if @paused
         @_ttyWrite = paused_ttyWrite
         @paused = true
         @emit 'pause'
         return this
      
      _interface.resume = ->
         return unless @paused
         @_ttyWrite = active_ttyWrite
         @paused = false
         @emit 'resume'
         return this
      
      active_ttyWrite = (s, key = {})->
         if key.ctrl and key.name == 'd' then @close()
         else _ttyWrite.apply this, arguments
      
      paused_ttyWrite = (s, key = {})->
         if key.ctrl
            switch key.name
               when 'c' then @emit 'SIGINT'     # ^c (interrupt)
               when 'd' then @close()           # ^d (EOF)
               when 'z'                         # ^z (process backgrounding)
                  return if process.platform == 'win32'
                  @emit 'SIGTSTP'

      [_ttyWrite, _interface._ttyWrite] = [_interface._ttyWrite, active_ttyWrite]
