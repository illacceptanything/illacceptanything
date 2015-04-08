var transformFactory = require('./unknown-tag');

describe("unknown-tag transform", function() {
  it("should add an error to the tag if it has no tagDef", function() {
    var doc = {}, tag = { tagName: 'bad-tag'};
    var transform = transformFactory();
    transform(doc, tag);
    expect(tag.errors).toEqual(['Unknown tag: bad-tag']);
  });
});