_       = require 'lodash'

# Configuration vars.
option '-r', '--reporter [REP]', 'Mocha reporter to display test results'

config =
   mocha:
      reporter:   'spec'
      dir:        'Test'
      ui:         'bdd'
      env:        'test'


# I try to use standard `make`-target names for these tasks.
# See: http://www.gnu.org/software/make/manual/make.html#Standard-Targets
{spawn}  = require 'child_process'
path     = require 'path'
task 'test', 'run testsuite through Mocha', (options) ->
   env = Object.create process.env,
      NODE_ENV: { value: mocha.env }
   
   spawn path.resolve('./node_modules/.bin/mocha'),
      [ '--compilers', 'coffee:coffee-script'
        '--reporter',   options.reporter or mocha.reporter
        '--ui',         mocha.ui
         path.resolve mocha.dir ]
      stdio: 'inherit'
      cwd: path.resolve mocha.dir
      env: env
