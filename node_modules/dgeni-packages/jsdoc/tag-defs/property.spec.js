var tagDefFactory = require('./property');

describe("property tagDef", function() {
  it("should add the injected transforms to the transforms property", function() {
    var extractNameTransform = function() {};
    var extractTypeTransform = function() {};
    var wholeTagTransform = function() {};

    var tagDef = tagDefFactory(extractTypeTransform, extractNameTransform, wholeTagTransform);
    expect(tagDef.transforms).toEqual([extractTypeTransform, extractNameTransform, wholeTagTransform]);
  });
});