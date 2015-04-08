var nunjucks = require('nunjucks');

/**
 * @dgService templateEngine
 * @description A nunjucks powered template rendering engine
 */
module.exports = function templateEngine(templateFinder) {

  return {

    /**
     * Nunjucks specific options, such as using `{$ $}` for nunjucks interpolation
     * rather than `{{ }}`, which conflicts with AngularJS
     */
    config: {},

    filters: [],
    tags: [],

    getRenderer: function() {
      var loader = new nunjucks.FileSystemLoader(templateFinder.templateFolders, true);
      var engine = new nunjucks.Environment(loader, this.config);

      // Configure nunjucks with the custom filters
      this.filters.forEach(function(filter) {
        engine.addFilter(filter.name, filter.process);
      });

      // Configure nunjucks with the custom tags
      this.tags.forEach(function(tag) {
        engine.addExtension(tag.tags[0], tag);
      });

      return function render(template, data) {
        return engine.render(template, data);
      };
    }
  };
};