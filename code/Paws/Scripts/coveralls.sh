#!/usr/bin/env sh

npm_package_config_mocha_reporter='mocha-lcov-reporter' \
./Scripts/coverage.sh 3>&1 | ./node_modules/.bin/coveralls
