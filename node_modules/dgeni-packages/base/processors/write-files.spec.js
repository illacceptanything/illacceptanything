var path = require('canonical-path');
var Q = require('q');

var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("writeFilesProcessor", function() {
  var processor, writeFileSpy, mockLog;


  beforeEach(function() {
    writeFileSpy = jasmine.createSpy('writeFile').and.returnValue(Q());

    var testPackage = mockPackage().factory('writeFile', function() { return writeFileSpy; });

    var dgeni = new Dgeni([testPackage]);
    var injector = dgeni.configureInjector();

    var readFilesProcessor = injector.get('readFilesProcessor');
    readFilesProcessor.basePath = path.resolve('some/path');

    processor = injector.get('writeFilesProcessor');
    processor.outputFolder = 'build';

    mockLog = injector.get('log');
  });

  it("should write each document to a file", function() {
    processor.$process([{ renderedContent: 'SOME RENDERED CONTENT', outputPath: 'doc/path.html' }]);
    expect(writeFileSpy).toHaveBeenCalledWith(path.resolve('some/path/build/doc/path.html'), 'SOME RENDERED CONTENT');
  });

  it("should log a debug message if a doc has no outputPath", function() {
    processor.$process([{ renderedContent: 'SOME RENDERED CONTENT', id: 'doc1', docType: 'test' }]);
    expect(mockLog.debug).toHaveBeenCalledWith('Document "doc1, test" has no outputPath.');
  });
});