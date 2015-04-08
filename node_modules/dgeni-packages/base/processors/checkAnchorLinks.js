var _ = require('lodash');
var path = require('canonical-path');

/**
 * @dgProcessor checkAnchorLinksProcessor
 * @param {Object} log A service that provides logging
 * @description Checks that the generated documents do not have any dangling anchor links.
 */
module.exports = function checkAnchorLinksProcessor(log, resolveUrl, extractLinks) {
  return {
    ignoredLinks: [/^http(?:s)?:\/\//, /^mailto:/, /^chrome:/],
    pathVariants: ['', '/', '.html', '/index.html'],
    checkDoc: function(doc) { return doc.path && path.extname(doc.outputPath) === '.html'; },
    base: null,
    webRoot: '/',
    $validate: {
      ignoredLinks: { presence: true },
      pathVariants: { presence: true },
      webRoot: { presence: true }
    },
    $runAfter:['writing-files'],
    $runBefore: ['files-written'],
    $process: function(docs) {
      var ignoredLinks = this.ignoredLinks;
      var pathVariants = this.pathVariants;
      var filesToCheck = this.filesToCheck;
      var base = this.base;
      var webRoot = this.webRoot;
      var checkDoc = this.checkDoc;

      var allDocs = [];
      var allValidReferences = {};

      // Extract and store all the possible valid anchor reference that can be found
      // in each doc to the allValidReferences hash for checking later
      _.forEach(docs, function(doc) {

        var linkInfo;

        // Only check specified output files
        if ( checkDoc(doc) ) {

          // Make the path to the doc relative to the webRoot
          var docPath = path.join(webRoot, doc.path);

          // Parse out all link hrefs, names and ids
          linkInfo = extractLinks(doc.renderedContent);

          linkInfo.path = docPath;
          linkInfo.outputPath = doc.outputPath;
          allDocs.push(linkInfo);

          _.forEach(pathVariants, function(pathVariant) {
            var docPathVariant = docPath + pathVariant;

            // The straight doc path is a valid reference
            allValidReferences[docPathVariant] = true;
            // The path with a trailing hash is valid
            allValidReferences[docPathVariant + '#'] = true;
            // The path referencing each name/id in the doc is valid
            _.forEach(linkInfo.names, function(name) {
              allValidReferences[docPathVariant + '#' + name] = true;
            });
          });
        }
      });

      var unmatchedLinkCount = 0;

      // Check that all anchor links in each doc point to valid
      // references within the docs collection
      _.forEach(allDocs, function(linkInfo) {
        log.silly('checking file', linkInfo);

        var unmatchedLinks = [];

        _(linkInfo.hrefs)

          // Filter out links that should be ignored
          .filter(function(href) {
            return _.all(ignoredLinks, function(rule) {
              return !rule.test(href);
            });
          })

          .forEach(function(link) {
            var normalizedLink = path.join(webRoot, resolveUrl(linkInfo.path, link, base));
            if ( !_.any(pathVariants, function(pathVariant) {
              return allValidReferences[normalizedLink + pathVariant];
            }) ) {
              unmatchedLinks.push(link);
            }
          });

        if ( unmatchedLinks.length ) {
          unmatchedLinkCount += unmatchedLinks.length;
          log.warn('Dangling Links Found in "' + linkInfo.outputPath + '":' +
            _.map(unmatchedLinks, function(link) { return '\n - ' + link; }));
        }
      });

      if ( unmatchedLinkCount ) {
        log.warn(unmatchedLinkCount, 'unmatched links');
      }
    }
  };
};
