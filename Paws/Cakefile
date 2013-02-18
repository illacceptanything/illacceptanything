_       = require 'lodash'

# Configuration vars.
option '-r', '--reporter [REP]', 'Mocha reporter to display test results'

config =
   dirs:
      source:  'Source'
      tests:   'Test'
      docs:    'Documentation'
   mocha:
      reporter:   'spec'
      ui:         'bdd'
      env:        'test'


# I try to use standard `make`-target names for these tasks.
# See: http://www.gnu.org/software/make/manual/make.html#Standard-Targets
{ spawn }   = require 'child_process'
path        = require 'path'
task 'test', 'run testsuite through Mocha', (options) ->
   env = Object.create process.env,
      NODE_ENV: { value: config.mocha.env }
   
   spawn path.resolve('./node_modules/.bin/mocha'),
      [ '--compilers', 'coffee:coffee-script'
        '--reporter',   options.reporter or config.mocha.reporter
        '--ui',         config.mocha.ui
         path.resolve config.dirs.test ]
      stdio: 'inherit'
      cwd: path.resolve config.dirs.test
      env: env

{ document: docco } = require 'docco'
task 'html', 'generate HTML documentation', (options) ->
   docco [path.join config.dirs.source, '*'],
      output: config.dirs.docs
   spawn 'open', [path.join config.dirs.docs, 'Paws.html']
