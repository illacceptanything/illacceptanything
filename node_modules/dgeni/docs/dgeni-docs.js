var path = require('canonical-path');
var Package = require('dgeni').Package;

module.exports = new Package('dgeniDocsPackage', [
  require('dgeni-packages/dgeni'),
  require('dgeni-packages/jsdoc'),
  require('dgeni-packages/nunjucks')
])

.config(function(log, readFilesProcessor, writeFilesProcessor, templateFinder, debugDumpProcessor) {

  log.level = 'info';

  readFilesProcessor.basePath = path.resolve(__dirname, '..');
  readFilesProcessor.sourceFiles = [{ include: 'lib/**/*.js', basePath: 'lib' }];
  writeFilesProcessor.outputFolder = 'docs/build';

  templateFinder.templateFolders.unshift('docs/templates');
  templateFinder.templatePatterns.unshift('common.template.html');
});