var transformFactory = require('./whole-tag');

describe("whole-tag transform", function() {
  it("should return the whole tag", function() {
    var transform = transformFactory();
    var doc = {}, tag = {}, value = {};
    expect(transform(doc, tag, value)).toBe(tag);
  });
});