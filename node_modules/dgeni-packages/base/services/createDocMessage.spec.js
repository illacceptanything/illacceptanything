var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("createDocMessage", function() {
  var createDocMessage;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    createDocMessage = injector.get('createDocMessage');
  });


  it("should generate a message with doc info", function() {
    var message = createDocMessage('some message', { id: 'doc1', name: 'doc-one', path: 'some/doc1', fileInfo: { relativePath: 'some/file.js'} , startingLine: 10, endingLine: 20 });
    expect(message).toEqual('some message - doc "doc1" - from file "some/file.js" - starting at line 10, ending at line 20');


    message = createDocMessage('some message', { name: 'doc-one', path: 'some/doc1', fileInfo: { relativePath: 'some/file.js'} , startingLine: 10 });
    expect(message).toEqual('some message - doc "doc-one" - from file "some/file.js" - starting at line 10');

    message = createDocMessage('some message', { path: 'some/doc1', fileInfo: { relativePath: 'some/file.js'} });
    expect(message).toEqual('some message - doc "some/doc1" - from file "some/file.js"');


    message = createDocMessage('some message', { path: 'some/doc1' });
    expect(message).toEqual('some message - doc "some/doc1"');

    message = createDocMessage('some message', { fileInfo: { relativePath: 'some/file.js'} });
    expect(message).toEqual('some message - doc - from file "some/file.js"');
  });


  it("should be able to wrap an original error", function() {
    var caught = false;
    try {
      throw new Error('original error');
    } catch(originalError) {
      caught = true;
      var message = createDocMessage('some message', { id: 'doc1', name: 'doc-one', path: 'some/doc1', fileInfo: { relativePath: 'some/file.js'} , startingLine: 10, endingLine: 20 }, originalError);
      expect(message).toContain('original error');
    }
    expect(caught).toBe(true);
  });
});