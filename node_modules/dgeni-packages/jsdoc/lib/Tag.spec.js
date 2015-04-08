var Tag = require('./Tag');

describe("Tag", function() {
  it("should put constructor parameters into member properties", function() {
    var tagDef = {};
    var tag = new Tag(tagDef, 'someName', 'a load of content', 12);
    expect(tag.tagDef).toBe(tagDef);
    expect(tag.tagName).toEqual('someName');
    expect(tag.description).toEqual('a load of content');
    expect(tag.startingLine).toEqual(12);
  });
});