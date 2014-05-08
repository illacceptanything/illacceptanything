#!./node_modules/.bin/coffee 
module.package = require '../package.json'
minimist = require 'minimist'
mustache = require 'mustache'

prettify = require('pretty-error').start ->
   Paws = require '../Library/Paws.js'
   T = Paws.debugging.tput
   _ = Paws.utilities._
   
   out = process.stdout
   err = process.stderr

   heart = '💖 '
   salutation = 'Paws loves you. Bye!'
   colour     = if T.colors == 256 then 219 else 5 # Pink(ish.)
   
   help = ->
      process.removeListener 'exit', exit
      
      #  -- stanard 80-column terminal -------------------------------------------------|
      usage = "\n" + _(figlets).sample() + """
         
         {{#title}}Usages:{{/title}}
{{#code}}   > paws.js [FLAGS] [operation] [file]
            > paws.js {{#u}}file.paws{{/u}} [--] [arguments]
            > paws.js {{#b}}interact{{/b}}
            > paws.js {{#b}}parse{{/b}} {{#u}}file.paws{{/u}}{{/code}}
         
         The first non-flag argument will be an operation to preform; the second depends
         on the operation, but is usually a path to a file to load. If no operation is
         instructed, then the `start` operation is used.
         
         {{#title}}Operations:{{/title}}
            [<none>|{{#b}}start{{/b}}] {{#u}}file.paws{{/u}}      Start the Paws reactor, load the given file
            {{#b}}interact{{/b}}                      Begin an interactive Paws session (a ‘repl’)
            {{#b}}parse{{/b}} {{#u}}file.paws{{/u}}               Show the computed parse-tree for a cPaws file
         
         {{#title}}Flags:{{/title}}
            --help:        Show usage information
            --version:     Show version information
         
         Paws.js also accepts several environment variables, in the form:
         {{#code}}   > VAR=value paws.js ...{{/code}}
         
         {{#title}}Variables:{{/title}}
            {{#b}}SILENT{{/b}}=[true|false]           Suppress all output from Paws.js itself
            {{#b}}VERBOSE{{/b}}=[0-10]                Manually adjust the logging level [default: {{#b}}4{{/b}}]
            {{#b}}COLOUR{{/b}}=[true|false]           Disable coloured output [default: {{#b}}true{{/b}}]
         
         Paws homepage: <{{#link}}http://Paws.mu{{/link}}>
         Report bugs:   <{{#link}}https://github.com/Paws/Paws.js/issues{{/link}}>
         Say hi:        <{{#link}}http://twitter.com/ELLIOTTCABLE{{/link}}>
         
      """
      #  -- standard 80-column terminal -------------------------------------------------|
      
      err.write mustache.render usage+"\n",
         heart: if Paws.use_colour() then heart else '<3'
         b: ->(text, r)-> T.bold r text
         u: ->(text, r)-> T.underline r text
         
         title: ->(text, r)-> T.bold T.underline r text
         link:  ->(text, r)->
            if Paws.use_colour() then T.sgr(34) + T.underline(r text) + T.sgr(39) else r text
         code:  ->(text, r)->
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
         out.write T.cursor_left() + T.cursor_left()
         out.write T.clr_eol()
         
         out.write T.cursor_right() + T.cursor_right() + T.cursor_right()
         out.write T.enter_blink_mode()
         out.write T.sgr 38, 5, colour
      
      salutation = '~ ' + salutation + (if Paws.use_colour() then heart else '<3') + "\n"
      out.write salutation
   
   process.on 'exit', exit
   
   
# ---- --- ---- --- ----
   
   # TODO: Ensure this works well for arguments passed to shebang-files
   argv = minimist process.argv.slice 2
   
   if (argv.help)
      return help()
   if (argv.version)
      return version()
   
   help()

prettify.skipNodeFiles()


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
