var _ = require('lodash');
var StringMap = require('stringmap');

/**
 * @dgProcessor computeIdsProcessor
 * @description
 * Compute the id property of each doc based on the tags and other meta-data from a set of templates
 */
module.exports = function computeIdsProcessor(log, aliasMap, createDocMessage) {

  var getIdMap, getAliasesMap;

  var initializeMaps = function(idTemplates) {
    getIdMap = new StringMap();
    getAliasesMap = new StringMap();

    idTemplates.forEach(function(template) {
      if ( template.docTypes ) {
        template.docTypes.forEach(function(docType) {

          if ( template.getId ) {
            getIdMap.set(docType, template.getId);
          } else if ( template.idTemplate ) {
             getIdMap.set(docType, _.template(template.idTemplate));
          }

          if ( template.getAliases ) {
            getAliasesMap.set(docType, template.getAliases);
          }

        });
      }
    });
  };

  return {
    $runAfter: ['computing-ids'],
    $runBefore: ['ids-computed'],
    $validate: {
      idTemplates: { presence: true }
    },
    idTemplates: [],
    $process: function(docs) {
      initializeMaps(this.idTemplates);

      docs.forEach(function(doc) {
        try {
          if ( !doc.id ) {
            var getId = getIdMap.get(doc.docType);
            if ( !getId ) {
              log.warn(createDocMessage('No idTemplate or getId(doc) method provided', doc));
            } else {
              doc.id = getId(doc);
            }
          }

          if ( !doc.aliases ) {
            var getAliases = getAliasesMap.get(doc.docType);
            if ( !getAliases ) {
              log.warn(createDocMessage('No getAlias(doc) method provided', doc));
            } else {
              doc.aliases = getAliases(doc);
            }
          }

          aliasMap.addDoc(doc);

        } catch(err) {
          throw new Error(createDocMessage('Failed to compute ids/aliases for doc', doc, err));
        }

        log.debug('computed id for:', '"' + doc.id + '" (' + doc.docType + ')');

      });
    }
  };
};
