#!/usr/bin/env sh

# Usage:
# ------
# This will run 'coffee-coverage' across the codebase, and then generate HTML coverage to file
# descriptor 3.
# 
#     npm run-script coverage
#     # ./Scripts/coverage.sh 3>coverage.html

if [ "$npm_package_config_mocha_reporter" != 'mocha-lcov-reporter' ]; then
       npm_package_config_mocha_reporter='html-cov'                 ; fi

./node_modules/.bin/coffeeCoverage --path=relative \
   $npm_package_config_dirs_source \
   $npm_package_config_dirs_instrumentation >/dev/null

env NODE_ENV='coverage' \
./node_modules/.bin/mocha --compilers 'coffee:coffee-script' \
   --reporter $npm_package_config_mocha_reporter \
   $npm_package_config_testFiles >&3
