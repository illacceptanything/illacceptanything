var tagDefFactory = require('./description');

describe("description tag-def", function() {
  var tagDef;

  beforeEach(function() {
    tagDef = tagDefFactory();
  });

  describe('transforms', function() {
    it("should prepend any non-tag specific description found in the jsdoc comment", function() {
      var doc = { tags: { description: 'general description'} };
      var tag = {};
      var value = "tag specific description";
      expect(tagDef.transforms(doc, tag, value)).toEqual('general description\ntag specific description');
    });
  });


  describe("defaultFn", function() {
    it("should get the contents of the non-tag specific description", function() {
      var doc = { tags: { description: 'general description'} };
      expect(tagDef.defaultFn(doc)).toEqual('general description');
    });
  });
});