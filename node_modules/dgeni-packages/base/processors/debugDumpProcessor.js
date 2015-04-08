var path = require('canonical-path');
var util = require("util");

/**
 * @dgProcessor debugDumpProcessor
 * @description
 * This processor dumps docs that match a filter to a file
 */
module.exports = function debugDumpProcessor(log, readFilesProcessor, writeFile) {
  return {
    filterFn: function(docs) { return docs; },
    outputPath: 'debug-dump.txt',
    depth: 2,

    $enabled: false,

    $validate: {
      filterFn: { presence: true },
      outputPath: { presence: true },
      depth: { presence: true }
    },

    $runBefore: ['writing-files'],

    $process: function(docs) {
      log.info('Dumping docs:', this.filterFn, this.outputPath);
      var filteredDocs = this.filterFn(docs);
      var dumpedDocs = util.inspect(filteredDocs, this.depth);
      var outputPath = path.resolve(readFilesProcessor.basePath, this.outputPath);
      return writeFile(outputPath, dumpedDocs).then(function() {
        return docs;
      });
    }
  };

};