#!./node_modules/.bin/coffee 
module.package = require '../package.json'
bluebird = require 'bluebird'
minimist = require 'minimist'
mustache = require 'mustache'
prettify = require('pretty-error').start()

path     = require 'path'
fs       = bluebird.promisifyAll require 'fs'

Paws     = require '../Library/Paws.js'
T = Paws.debugging.tput
_ = Paws.utilities._

out = process.stdout
err = process.stderr

heart = '💖 '
salutation = 'Paws loves you. Bye!'

# TODO: Ensure this works well for arguments passed to shebang-files
argf = minimist process.argv.slice(2), boolean: true
argv = argf._

sources = _([argf.e, argf.expr, argf.expression])
   .flatten().compact().map (expression)-> { from: expression, code: expression }
   .value()

Paws.info "-- Arguments: ", argv.join(' :: ')
Paws.info "-- Flags: ", argf
Paws.info "-- Sources: ", sources

choose = ->
   if (argf.help)
      return help()
   if (argf.version)
      return version()
   
   help() if _.isEmpty(argv[0]) and !sources.length
   
   switch operation = argv.shift()
      
      when 'pa', 'parse'
         readSourcesAsync(argv).then (files)->
            sources.push files...
            _.forEach sources, (source)->
               Paws.info "-- Parse-tree for '#{T.bold source.from}':"
               expr = Paws.parser.parse source.code, root: true
               out.write expr.serialize() + "\n"
      
      # FIXME: Single TAP count / output, for all Collections
      when 'ch', 'check'
         {Collection} = require '../Source/rule.coffee'
         readSourcesAsync(argv).then (files)->
            # FIXME: Promisify this a bit more.
            _.forEach files, (file)->
               if /\.rules\.yaml$/i.test file.from then rule_file file else sources.push file
            
            _.forEach sources, (source)-> rule_unit source
         
         rule_file = (source)->
            Paws.info "-- Staging rules in '#{T.bold source.from}' from the command-line ..."
            _.forEach _.values(require('yamljs').parse source.code), (book)->
               collection = Collection.from book
               
               if argf['expose-specification'] == true
                  _.forEach collection.rules, (rule)->
                     rule.body.locals.inject Paws.primitives 'specification' 
               
               collection.dispatch()
               collection.report()
               collection.complete()
         
         rule_unit = (source)->
            Paws.info "-- Staging '#{T.bold source.from}' from the command-line ..."
            root = Paws.generateRoot source.code
            
            if argf['expose-specification'] == true
               root.locals.inject Paws.primitives 'specification' 
            
            # FIXME: Rules created in libspace using the `specification` namespace will get added to
            #        the same `Collection` as the ‘root’ rules. This is fixed for YAML rulebooks,
            #        wherein we can specifically add the rulebook rules to their own collection, and
            #        then instantiate a new `Collection` for any rules created during the tests; but
            #        it's broken here. (This may not matter, as the only rulebooks actually
            #        *testing* `specification` functionality are currently, intentionally, in YAML.)
            collection = new Collection
            collection.dispatch()
            collection.report()
            
            here = new Paws.reactor.Unit
            
            # FIXME: This is a bit of a hack. Need a first-class citizen methdoology to predicate
            #        code on the completion of a Unit, *and* some better way to determine when to
            #        dispatch tests.
            here.on 'flushed', ->
               if root.complete() and here.listeners('flushed').length == 1 
                  collection.complete()
            
            here.stage root
            here.start() if argf.start == true
      
      when 'in', 'interact', 'interactive'
         Interactive = require '../Source/interactive.coffee'
         interact = new Interactive
         interact.on 'close', -> goodbye 0
         interact.start()
      
      when 'st', 'start'
         go = -> _.forEach sources, (source)->
            Paws.info "-- Staging '#{T.bold source.from}' from the command-line ..."
            root = Paws.generateRoot source.code
            
            here = new Paws.reactor.Unit
            here.stage root
            
            here.start() unless argf.start == false
         
         if _.isEmpty argv[0]
            go()
         else
            readSourcesAsync(argv).then (files)->
               sources.push files...
               go()
      
      else argv.unshift('start', operation) and choose()

process.nextTick choose


# ---- --- ---- --- ----

help = -> readFilesAsync([extra('help.mustache'), extra('figlets.mustache.asv')]).then ([template, figlets])->
   figlets = records_from figlets
   
   divider = T.invert( new Array(Math.ceil((T.columns + 1) / 2)).join('- ') )
   
   usage = divider + "\n" + _(figlets).sample() + template + divider
   #  -- standard 80-column terminal -------------------------------------------------|
   
   err.write mustache.render usage+"\n",
      heart: if Paws.use_colour() then heart else '<3'
      b: ->(text, r)-> T.bold r text
      u: ->(text, r)-> T.underline r text
      c: ->(text, r)-> if Paws.use_colour() then T.invert r text else '`'+r(text)+'`'
      
      op:   ->(text, r)-> T.fg 2, r text
      bgop: ->(text, r)-> T.bg 2, r text
      flag: ->(text, r)-> T.fg 6, r text
      bgflag: ->(text, r)-> T.bg 6, r text
      
      title: ->(text, r)-> T.bold T.underline r text
      link:  ->(text, r)->
         if Paws.use_colour() then T.sgr(34) + T.underline(r text) + T.sgr(39) else r text
      prompt: -> T.bg 7,'$'
      pre:  ->(text, r)->
         lines = text.split "\n"
         lines = _(lines).map (line)->
            line = r line
            sgr_sanitized_line = line.replace /\033.*?m/g, ''
            spacing = new Array(T.columns - sgr_sanitized_line.length - 4).join(' ')
            '  ' + (if Paws.use_colour() then T.invert T.fg 10, ' ' + line + spacing else line) + '  '
         lines.join("\n")
   
   version()

version = ->
   # TODO: Extract this `git describe`-style, platform-independant?
   release      = module.package['version'].split('.')[0]
   release_name = module.package['version-name']
   err.write """
      Paws.js release #{release}, “#{release_name}”
         conforming to: Paws' Nucleus 10 (ish.)
   """ + "\n"
   process.exit 1

goodbye = (code = 0)->
   length = salutation.length + 3
   if Paws.use_colour()
      # Get rid of the "^C",
      err.write T.cursor_left() + T.cursor_left()
      err.write T.clr_eol()
      
      err.write T.column_address(T.columns - 1 - length - 2)
      err.write T.enter_blink_mode() unless process.env['NOBLINK']
   
   salutation = '~ '+salutation+' '+ (if Paws.use_colour() then heart else '<3') + "\n"
   err.write if T.colors == 256 then T.xfg 219, salutation else T.fg 5, salutation
   
   process.exit code

process.on 'SIGINT', -> goodbye 255

# TODO: More robust file resolution
readFilesAsync = (files)->
   bluebird.map files, (file)-> fs.readFileAsync file, 'utf8'

readSourcesAsync = (files)->
   bluebird.map files, (file)->
      fs.readFileAsync file, 'utf8'
      .then (source)-> from: file, code: source

extra = (extra)-> path.join __dirname, 'Extras', extra

records_from = (asv)->
   # Using ASCII-delimited records: http://ell.io/i10pCz
   record_seperator = String.fromCharCode 30
   asv.split record_seperator


# ---- --- ---- --- ----

prettify.skipNodeFiles()
bluebird.onPossiblyUnhandledRejection (error)->
   console.error prettify.render error
   process.exit 1
