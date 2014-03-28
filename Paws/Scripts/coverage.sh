#!/usr/bin/env sh

# Usage:
# ------
# This will run 'coffee-coverage' across the codebase, and then generate HTML coverage to file
# descriptor 3.
# 
#     npm run-script coverage
#     # ./Scripts/coverage.sh 3>./Library/coverage.html

if [ "$npm_package_config_mocha_reporter" != 'mocha-lcov-reporter' ]; then
       npm_package_config_mocha_reporter='html-cov'                 ; fi

./node_modules/.bin/coffeeCoverage --path=relative \
   --exclude 'additional.coffee' \
   "$npm_package_config_dirs_source" \
   "$npm_package_config_dirs_instrumentation" >/dev/null

./node_modules/.bin/coffee --compile \
   --output "$npm_package_config_dirs_instrumentation" \
   "$npm_package_config_dirs_source/additional.coffee"

env NODE_ENV='coverage' \
./node_modules/.bin/mocha --compilers 'coffee:coffee-script/register' \
   --reporter "$npm_package_config_mocha_reporter" \
   "$npm_package_config_testFiles" >&3
