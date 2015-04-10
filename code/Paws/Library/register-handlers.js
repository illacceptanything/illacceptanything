//  Register coffee-coverage if coverage is enabled.
if (process.env['NODE_ENV'] === 'coverage')
   require('coffee-coverage').register({
      path: 'relative'
    , basePath: require('path').resolve(__dirname, '../')
    , exclude: [
         'Test', 'node_modules', '.git'
       , 'Source/primitives'
       , 'Source/additional.coffee'
       , 'Executables' // This feels like cheating.
      ]
    , initAll: true
   })
