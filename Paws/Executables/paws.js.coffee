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
      process.removeListener 'exit', exit
      
      #  -- stanard 80-column terminal -------------------------------------------------|
      usage = "\n" + _(figlets).sample() + """
         
{{#title}}Usages:{{/title}}
{{#pre}}   > paws.js [{{#flag}}flags{{/flag}}] {{#op}}operation{{/op}} [params]
            > paws.js {{#u}}file.paws{{/u}} [--] [arguments]
            > paws.js {{#op}}interact{{/op}}
            > paws.js {{#op}}parse{{/op}} {{#u}}file.paws{{/u}}{{/pre}}
         
         The first non-flag argument will be an operation to preform; the second depends
         on the operation, but is usually a path to a file to load. If no operation is
         instructed, then the `start` operation is used.
         
{{#title}}Operations:{{/title}}
            [<none>|{{#op}}start{{/op}}] {{#u}}file.paws{{/u}}      Start the Paws reactor, load the given file
            {{#op}}interact{{/op}}                      Begin an interactive Paws session (a ‘repl’)
            {{#op}}parse{{/op}} {{#u}}file.paws{{/u}}               Show the computed parse-tree for a cPaws file
         
{{#title}}Flags:{{/title}}
{{#flag}}   --help{{/flag}}:        Show usage information
{{#flag}}   --version{{/flag}}:     Show version information
            
{{#flag}}   -e "EXPR", --expression="EXPR"{{/flag}}:
               For {{#b}}parse{{/b}} and {{#b}}start{{/b}}, allows you to provide a cPaws expression at the
               command-line, to substitute for a file. (If at least one {{#c}}--expression{{/c}} is
               included, the required filname for those operations may be omitted.)
         
         
         Paws.js also accepts several environment variables, in the form:
{{#pre}}   > VAR=value paws.js ...{{/pre}}
         
{{#title}}Variables:{{/title}}
            {{#b}}SILENT{{/b}}=[true|false]           Suppress all output from Paws.js itself
            {{#b}}VERBOSE{{/b}}=[0-10]                Manually adjust the logging level [default: {{#b}}4{{/b}}]
            {{#b}}COLOUR{{/b}}=[true|false]           Disable coloured output [default: {{#b}}true{{/b}}]
         
         Paws homepage: <{{#link}}http://Paws.mu{{/link}}>
         Report bugs:   <{{#link}}https://github.com/ELLIOTTCABLE/Paws.js/issues{{/link}}>
         
         Say hi:        <{{#link}}http://twitter.com/ELLIOTTCABLE{{/link}}>
                     or <{{#link}}http://ell.io/IRC{{/link}}> ({{#b}}#ELLIOTTCABLE{{/b}} on the Freenode IRC network)
         
      """
      #  -- standard 80-column terminal -------------------------------------------------|
      
      err.write mustache.render usage+"\n",
         heart: if Paws.use_colour() then heart else '<3'
         b: ->(text, r)-> T.bold r text
         u: ->(text, r)-> T.underline r text
         c: ->(text, r)-> T.invert r text
         
         op:   ->(text, r)-> T.fg 2, r text
         flag: ->(text, r)-> T.fg 6, r text
         
         title: ->(text, r)-> T.bold T.underline r text
         link:  ->(text, r)->
            if Paws.use_colour() then T.sgr(34) + T.underline(r text) + T.sgr(39) else r text
         pre:  ->(text, r)->
            if Paws.use_colour()
               lines = text.split "\n"
               lines = _(lines).map (line)->
                  sanitized_line = line.replace /\{\{\{?[^}]+\}?\}\}/g, ''
                  line + new Array(T.columns - sanitized_line.length + 1).join(' ')
               T.invert r lines.join("\n")
            else r text
      
      version()
   
   version = ->
      err.write """
         Paws.js version #{module.package.version} (Paws 10p)
      """ + "\n"
      process.exit 1
   
   exit = ->
      if Paws.use_colour()
         # Get rid of the "^C",
         err.write T.cursor_left() + T.cursor_left()
         err.write T.clr_eol()
         
         err.write T.cursor_right() + T.cursor_right() + T.cursor_right()
         err.write T.enter_blink_mode() unless process.env['NOBLINK']
      
      salutation = '~ '+salutation+' '+ (if Paws.use_colour() then heart else '<3') + "\n"
      err.write if T.colors == 256 then T.xfg 219, salutation else T.fg 5, salutation
      
      process.exit 0
   
   process.on 'exit', exit
   process.on 'SIGINT', -> process.exit 0
   
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
   
   help() if _.isEmpty argv[0]
   choose = -> switch operation = argv.shift()
      
      when 'pa', 'parse'
         readFilesAsync(argv).then (files)->
            sources.push files...
            _.forEach sources, (source)->
               Paws.info "-- Parse-tree for '#{T.bold source.from}':"
               expr = Paws.parser.parse source.code, root: true
               out.write expr.serialize() + "\n"
      
      when 'st', 'start'
         readFilesAsync(argv).then (files)->
            sources.push files...
            _.forEach sources, (source)->
               Paws.info "-- Staging '#{T.bold source.from}' from the command-line ..."
               expr = Paws.parser.parse source.code, root: true
               
               here = new Paws.reactor.Unit
               here.stage new Execution expr
               
               here.start() unless argf.start == false
      
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
