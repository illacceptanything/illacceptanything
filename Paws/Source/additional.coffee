`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
# This file includes some additional APIs, both internal and external, that we expose on the `Paws`
# exports-object. It's called as the first action in `Paws.coffee`.
util = require 'util'

module.exports = additional =
   
   debugging: debugging = new class Debugging
      use_colour = true # Default
      verbosity = 4 # Default
      environment_verbosity = Infinity
      
      constructor: ->
         # FIXME: This `require` will break browserify.
         @tput = new (require('blessed').Tput) term: process.env['TERM']

      # This is an exposed, bi-directional mapping of verbosity-names:
      # 
      #     Paws.verbosities[4] === Paws.verbosities['warning']
      verbosities: verbosities =
         #    0       1      2       3      4      5     6     7      8     9
         "emergency alert critical error warning notice info debug verbose wtf".split(' ')
      verbosities[name] = minimum for name, minimum in verbosities
      
      
      write_browser = (console?.error || console?.log || noop).bind console
      write_cli = (objects...)->
         output = util.format.apply(util, arguments) + '\n'
         process.stderr.write output, 'utf8'
      
      # FIXME: Temporary. I'd really like this to be external, and more robust.
      write: write = if window? or process?.browser?
         write_browser
      else
         write_cli
      
      debug: -> null # NYI
         
      inject: (exports)->
         exports.verbosity  = -> verbosity
         exports.is_silent  = -> verbosity == 0
         
         exports.use_colour = -> use_colour
         
         for name, v in verbosities
            exports[name] = do (name, v)->-> if v <= verbosity 
               
               if verbosity > 9 or process.env.DEBUG_VERBOSITY
                  write "-- Verbosity of #{v}/#{verbosities[v]} ≤ "+
                      "#{verbosity}/#{verbosities[verbosity] ? '???'}; printing message:"
               
               write.apply debugging, arguments
         
         variables = silent: 0, quiet: 2, verbose: 8, WTF: 9
         # We configure the `verbosity` itself in one of two ways: by calling an internal API at
         # runtime; or by setting an environment-variable before entry to the Paws library. (The
         # latter cannot affect the verbosity after the execution of this file.)
         # 
         #     Paws.VERBOSE(9) // Very verbose output
         #     // (or `VERBOSE=9 ./program.paws`)
         #     
         #     Paws.QUIET()    // Silence errors
         #     // (or `QUIET=true ./program.paws`)
         # 
         # NOTE: Quieter-overrides-louder, in environment variables. For instance, if `SILENT` is
         #       defined, that overrides `VERBOSE`. To boot, the verbosity cannot be set *higher*
         #       via the API, if it has already been set by an environment variable (meaning
         #       `SILENT` will always mean `SILENT`.)
         # ----
         # TODO: Verify that this is compatible all the way back to IE6. I'm a bit iffy about the
         #       isFinite() shit.
         for own name, ddefault of variables
            name = name.toUpperCase()
            exports[name] = do (name, ddefault)-> (level = true, opts = {environmental: no})->
               if isFinite (l = parseInt level, 10)
                  verbosity = l
               else if level == true or
                       level.charAt?(0) == 'y' or # yes
                       level.charAt?(0) == 't'    # true
                  verbosity = ddefault unless environment_verbosity < ddefault # Silence reigns.
               
               environment_verbosity = verbosity if opts.environmental
               
               exports.wtf "-- Verbosity set to: #{verbosity}/#{verbosities[verbosity] ? '???'}"
            
            if process.env[name] and not environment_verbosity
               exports[name](process.env[name], environmental: yes) 
         
         exports.colour = exports.color = (use = true)->
            if use == no or
               use.charAt?(0) == 'n' or # no
               use.charAt?(0) == 'f'    # false
             use_colour = no
            else
             use_colour = yes
         
         if env_colour = process.env['COLOUR'] ? process.env['COLOR']
            exports.colour env_colour
