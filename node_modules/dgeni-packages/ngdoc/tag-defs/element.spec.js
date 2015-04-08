var tagDefFactory = require('./element');

describe("element tag-def", function() {
  it("should set default based on docType", function() {

     var tagDef =  tagDefFactory();
     expect(tagDef.defaultFn({ docType: 'directive' })).toEqual('ANY');
     expect(tagDef.defaultFn({ docType: 'input' })).toEqual('ANY');
     expect(tagDef.defaultFn({ docType: 'service' })).toBeUndefined();

  });
});