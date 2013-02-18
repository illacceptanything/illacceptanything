_       = require 'lodash'

# Configuration vars.
option '-g', '--grep [PATTERN]', 'see `mocha --help`'
option '-i', '--invert',         'see `mocha --help`'
option '-r', '--reporter [REP]', 'specify Mocha reporter to display test results'
option '-W', '--wait',           'open browser and wait, on documentation tasks'

config =
   dirs:
      source:  'Source'
      tests:   'Test'
      docs:    'Documentation'
   mocha:
      reporter:   'spec'
      ui:         'bdd'
      env:        'test'
   docco:
      browser:    'Google Chrome' # Browser to open HTML documentation in


# I try to use standard `make`-target names for these tasks.
# See: http://www.gnu.org/software/make/manual/make.html#Standard-Targets
{ spawn }   = require 'child_process'
path        = require 'path'
task 'test', 'run testsuite through Mocha', (options) ->
   env = Object.create process.env,
      NODE_ENV: { value: config.mocha.env }
   
   spawn path.resolve('./node_modules/.bin/mocha'),
      _.compact [ '--grep', (options.grep or '.'), (if options.invert then '--invert')
                  '--compilers', 'coffee:coffee-script'
                  '--reporter',   options.reporter or config.mocha.reporter
                  '--ui',         config.mocha.ui
                  path.resolve config.dirs.tests ]
      stdio: 'inherit'
      cwd: path.resolve config.dirs.tests
      env: env


{ document: docco } = require 'docco'
task 'docs', 'generate HTML documentation via Docco', (options) ->
   docco [path.join config.dirs.source, '*'], { output: config.dirs.docs }, ->
      invoke 'docs:open' if options.wait

task 'docs:open', (options) ->
   browser = spawn 'open',
      _.compact [ path.join(config.dirs.docs, 'Paws.html')
                  '-a', config.docco.browser
                  (if options.wait then '-W') ]
   
   if options.wait
      browser.on 'exit', -> invoke 'clean'

task 'html', -> invoke 'docs'


task 'clean', "remove git-ignore'd build products", ->
   spawn 'mv', ['node_modules/', 'node_modules-PRECLEAN'] # Hacky as fuck.
   spawn 'git', ['clean', '-fXd']
   spawn 'mv', ['node_modules-PRECLEAN/', 'node_modules']
