#!./node_modules/.bin/coffee 
module.package = require '../package.json'
bluebird = require 'bluebird'
minimist = require 'minimist'
mustache = require 'mustache'

fs = bluebird.promisifyAll require 'fs'

prettify = require('pretty-error').start ->
   Paws = require '../Library/Paws.js'
   T = Paws.debugging.tput
   _ = Paws.utilities._
   
   out = process.stdout
   err = process.stderr
   
   heart = '💖 '
   salutation = 'Paws loves you. Bye!'
   
   help = ->
      seperator = T.invert( new Array(Math.ceil((T.columns + 1) / 2)).join('- ') )
      
      #  -- standard 80-column terminal ------------------------------------------------|
      usage = seperator + "\n" + _(figlets).sample() + """
         
{{#title}}Usage:{{/title}}
{{#pre}}{{prompt}} paws.js [{{#bgflag}}flags{{/bgflag}}] {{#bgop}}operation{{/bgop}} [operation parameters]
         
           # for example,
         {{prompt}} paws.js {{#u}}foo.paws{{/u}} [[--] arguments]
         {{prompt}} paws.js {{#bgop}}interact{{/bgop}}
         {{prompt}} paws.js {{#bgop}}parse{{/bgop}} {{#u}}bar.paws{{/u}}{{/pre}}
         
         The first non-flag argument will be an operation to preform; the second depends
         on the operation, but is usually a path to a file to load. If no operation is
         instructed, then the {{#op}}start{{/op}} operation is used.
         
         In true UNIX style, single-letter flags may be combined serially; that is,
         {{#c}}-aBc{{/c}} is synonymous with {{#c}}-a -B -c{{/c}}. Furthermore, all arguments following a
         bare {{#c}}--{{/c}} are passed to the program un-parsed (that is, even if it's a flag that
         would otherwise be interpreted by the Paws.js CLI, it will be ignored and passed
         verbatim to the Paws runtime.)
         
{{#title}}Operations:{{/title}}
            {{#op}}start{{/op}} {{#u}}filename.paws{{/u}}           Start the Paws reactor, load the given unit
            {{#op}}parse{{/op}} {{#u}}filename.paws{{/u}}           Show the computed parse-tree for a cPaws file
            {{#op}}interact{{/op}}                      Begin an interactive Paws session (a ‘repl’)
         
{{#title}}Flags:{{/title}}
{{#flag}}   --help{{/flag}}:        Show this usage information
{{#flag}}   --version{{/flag}}:     Show version information
            
{{#flag}}   -e "EXPR"{{/flag}},{{#flag}} --expression="EXPR"{{/flag}}:
               For {{#op}}parse{{/op}} and {{#op}}start{{/op}}, allows you to provide a cPaws expression at the
               command-line, to substitute for a file. (If at least one {{#c}}--expression{{/c}}
               is included, the filname for those operations, normally a required
               parameter, may be omitted.)
            
{{#flag}}   --[no-]start{{/flag}}:
               Disable the self-scheduling reactor functionality. Under normal
               circumstances (i.e. {{#c}}--start{{/c}}, the default) a Paws reactor runs
               indefinately, processing combinations when they become available.
               
               With the {{#c}}--no-start{{/c}} flag, however, the reactor will shut down as soon
               as all immediately-known stagings have been completed. This means your
               program will more intuitively ‘automatically exit,’ but it also means any
               deferred stagings in your code (stagings that cannot immediately execute,
               for instance those with ownership-conflicts, or those deferred by timing
               primitives) may not complete before the reactor shuts down.
         
         
         Paws.js also accepts several environment variables, in the form:
{{#pre}}{{prompt}} VAR=value paws.js ...{{/pre}}
         
{{#title}}Variables:{{/title}}
            {{#b}}SILENT{{/b}}=[true|false]           Suppress all output from Paws.js itself
            {{#b}}VERBOSE{{/b}}=[0-10]                Manually adjust the logging level [default: {{#b}}4{{/b}}]
            {{#b}}COLOUR{{/b}}=[true|false]           Disable coloured output [default: {{#b}}true{{/b}}]
         
         Paws homepage: <{{#link}}http://Paws.mu{{/link}}>
         Report bugs:   <{{#link}}https://github.com/ELLIOTTCABLE/Paws.js/issues{{/link}}>
         
         Say hi:        <{{#link}}http://twitter.com/ELLIOTTCABLE{{/link}}>
                     or <{{#link}}http://ell.io/IRC{{/link}}> ({{#b}}#ELLIOTTCABLE{{/b}} on the Freenode IRC network)
         
         
      """ + seperator
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
      bluebird.map files, (file)->
         fs.readFileAsync(file, 'utf8').then (source)-> { from: file, code: source }
   
# ---- --- ---- --- ----
   
   # TODO: Ensure this works well for arguments passed to shebang-files
   argf = minimist process.argv.slice 2
   argv = argf._
   
   if (argf.help)
      return help()
   if (argf.version)
      return version()
   
   sources = _([argf.e, argf.expr, argf.expression])
      .flatten().compact().map (expression)-> { from: expression, code: expression }
      .value()
   
   help() if _.isEmpty(argv[0]) and !sources.length
   
   choose = -> switch operation = argv.shift()
      
      when 'pa', 'parse'
         readFilesAsync(argv).then (files)->
            sources.push files...
            _.forEach sources, (source)->
               Paws.info "-- Parse-tree for '#{T.bold source.from}':"
               expr = Paws.parse Paws.parse.prepare source.code
               out.write expr.serialize() + "\n"
      
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
            readFilesAsync(argv).then (files)->
               sources.push files...
               go()
      
      else argv.unshift('start', operation) and choose()
   
   choose()

prettify.skipNodeFiles()
bluebird.onPossiblyUnhandledRejection (error)->
   console.error prettify.render error
   process.exit 1


# ---- --- ---- --- ----

figlets = [ """
Hi! My name's
__________                               __        
\\______   \\_____ __  _  ________        |__| ______
 |     ___/\\__  \\\\ \\/ \\/ /  ___/        |  |/  ___/
 |    |     / __ \\\\     /\\___ \\         |  |\\___ \\ 
 |____|    (____  /\\\/\\_//____  > /\\ /\\__|  /____  >
                \\\/           \\\/  \\\/ \\______|    \\/ 
                                    ... and I love you lots. {{{heart}}}
""", """
Hi! My name's
  _____                     _     
 |  __ \\                   (_)    
 | |__) |_ ___      _____   _ ___ 
 |  ___/ _` \\ \\ /\\ / / __| | / __|
 | |  | (_| |\\ V  V /\\__ \\_| \\__ \\
 |_|   \\__,_| \\_/\\_/ |___(_) |___/  ... and I love you lots. {{{heart}}}
                          _/ |
                         |__/
""", """
Hi! My name's
██████╗  █████╗ ██╗    ██╗███████╗        ██╗███████╗
██╔══██╗██╔══██╗██║    ██║██╔════╝        ██║██╔════╝
██████╔╝███████║██║ █╗ ██║███████╗        ██║███████╗
██╔═══╝ ██╔══██║██║███╗██║╚════██║   ██   ██║╚════██║
██║     ██║  ██║╚███╔███╔╝███████║██╗╚█████╔╝███████║
╚═╝     ╚═╝  ╚═╝ ╚══╝╚══╝ ╚══════╝╚═╝ ╚════╝ ╚══════╝
                                    ... and I love you lots. {{{heart}}}
""", """
Hi! My name's
   ___                      _
  / _ \\___ __    _____     (_)
 / ___/ _ `/ |/|/ (_-<_   / (_-<
/_/   \\_,_/|__,__/___(_)_/ /___/  ... and I love you lots. {{{heart}}}
                      |___/
""", """
Hi! My name's
 ______   ______     __     __     ______       __     ______    
/\\  == \\ /\\  __ \\   /\\ \\  _ \\ \\   /\\  ___\\     /\\ \\   /\\  ___\\   
\\ \\  _-/ \\ \\  __ \\  \\ \\ \\/ ".\\ \\  \\ \\___  \\   _\\_\\ \\  \\ \\___  \\  
 \\ \\_\\    \\ \\_\\ \\_\\  \\ \\__/".~\\_\\  \\/\\_____\\ /\\_____\\  \\/\\_____\\ 
  \\/_/     \\/_/\\/_/   \\/_/   \\/_/   \\/_____/ \\/_____/   \\/_____/ 
                                    ... and I love you lots. {{{heart}}}
""","""
Hi! My name's
 ██▓███   ▄▄▄       █     █░  ██████       ▄▄▄██▀▀▀██████ 
▓██░  ██▒▒████▄    ▓█░ █ ░█░▒██    ▒         ▒██ ▒██    ▒ 
▓██░ ██▓▒▒██  ▀█▄  ▒█░ █ ░█ ░ ▓██▄           ░██ ░ ▓██▄   
▒██▄█▓▒ ▒░██▄▄▄▄██ ░█░ █ ░█   ▒   ██▒     ▓██▄██▓  ▒   ██▒
▒██▒ ░  ░ ▓█   ▓██▒░░██▒██▓ ▒██████▒▒ ██▓  ▓███▒ ▒██████▒▒
▒▓▒░ ░  ░ ▒▒   ▓▒█░░ ▓░▒ ▒  ▒ ▒▓▒ ▒ ░ ▒▓▒  ▒▓▒▒░ ▒ ▒▓▒ ▒ ░
░▒ ░       ▒   ▒▒ ░  ▒ ░ ░  ░ ░▒  ░ ░ ░▒   ▒ ░▒░ ░ ░▒  ░ ░
░░         ░   ▒     ░   ░  ░  ░  ░   ░    ░ ░ ░ ░  ░  ░  
               ░  ░    ░          ░    ░   ░   ░       ░  
                                       ░  ... and I love you lots. {{{heart}}}
""","""
Hi! My name's
╔═╗┌─┐┬ ┬┌─┐  ┬┌─┐
╠═╝├─┤│││└─┐  │└─┐
╩  ┴ ┴└┴┘└─┘o└┘└─┘  ... and I love you lots. {{{heart}}}

""","""
Hi! My name's
 _|_|_|                                                _|            
 _|    _|    _|_|_|  _|      _|      _|    _|_|_|            _|_|_|  
 _|_|_|    _|    _|  _|      _|      _|  _|_|          _|  _|_|      
 _|        _|    _|    _|  _|  _|  _|        _|_|      _|      _|_|  
 _|          _|_|_|      _|      _|      _|_|_|    _|  _|  _|_|_|    
                                                       _|            
                                                     _|              
                                    ... and I love you lots. {{{heart}}}
""","""
Hi! My name's
 ______                          __        
|   __ \\.---.-.--.--.--.-----.  |__|.-----.
|    __/|  _  |  |  |  |__ --|__|  ||__ --|
|___|   |___._|________|_____|__|  ||_____|  ... and I love you lots. {{{heart}}}
                               |___|       
""","""
Hi! My name's
    _____                         
   (, /   )                ,    
    _/__ / _  _   _ _        _  
    /     (_(_(_(/ /_)_ o /_/_)_  ... and I love you lots. {{{heart}}}
 ) /                   .-/      
(_/                   (_/       

"""]
