var _ = require('lodash');

/**
 * @dgProcessor providerDocsProcessor
 * @description
 * Connect docs for services to docs for their providers
 */
module.exports = function providerDocsProcessor(log, aliasMap, createDocMessage) {
  return {
    $runAfter: ['ids-computed', 'memberDocsProcessor'],
    $runBefore: ['computing-paths'],
    $process: function(docs) {

      // Map services to their providers
      _.forEach(docs, function(doc) {
        if ( doc.docType === 'provider' ) {
          var serviceId = doc.id.replace(/provider:/, 'service:').replace(/Provider$/, '');
          var serviceDocs = aliasMap.getDocs(serviceId);

          if ( serviceDocs.length === 1 ) {
            serviceDoc = serviceDocs[0];
            doc.serviceDoc = serviceDoc;
            serviceDoc.providerDoc = doc;
          } else if ( serviceDocs.length === 0 ) {
            log.warn(createDocMessage('Missing service "' + serviceId + '" for provider', doc));
          } else {
            log.warn(createDocMessage('Ambiguous service name "' + serviceId + '" for provider', doc) + '\n' +
              _.reduce(serviceDocs, function(msg, doc) {
                return msg + '\n  "' + doc.id + '"';
              }, 'Matching docs: '));
          }
        }
      });
    }
  };
};