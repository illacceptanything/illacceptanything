var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("runnableExampleInlineTagDef", function() {

  var exampleMap, tagDef;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();

    exampleMap = injector.get('exampleMap');
    exampleMap.set('some-example', {
      runnableExampleDoc: {
        renderedContent: 'The rendered content of the some-example example'
      }
    });
    tagDef = injector.get('runnableExampleInlineTagDef');
  });

  it("should have the correct name", function() {
    expect(tagDef.name).toEqual('runnableExample');
  });

  it("should lookup the runnableExampleDoc identified in the tag description and return its renderedContent", function() {
    expect(tagDef.handler({}, 'runnableExample', 'some-example')).toEqual('The rendered content of the some-example example');
  });
});