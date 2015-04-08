var _ = require('lodash');

/**
 * @dgProcessor unescapeCommentsProcessor
 * @description
 * Some files (like CSS) use the same comment markers as the jsdoc comments, such as /&amp;#42;.
 * To get around this we HTML encode them in the source.
 * This processor unescapes them back to normal comment markers
 */
module.exports = function unescapeCommentsProcessor() {
  return {
    $runAfter: ['docs-rendered'],
    $runBefore: ['writing-files'],
    $process: function(docs) {
      _.forEach(docs, function(doc) {
        doc.renderedContent = doc.renderedContent.replace(/\/&amp;#42;/g, '/*').replace(/&amp;#42;\//g, '*/');
      });
    }
  };
};