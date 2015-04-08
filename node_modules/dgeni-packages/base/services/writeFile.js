var fs = require('q-io/fs');

/**
 * @dgService writeFile
 * @description
 * Write the given contents to a file, ensuring the path to the file exists
 */
module.exports = function writeFile() {
  return function(file, content) {
    return fs.makeTree(fs.directory(file)).then(function() {
      return fs.write(file, content, 'wb');
    });
  };
};