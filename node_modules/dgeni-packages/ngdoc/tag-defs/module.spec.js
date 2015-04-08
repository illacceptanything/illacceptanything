var path = require('canonical-path');
var tagDefFactory = require('./module');

describe("module tag-def", function() {
  it("should calculate the module from the second segment of the file path", function() {
     var tagDef = tagDefFactory();
     expect(tagDef.defaultFn({ area: 'api', fileInfo: { relativePath: 'ng/service/$http.js' } })).toEqual('ng');
  });

  it("should use the relative file path", function() {
     var tagDef = tagDefFactory();
     var relativePath = 'ng/service/$http.js';
     expect(tagDef.defaultFn({ area: 'api', fileInfo: { filePath: path.resolve(relativePath), relativePath: relativePath } })).toEqual('ng');
  });

  it("should not calculate module if the doc is not in 'api' area", function() {
     var tagDef = tagDefFactory();
     var relativePath = 'guide/concepts.ngdoc';
     expect(tagDef.defaultFn({ area: 'guide', fileInfo: { filePath: path.resolve(relativePath), relativePath: relativePath } })).toBeUndefined();
  });

  it("should not calculate module if the doc has docType 'overview'", function() {
     var tagDef = tagDefFactory();
     var relativePath = 'api/index.ngdoc';
     expect(tagDef.defaultFn({ docType: 'overview', area: 'api', fileInfo: { filePath: path.resolve(relativePath), relativePath: relativePath } })).toBeUndefined();
  });
});