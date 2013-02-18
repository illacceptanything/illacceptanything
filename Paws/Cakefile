_       = require 'lodash'

# Configuration vars.
option '-w', '--watch',          'watch files for changes, and recompile results'
option '-g', '--grep [PATTERN]', 'see `mocha --help`'
option '-i', '--invert',         'see `mocha --help`'
option '-r', '--reporter [REP]', 'specify Mocha reporter to display test results'
option '-W', '--wait',           'open browser and wait, on documentation tasks'
option '-t', '--tests',          'include tests in the bundle'

config =
   dirs:
      source:     'Source'
      tests:      'Test'
      docs:       'Documentation'
      products:   'Library'
   mocha:
      reporter:   'spec'
      ui:         'bdd'
      env:        'test'
   docco:
      browser:    'Google Chrome' # Browser to open HTML documentation in
   
   package:    require './package.json'


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


task 'compile', "write out JavaScript to lib/", -> # NYI

browserify = require 'browserify'
glob       = require 'glob'
fs         = require 'fs'
task 'compile:browser', "bundle JavaScript through Browserify", (options) ->
   bundle = browserify
      watch: options.watch
      cache: true
      exports: ['require', 'process']
   
   bundle.addEntry path.join config.dirs.source, 'Paws.coffee'
   if options.tests
      bundle.addEntry file for file in glob.sync config.package.testling.files
   
   fs.writeFile config.package.main.replace(/(?=\.(?:js|coffee))|$/, '.bundle'), bundle.bundle()


task 'clean', "remove git-ignore'd build products", ->
   spawn 'mv', ['node_modules/', 'node_modules-PRECLEAN'] # Hacky as fuck.
   spawn 'git', ['clean', '-fXd']
   spawn 'mv', ['node_modules-PRECLEAN/', 'node_modules']

task 'html',  -> invoke 'docs'
task 'build', -> invoke 'compile'
