#!/usr/bin/env sh

# Usage:
# ------
# This simply invokes `mocha` with some sensible defaults.
# 
#     FLAGS="--reporter dot --watch" npm run-script test
#     FLAGS="--grep API" npm run-script test
# 
# Unfortunately, at the moment, there's no way to pass flags to `npm run-script`; so I roll in flags
# using an environment-variable (`$FLAGS`). See: https://github.com/isaacs/npm/issues/3494

env NODE_ENV="$npm_package_config_mocha_ENV"       \
./node_modules/.bin/mocha                          \
   --compilers coffee:coffee-script/register       \
   --reporter "$npm_package_config_mocha_reporter" \
   --ui "$npm_package_config_mocha_ui"             \
   $FLAGS "$npm_package_config_testFiles"
