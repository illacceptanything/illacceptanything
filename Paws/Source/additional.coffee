# This file includes some additional APIs, both internal and external, that we expose on the `Paws`
# exports-object. It's called as the first action in `Paws.coffee`.
verbosity = 4

module.exports = (Paws)->
   
   # TODO: This could easily be reduced to a module.
   variables = silent: 0, quiet: 2, verbose: 8, WTF: 9
   
   Paws.verbosity = -> verbosity
   
   # This is an exposed, bi-directional mapping of verbosity-names:
   # 
   #     Paws.verbosities[4] === Paws.verbosities['warning']
   Paws.verbosities = verbos =
      #    0       1      2       3      4      5     6     7      8     9
      "emergency alert critical error warning notice info debug verbose wtf".split(' ')
   Paws.verbosities[name] = minimum for name, minimum in Paws.verbosities
   
   # First off, we expose a JavaScript API to report an error.
   # ----
   # FIXME: This API is temporary. I need to replace it with something a little more robust than
   #        a straight console.log
   # FIXME: Replace the '-- ' in these direct invocations with whatever *actual* debugging
   #        mechanism we end up using
   # XXX: WHY DOES `verbosity < maximum` HAVE TO BE BACKWARDS D:
   for name, v in Paws.verbosities
      Paws[name] = do (name, v)->-> if v <= verbosity 
         out = process?.stderr?.write.bind(process.stderr) || console.log.bind(console)
         
         if verbosity >= 9 or process.env.DEBUG_VERBOSITY
            out "-- Verbosity of #{v}:#{verbos[v]} ≤ #{verbosity}:#{verbos[verbosity] ? '???'}; " +
                "printing message:"
         
         out.apply this, arguments
   
   # Second, we configure the `verbosity` itself in one of two ways: by calling an internal API
   # at runtime; or by setting an environment-variable before entry to the Paws library. (The
   # latter cannot affect the verbosity after the execution of this file.)
   # 
   #     Paws.VERBOSE(9) // Very verbose output
   #     // (or `VERBOSE=9 ./program.paws`)
   #     
   #     Paws.QUIET()    // Silence errors
   #     // (or `QUIET=true ./program.paws`)
   # 
   # NOTE: Quieter-overrides-louder, in environment variables. For instance, if `SILENT` is
   #       defined, that overrides `VERBOSE`.
   # ----
   # TODO: Verify that this is compatible all the way back to IE6. I'm a bit iffy about the
   #       isFinite() shit.
   for own name, ddefault of variables
      name = name.toUpperCase()
      Paws[name] = do (name, ddefault)-> (level)->
         level = true unless level?
         if isFinite (l = parseInt level, 10)
            verbosity = l
         else if level == true or
                 level.charAt?(0) == 'y' or # yes
                 level.charAt?(0) == 't'    # true
            verbosity = ddefault
         Paws.wtf "-- Verbosity set to: #{verbosity}:#{verbos[verbosity] ? '???'}"
      
      Paws[name] process.env[name] if process.env[name]?
