_       = require 'lodash'

# Configuration vars.
option '-r', '--reporter [REP]', 'Mocha reporter to display test results'

mocha =
   reporter:   'spec'
   dir:        'Test'
   ui:         'bdd'
   env:        'test'



path    = require 'path'
{spawn} = require 'child_process'

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
