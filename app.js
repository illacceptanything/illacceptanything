console.log("insert app.js here, be sure to include all the js libraries ever.");

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
