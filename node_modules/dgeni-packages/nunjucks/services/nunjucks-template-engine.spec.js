var rewire = require('rewire');
var engineFactory = rewire('./nunjucks-template-engine');

describe("nunjucksTemplateEngine service", function() {
  var nunjucks, addFilterSpy, addExtensionSpy, engine, mockTemplateFinder;

  beforeEach(function() {

    nunjucks = engineFactory.__get__('nunjucks');

    nunjucks.Environment = jasmine.createSpy('Environment');
    nunjucks.Environment.prototype.addFilter = addFilterSpy = jasmine.createSpy('addFilter');
    nunjucks.Environment.prototype.addExtension = addExtensionSpy = jasmine.createSpy('addExtension');

    mockTemplateFinder = {
      templateFolders: 'templates'
    };
    engine = engineFactory(mockTemplateFinder);
  });

  describe("getRenderer()", function() {

    it("should configure nunjucks", function() {

      engine.templateFolders = ['templates'];
      engine.config = { foo: 'bar' };

      var render = engine.getRenderer();
      expect(render).toEqual(jasmine.any(Function));

      expect(nunjucks.Environment).toHaveBeenCalledWith(
        jasmine.any(nunjucks.FileSystemLoader),
        engine.config
      );
    });


    it("should load the given custom filters", function() {

      var dummyFilter = { name: 'test', process: function() {} };
      engine.filters.push(dummyFilter);

      var render = engine.getRenderer();
      expect(render).toEqual(jasmine.any(Function));

      expect(addFilterSpy).toHaveBeenCalledWith(dummyFilter.name, dummyFilter.process);
    });


    it("should load the given custom tags", function() {

      var dummyExtension = { tags: ['dummy']};
      engine.tags.push(dummyExtension);

      var render = engine.getRenderer();
      expect(render).toEqual(jasmine.any(Function));

      expect(addExtensionSpy).toHaveBeenCalledWith('dummy', dummyExtension);
    });
  });
});