# grunt-jasmine-node

A grunt.js task to run your jasmine feature suite using jasmine-node.

## Getting Started
Install this grunt plugin next to your project's grunt.js gruntfile with: `npm install grunt-jasmine-node`

Then add this line to your project's `grunt.js` grunt file:

```javascript
grunt.initConfig({
  jasmine_node: {
    specNameMatcher: "./spec", // load only specs containing specNameMatcher
    projectRoot: ".",
    requirejs: false,
    forceExit: true,
    jUnit: {
      report: false,
      savePath : "./build/reports/jasmine/",
      useDotNotation: true,
      consolidate: true
    }
  }
});

grunt.loadNpmTasks('grunt-jasmine-node');

grunt.registerTask('default', 'jasmine_node');
```

## Bugs

Help us squash them by submitting an issue that describes how you encountered it; please be as specific as possible including operating system, node, grunt, and grunt-jasmine-node versions.

## Release History

see [GitHub Repository](/s9tpepper/grunt-jasmine-node).

## License
Copyright (c) 2012 "s9tpepper" Omar Gonzalez & contributors.
Licensed under the MIT license.
