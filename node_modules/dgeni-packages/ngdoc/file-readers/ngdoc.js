/**
 * @dgService ngdocFileReader
 * @description
 * This file reader will pull the contents from a text file (by default .ngdoc)
 *
 * The doc will initially have the form:
 * ```
 * {
 *   content: 'the content of the file',
 *   startingLine: 1
 * }
 * ```
 */
module.exports = function ngdocFileReader() {
  return {
    name: 'ngdocFileReader',
    defaultPattern: /\.ngdoc$/,
    getDocs: function(fileInfo) {
      // We return a single element array because ngdoc files only contain one document
      return [{
        content: fileInfo.content,
        startingLine: 1
      }];
    }
  };
};