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
[ -n "$DEBUG_SCRIPTS" ] && VERBOSE=0

[ -n "$DEBUG_SCRIPTS" ] && puts \
   "Source dir:            '$npm_package_config_dirs_source'"           \
   "Instrumentation dir:   '$npm_package_config_dirs_instrumentation'"  \
   "Test dir:              '$npm_package_config_dirs_test'"             \
   "Mocha reporter:        '$npm_package_config_mocha_reporter'"        \
   "Mocha files:           '$npm_package_config_mocha_files'"           \
   "" >&2

[ -n "$DEBUG_SCRIPTS" ] && set -o verbose


./node_modules/.bin/coffeeCoverage --path=relative \
   --exclude 'additional.coffee' \
   "$npm_package_config_dirs_source" \
   "$npm_package_config_dirs_instrumentation" >/dev/null

./node_modules/.bin/coffee --compile \
   --output "$npm_package_config_dirs_instrumentation" \
   "$npm_package_config_dirs_source/additional.coffee"
./node_modules/.bin/coffee --compile \
   --output "$npm_package_config_dirs_test" \
   "$npm_package_config_dirs_test/support.coffee"

env NODE_ENV='coverage' \
./node_modules/.bin/mocha --compilers 'coffee:coffee-script/register' \
   --reporter "$npm_package_config_mocha_reporter" \
   "$npm_package_config_mocha_files" >&3
