#!/usr/bin/env sh
                                                                                      set +o verbose
# Usage:
# ------
# This preforms the various compilation / build steps necessary to use this implementation. It's
# automatically run on `npm install`.
# 
#     npm run-script build

# FIXME: The existence of this script is very problematic. This basically means Windows users
#        (especially those using PowerShell instead of, say, cygwin) cannot clone-and-immediately-
#        install. I need to figure out the dualistic shell-script/batch-file trick, but so far, I've
#        been unable to wrangle it.

printf "%s\n%s"                                                \
   '#!/usr/bin/env node'                                       \
   "$(coffee -cbp --no-header ./Executables/paws.js.coffee)"   \
      > ./Executables/paws.js

pegjs 'Source/cPaws.pegjs' 'Library/cPaws-parser.js'
