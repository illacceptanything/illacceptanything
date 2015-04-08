var _ = require('lodash');
var path = require('canonical-path');
var fs = require('q-io/fs');
var Q = require('q');

/**
 * @dgProcessor writeFilesProcessor
 * @param {Object} log A service that provides logging
 * @description Write the value of `doc.renderedContent` to a file a  `doc.outputPath`.
 * @property {String} outputFolder The base path to the folder where files are outputted
 */
module.exports = function writeFilesProcessor(log, readFilesProcessor, writeFile) {
  return {
    outputFolder: null,
    $validate: {
      outputFolder: { presence: true },
    },
    $runAfter:['writing-files'],
    $runBefore: ['files-written'],
    $process: function(docs) {
      var outputFolder = this.outputFolder;
      return Q.all(_.map(docs, function(doc) {

        if ( !doc.outputPath ) {
          log.debug('Document "' + doc.id + ', ' + doc.docType + '" has no outputPath.');
        } else {

          var outputFile = path.resolve(readFilesProcessor.basePath, outputFolder, doc.outputPath);

          log.silly('writing file', outputFile);
          return writeFile(outputFile, doc.renderedContent).then(function() {
            log.debug('written file', outputFile);
            return outputFile;
          });

        }
      })).then(function() {
        return docs;
      });
    }
  };
};