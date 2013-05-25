_       = require 'lodash'

# Configuration vars.
option '-w', '--watch',          '(test, compile:client) watch files for changes, and recompile results'
option '-W', '--wait',           '(*:open) open browser and wait'
option '-g', '--grep [PATTERN]', '(test) see `mocha --help`'
option '-i', '--invert',         '(test) see `mocha --help`'
option '-r', '--reporter [REP]', '(test) specify Mocha reporter to display test results'
option '-t', '--tests',          '(compile:client) include tests in the bundle'
option '-a', '--browser [BROW]', '(*:open) select browser to use'

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

open_wait_task = (opts, path) ->
   browser = spawn 'open',
      _.compact [ path, '-a', opts.browser ? config.docco.browser, (if opts.wait then '-W') ]
   
   if opts.wait
      browser.on 'exit', -> invoke 'clean'


# I try to use standard `make`-target names for these tasks.
# See: http://www.gnu.org/software/make/manual/make.html#Standard-Targets
{spawn, exec}  = require 'child_process'
path           = require 'path'
task 'test', 'run testsuite through Mocha', (options) ->
   env = Object.create process.env,
      NODE_ENV: { value: config.mocha.env }
   
   child = spawn path.resolve('./node_modules/.bin/mocha'),
      _.compact [ '--grep', (options.grep or '.'), (if options.invert then '--invert')
                  '--watch' if options.watch
                  '--compilers', 'coffee:coffee-script'
                  '--reporter',   options.reporter or config.mocha.reporter
                  '--ui',         config.mocha.ui
                  path.resolve config.dirs.tests ]
      stdio: 'inherit'
      cwd: path.resolve config.dirs.tests
      env: env
   child.on 'exit', (code) -> process.on('exit', -> process.exit code) if code > 0

task 'test:client', (options) ->
   options.tests = true
   invoke 'compile:client'
   invoke 'test:client:open'

task 'test:client:open', (options) ->
   open_wait_task options, path.join(config.dirs.products, 'tests.html')

task 'travis', ->
   exec 'npm run-script coveralls', (error) ->
      process.exit 1 if error
      invoke 'test'
      

{ document: docco } = require 'docco'
task 'docs', 'generate HTML documentation via Docco', (options) ->
   docco [path.join config.dirs.source, '*'], { output: config.dirs.docs }, ->
      invoke 'docs:open' if options.wait
task 'docs:open', (options) ->
   open_wait_task options, path.join(config.dirs.docs, 'Paws.html')


#task 'compile', "write out JavaScript to lib/", -> # NYI

browserify = require 'browserify'
coffeeify  = require 'coffeeify'
glob       = require 'glob'
fs         = require 'fs'
task 'compile:client', "bundle JavaScript through Browserify", (options) ->
   bundle = browserify()
     #watch: options.watch # FIXME: Lost in 1.0 -> 2.0
     #cache: true # FIXME: Lost in 1.0 -> 2.0
     #exports: ['require', 'process'] # FIXME: Lost in 1.0 -> 2.0
   bundle.transform coffeeify
   
   bundle.ignore 'vm'
   
   bundle.add path.resolve process.cwd(), config.dirs.source, 'Paws.coffee'
   if options.tests
      bundle.add path.resolve process.cwd(), file for file in glob.sync config.package.testling.files
   
   bundle.bundle(debug: yes).pipe fs.createWriteStream(
      config.package.main.replace(/(?=\.(?:js|coffee))|$/, '.bundle') )


task 'clean', "remove git-ignore'd build products", ->
   exec 'npm run-script clean', (error) ->
      process.exit 1 if error

task 'html',  -> invoke 'docs'
#task 'build', -> invoke 'compile'
