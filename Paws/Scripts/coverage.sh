#!/usr/bin/env sh
                                                                                      set +o verbose
# Usage:
# ------
# This will run 'coffee-coverage' across the codebase, and then generate HTML coverage to file
# descriptor 3.
# 
#     npm run-script coverage
#     # ./Scripts/coverage.sh 3>./Library/coverage.html
puts() { printf %s\\n "$*" ;}

if [ "$npm_package_config_mocha_reporter" != 'mocha-lcov-reporter' ]; then
       npm_package_config_mocha_reporter='html-cov'                 ; fi


# FIXME: This should really support comma-seperated DEBUG values, as per `node-debug`:
#        https://github.com/visionmedia/debug
[ "$DEBUG" = 'Paws.js:scripts' ] && DEBUG_SCRIPTS=0
[ -n "$DEBUG_SCRIPTS" ] && puts "Script debugging enabled (in: `basename $0`)." >&2
[ -n "$DEBUG_SCRIPTS" ] && VERBOSE="${VERBOSE:-7}"

[ -n "$DEBUG_SCRIPTS" ] && puts "
   Mocha reporter:        '$npm_package_config_mocha_reporter'
   Mocha files:           '$npm_package_config_test_files'
" >&2

[ -n "$DEBUG_SCRIPTS" ] && set -o verbose


env NODE_ENV='coverage' \
./node_modules/.bin/mocha \
   --compilers 'coffee:coffee-script/register' --require 'Library/register-handlers.js' \
   --reporter "$npm_package_config_mocha_reporter" \
   "$npm_package_config_test_files" >&3
