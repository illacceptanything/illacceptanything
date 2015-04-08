var path = require('canonical-path');
var Package = require('dgeni').Package;

module.exports = new Package('ngdoc', [require('../jsdoc'), require('../nunjucks')])

.factory(require('./file-readers/ngdoc'))
.factory(require('./inline-tag-defs/link'))
.factory(require('./services/getAliases'))
.factory(require('./services/getDocFromAlias'))
.factory(require('./services/getLinkInfo'))
.factory(require('./services/getTypeClass'))
.factory(require('./services/moduleMap'))

.processor(require('./processors/filterNgdocs'))
.processor(require('./processors/generateComponentGroups'))
.processor(require('./processors/memberDocs'))
.processor(require('./processors/moduleDocs'))
.processor(require('./processors/providerDocs'))


.config(function(readFilesProcessor, ngdocFileReader) {
  readFilesProcessor.fileReaders.push(ngdocFileReader);
})


.config(function(parseTagsProcessor, getInjectables) {
  parseTagsProcessor.tagDefinitions =
      parseTagsProcessor.tagDefinitions.concat(getInjectables(require('./tag-defs')));
})


.config(function(inlineTagProcessor, linkInlineTagDef) {
  inlineTagProcessor.inlineTagDefinitions.push(linkInlineTagDef);
})


.config(function(templateFinder, templateEngine, getInjectables) {

  templateFinder.templateFolders.unshift(path.resolve(__dirname, 'templates'));

  templateEngine.config.tags = {
    variableStart: '{$',
    variableEnd: '$}'
  };

  templateFinder.templatePatterns = [
    '${ doc.template }',
    '${doc.area}/${ doc.id }.${ doc.docType }.template.html',
    '${doc.area}/${ doc.id }.template.html',
    '${doc.area}/${ doc.docType }.template.html',
    '${ doc.id }.${ doc.docType }.template.html',
    '${ doc.id }.template.html',
    '${ doc.docType }.template.html'
  ].concat(templateEngine.templatePatterns);

  templateEngine.filters = templateEngine.filters.concat(getInjectables([
    require('./rendering/filters/code'),
    require('./rendering/filters/link'),
    require('./rendering/filters/type-class')
  ]));

  templateEngine.tags = templateEngine.tags.concat(getInjectables([require('./rendering/tags/code')]));

})


.config(function(computeIdsProcessor, createDocMessage, getAliases) {

  computeIdsProcessor.idTemplates.push({
    docTypes: ['module' ],
    idTemplate: 'module:${name}',
    getAliases: getAliases
  });

  computeIdsProcessor.idTemplates.push({
    docTypes: ['method', 'property', 'event'],
    getId: function(doc) {
      var parts = doc.name.split('#');
      var name = parts.pop();
      parts.push(doc.docType + ':' + name);
      return parts.join('#');
    },
    getAliases: getAliases
  });

  computeIdsProcessor.idTemplates.push({
    docTypes: ['provider', 'service', 'directive', 'input', 'object', 'function', 'filter', 'type' ],
    idTemplate: 'module:${module}.${docType}:${name}',
    getAliases: getAliases
  });


})

.config(function(computePathsProcessor, createDocMessage) {
  computePathsProcessor.pathTemplates.push({
    docTypes: ['provider', 'service', 'directive', 'input', 'object', 'function', 'filter', 'type' ],
    pathTemplate: '${area}/${module}/${docType}/${name}',
    outputPathTemplate: 'partials/${area}/${module}/${docType}/${name}.html'
  });
  computePathsProcessor.pathTemplates.push({
    docTypes: ['module' ],
    pathTemplate: '${area}/${name}',
    outputPathTemplate: 'partials/${area}/${name}/index.html'
  });
  computePathsProcessor.pathTemplates.push({
    docTypes: ['componentGroup' ],
    pathTemplate: '${area}/${moduleName}/${groupType}',
    outputPathTemplate: 'partials/${area}/${moduleName}/${groupType}/index.html'
  });
});