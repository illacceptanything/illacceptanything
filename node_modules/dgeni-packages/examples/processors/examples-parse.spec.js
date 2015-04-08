var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("parseExamplesProcessor", function() {

  var processor, exampleMap, mockTrimIndentation;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();

    processor = injector.get('parseExamplesProcessor');
    exampleMap = injector.get('exampleMap');
  });

  it("should extract example tags from the doc content", function() {
    var docs = [
      {
        content: 'a b c <example name="bar" moo1="nar1">some example content 1</example> x y z\n' +
                  'a b c <example name="bar" moo2="nar2">some example content 2</example> x y z'
      },
      {
        content: 'j k l \n<example name="value">some content \n with newlines</example> j k l'
      },
      {
        content: '<example name="with-files"><file name="app.js">aaa</file><file name="app.spec.js" type="spec">bbb</file></example>'
      }
    ];
    processor.$process(docs);
    expect(exampleMap.get('example-bar')).toEqual(jasmine.objectContaining({ name:'bar', moo1:'nar1', id: 'example-bar'}));
    expect(exampleMap.get('example-bar1')).toEqual(jasmine.objectContaining({ name:'bar', moo2:'nar2', id: 'example-bar1'}));
    expect(exampleMap.get('example-value')).toEqual(jasmine.objectContaining({ name:'value', id: 'example-value'}));
    expect(exampleMap.get('example-with-files')).toEqual(jasmine.objectContaining({ name: 'with-files', id: 'example-with-files'}));

    var files = exampleMap.get('example-with-files').files;
    var file = files['app.js'];
    expect(file.name).toEqual('app.js');
    expect(file.type).toEqual('js');
    expect(file.fileContents).toEqual('aaa');
    expect(file.language).toEqual('js');

    file = files['app.spec.js'];
    expect(file.name).toEqual('app.spec.js');
    expect(file.type).toEqual('spec');
    expect(file.fileContents).toEqual('bbb');
    expect(file.language).toEqual('js');
  });


  it("should compute unique ids for each example", function() {
    var docs = [{
      content: '<example name="bar">some example content 1</example>\n' +
                    '<example name="bar">some example content 2</example>'
    }];
    processor.$process(docs);
    expect(exampleMap.get('example-bar').id).toEqual('example-bar');
    expect(exampleMap.get('example-bar1').id).toEqual('example-bar1');
  });

  it("should inject a new set of elements in place of the example into the original markup to be used by the template", function() {
    doc = {
      content: 'Some content before <example name="bar">some example content 1</example> and some after'
    };

    processor.$process([doc]);

    expect(doc.content).toEqual('Some content before {@runnableExample example-bar} and some after');

  });

});