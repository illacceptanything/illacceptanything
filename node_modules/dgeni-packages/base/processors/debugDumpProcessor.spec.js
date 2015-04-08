var path = require('canonical-path');
var Q = require('q');

var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');


describe("debugDumpProcessor", function() {
  it("should write out the docs to a file", function() {

    var writeFileSpy = jasmine.createSpy('writeFile').and.returnValue(Q());
    var testPackage = mockPackage().factory('writeFile', function() { return writeFileSpy; });

    var dgeni = new Dgeni([testPackage]);
    var injector = dgeni.configureInjector();

    var readFilesProcessor = injector.get('readFilesProcessor');
    readFilesProcessor.basePath = path.resolve('some/path');

    var processor = injector.get('debugDumpProcessor');

    processor.outputPath = 'build/dump.txt';
    processor.$process([{ val: 'a' }, { val: 'b' }]);
    expect(writeFileSpy).toHaveBeenCalledWith(path.resolve('some/path/build/dump.txt'), "[ { val: 'a' }, { val: 'b' } ]");
  });
});