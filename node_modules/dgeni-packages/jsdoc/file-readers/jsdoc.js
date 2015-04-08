var _ = require('lodash');
var esprima = require('esprima');

/**
 * @dgService jsdocFileReader
 * @description
 * This file reader will create a simple doc for each js
 * file including a code AST of the JavaScript in the file.
 */
module.exports = function jsdocFileReader(log) {
  return {
    name: 'jsdocFileReader',
    defaultPattern: /\.js$/,
    getDocs: function(fileInfo) {

      try {
        fileInfo.ast = esprima.parse(fileInfo.content, {
          loc: true,
          attachComment: true
        });
      } catch(ex) {
       ex.file = fileInfo.filePath;
        throw new Error(
          _.template('JavaScript error in file "${file}"" [line ${lineNumber}, column ${column}]: "${description}"', ex));
      }

      return [{
        docType: 'jsFile'
      }];
    }
  };
};