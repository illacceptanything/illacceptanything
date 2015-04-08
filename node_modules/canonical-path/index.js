var path = require('path');
var i;

function canonicalPath(filePath) {
  if( path.sep === '\\') {
    filePath = filePath.replace(/\\/g, '/');
  }
  return filePath;
}

function wrapWithCanonical(fn) {
  return function() {
    return canonicalPath(fn.apply(path, arguments));
  };
}

var fns = ['normalize', 'join', 'resolve', 'relative', 'dirname', 'basename', 'extname'];
var props = ['sep', 'delimiter'];

fns.forEach(function(fn) {
  exports[fn] = wrapWithCanonical(path[fn]);
});
props.forEach(function(prop) {
  exports[prop] = path[prop];
});
exports.canonical = canonicalPath;
