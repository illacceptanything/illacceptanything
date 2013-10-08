#!./node_modules/.bin/coffee 
Tput = require('blessed').Tput
tput = new Tput term: process.env.TERM

Paws = require '../Library/Paws.js'

salutation = 'Paws loves you 💖 '
colour     = if tput.colors == 256 then 219 else 5 # Pink(ish.)

ii = process.stdin
oo = process.stdout


Step = ->
   if oo.isTTY
      process.on 'SIGINT', exit
      
      ii.resume()
      
   else # not oo.isTTY
      # ...


exit = ->
   # Get rid of the "^C",
   oo.write tput.cursor_left() + tput.cursor_left()
   oo.write tput.clr_eol()
   
   # ... and write out the salutation.
   salutation = '~ ' + salutation
   
   oo.write tput.cursor_right() + tput.cursor_right() + tput.cursor_right()
   oo.write csi '38;5;' + colour + 'm'
   oo.write salutation + "\n"
   
   process.exit 0

csi = (text)-> '\x1b[' + text


# ----
Step on #up!
