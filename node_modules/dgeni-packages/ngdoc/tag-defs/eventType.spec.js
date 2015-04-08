var tagDefFactory = require('./eventType');

describe("eventType tag-def", function() {
  it("should split into eventType and eventTarget", function() {
    var doc = {}, tag = {};
    var tagDef = tagDefFactory();
    var value = tagDef.transforms(doc, tag, 'broadcast on module:ng.directive:ngInclude');
    expect(value).toEqual('broadcast');
    expect(doc.eventTarget).toEqual('module:ng.directive:ngInclude');
  });
});