var _ = require('lodash');

/**
 * @dgProcessor generateComponentGroupsProcessor
 * @description
 * Generate documents for each group of components (by type) within a module
 */
module.exports = function generateComponentGroupsProcessor(moduleMap) {
  return {
    $runAfter: ['moduleDocsProcessor'],
    $runBefore: ['computing-paths'],
    $process: function(docs) {

      moduleMap.forEach(function(module) {

        _(module.components)
          .groupBy('docType')
          .tap(function(docTypes) {
            // We don't want the overview docType to be represented as a componentGroup
            delete docTypes.overview;
          })
          .map(function(docs, docType) {
            return {
              id: module.id + '.' + docType,
              docType: 'componentGroup',
              groupType: docType,
              moduleName: module.name,
              moduleDoc: module,
              area: module.area,
              name: docType + ' components in '  + module.name,
              components: docs
            };
          })
          .tap(function(groups) {
            module.componentGroups = groups;
            _.forEach(groups, function(group) {
              docs.push(group);
            });
          });
      });
    }
  };
};