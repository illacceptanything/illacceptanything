var Package = require('dgeni').Package;

/**
 * @dgPackage nunjucks
 * @description Provides a template engine powered by Nunjucks
 */
module.exports = new Package('nunjucks', ['base'])

.factory(require('./services/renderMarkdown'))
.factory(require('./services/nunjucks-template-engine'))
.factory(require('./rendering/tags/marked'))
.factory(require('./rendering/filters/marked'))

.config(function(templateEngine, markedNunjucksTag, markedNunjucksFilter) {
  templateEngine.tags.push(markedNunjucksTag);
  templateEngine.filters = templateEngine.filters
    .concat(require('./rendering/filters/change-case'))
    .concat([
      require('./rendering/filters/first-line'),
      require('./rendering/filters/first-paragraph'),
      require('./rendering/filters/json'),
      markedNunjucksFilter
    ]);
});
