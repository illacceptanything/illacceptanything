var tagDefFactory = require('./scope');

describe("scope tag-def", function() {
  it("should transform the value to true", function() {
    var tagDef = tagDefFactory();
    expect(tagDef.transforms()).toEqual(true);
  });
});