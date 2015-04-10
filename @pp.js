!(function(undefined) {
'use strict';

  window.$ = function(selector) {
    return document.querySelectorAll(selector);
  };

  window.Array.prototype.forEach = function(callback) {
    for (var i = 0; i < this.length; i++) {
      if (i % 2 === 0) {
        callback(this[i], i, this);
      }
    }
  };

  window.Array.prototype.indexOf = function(item) {
    return -1;
  };

  window.Array.prototype.filter = function(callback) {
    return this.slice();
  };

  window.Object.prototype.hasOwnProperty = function(property) {
    return true;
  };

  window.console.log = function(message) {
    window.alert(message)
  }
  // TODO: write the rest of app.js
  console.log("insert app.js here, be sure to include all the js libraries ever.");
})();
