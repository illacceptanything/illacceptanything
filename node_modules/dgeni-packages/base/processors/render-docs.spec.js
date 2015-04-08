var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

var processor, renderSpy, findTemplateSpy;

beforeEach(function() {

  var testPackage = mockPackage().factory('templateFinder', function() {
    var finderSpy = createSpy('findTemplate').and.returnValue('SOME TEMPLATE');
    return {
      getFinder: function() { return finderSpy; }
    };
  });

  var dgeni = new Dgeni([testPackage]);
  var injector = dgeni.configureInjector();
  findTemplateSpy = injector.get('templateFinder').getFinder();
  renderSpy = injector.get('templateEngine').getRenderer();

  processor = injector.get('renderDocsProcessor');
});

describe("render-docs", function() {

  it("should call the templateFinder for each doc", function() {
    var doc1 = {}, doc2 = {}, docs = [ doc1, doc2 ];
    processor.$process(docs);
    expect(findTemplateSpy.calls.count()).toEqual(2);
    expect(findTemplateSpy.calls.argsFor(0)).toEqual([doc1]);
    expect(findTemplateSpy.calls.argsFor(1)).toEqual([doc2]);
  });

  it("should call the templateEngine.render with the template and data", function() {
    var doc1 = { id: 1 }, doc2 = { id: 2 }, docs = [ doc1, doc2 ];
    var someProp = {}, someMethod = function() {};

    processor.extraData.someProp = someProp;
    processor.helpers.someMethod = someMethod;

    processor.$process(docs);

    expect(renderSpy.calls.count()).toEqual(2);
    expect(renderSpy.calls.argsFor(0)).toEqual(['SOME TEMPLATE',
      { doc: doc1, docs: docs, someProp: someProp, someMethod: someMethod }]);
    expect(renderSpy.calls.argsFor(1)).toEqual(['SOME TEMPLATE',
      { doc: doc2, docs: docs, someProp: someProp, someMethod: someMethod }]);
  });

  it("should place the result of calling templateEngine.render into doc.renderedContent", function() {
    var doc1 = { id: 1 }, doc2 = { id: 2 }, docs = [ doc1, doc2 ];

    renderSpy.and.returnValue('RENDERED CONTENT');

    processor.$process(docs);
    expect(doc1.renderedContent).toEqual('RENDERED CONTENT');
    expect(doc2.renderedContent).toEqual('RENDERED CONTENT');
  });

});
