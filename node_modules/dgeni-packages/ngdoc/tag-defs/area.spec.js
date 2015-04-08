var tagDefFactory = require('./area');

describe("area tag-def", function() {
  it("should set default based on fileType", function() {
     var tagDef =  tagDefFactory();
     expect(tagDef.defaultFn({ fileInfo: { extension: 'js' } })).toEqual('api');
     expect(tagDef.defaultFn({ fileInfo: { relativePath: 'guide/concepts.ngdoc' } })).toEqual('guide');
  });
});