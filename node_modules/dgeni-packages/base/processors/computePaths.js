var _ = require('lodash');
var path = require('canonical-path');
var StringMap = require('stringmap');

/**
 * @dgProcessor computePathsProcessor
 * @description Compute the path and outputPath for docs that do not already have them from a set of templates
 */
module.exports = function computePathsProcessor(log, createDocMessage) {
  var pathTemplateMap, outputPathTemplateMap;

  var initializeMaps = function(pathTemplates) {
    pathTemplateMap = new StringMap();
    outputPathTemplateMap = new StringMap();

    pathTemplates.forEach(function(template) {
      if ( template.docTypes ) {
        template.docTypes.forEach(function(docType) {

          if ( template.getPath ) {
            pathTemplateMap[docType] = template.getPath;
          } else if ( template.pathTemplate ) {
             pathTemplateMap[docType] = _.template(template.pathTemplate);
          }

          if ( template.getOutputPath ) {
            outputPathTemplateMap[docType] = template.getOutputPath;
          } else if ( template.outputPathTemplate ) {
             outputPathTemplateMap[docType] = _.template(template.outputPathTemplate);
          }
        });
      }
    });
  };

  return {
    $validate: {
      pathTemplates: { presence: true }
    },
    pathTemplates: [],
    $runAfter: ['computing-paths'],
    $runBefore: ['paths-computed'],
    $process: function(docs) {

      initializeMaps(this.pathTemplates);

      docs.forEach(function(doc) {

        try {

          if ( !doc.path ) {
            var getPath = pathTemplateMap[doc.docType];
            if ( !getPath ) {
              log.warn(createDocMessage('No path template provided', doc));
            } else {
              doc.path = getPath(doc);
            }
          }

          if ( !doc.outputPath ) {
            var getOutputPath = outputPathTemplateMap[doc.docType];
            if ( !getOutputPath ) {
              log.warn(createDocMessage('No output path template provided', doc));
            } else {
              doc.outputPath = getOutputPath(doc);
            }
          }

        } catch(err) {
          throw new Error(createDocMessage('Failed to compute paths for doc', doc, err));
        }

        log.debug(createDocMessage('path: ' + doc.path + '; outputPath: ' + doc.outputPath, doc));
      });
    }
  };
};