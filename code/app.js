// Run some python
var execSync = require('child_process').execSync;
process.stdout.write(execSync('python -c "print(\'hello world\')"').toString());
console.log("insert app.js here, be sure to include all the js libraries ever.");
var penis = '8====D'

var all_the_js_libraries_ever = []; // TODO: someone fill this in

var App = function() {
  this.app = 'app';
  return this;
};

App.prototype = {
  init: function() {
    return this;
  },

  includeLibraries: function() {
    for (var i = 0; i < all_the_js_libraries_ever.length; i++) {
      // the novelty has worn off by this point
    }
  }
}
