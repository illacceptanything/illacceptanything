var tagDefFactory = require('./restrict');

describe("restrict tag-def", function() {
  var tagDef;

  beforeEach(function() {
    tagDef = tagDefFactory();
  });

  it("should convert a restrict tag text to an object", function() {
    expect(tagDef.transforms({}, {}, 'A')).toEqual({ element: false, attribute: true, cssClass: false, comment: false });
    expect(tagDef.transforms({}, {}, 'C')).toEqual({ element: false, attribute: false, cssClass: true, comment: false });
    expect(tagDef.transforms({}, {}, 'E')).toEqual({ element: true, attribute: false, cssClass: false, comment: false });
    expect(tagDef.transforms({}, {}, 'M')).toEqual({ element: false, attribute: false, cssClass: false, comment: true });
    expect(tagDef.transforms({}, {}, 'ACEM')).toEqual({ element: true, attribute: true, cssClass: true, comment: true });
  });

  it("should default to restricting to an attribute if no tag is found and the doc is for a directive", function() {
    expect(tagDef.defaultFn({ docType: 'directive' })).toEqual({ element: false, attribute: true, cssClass: false, comment: false });
    expect(tagDef.defaultFn({ docType: 'input' })).toEqual({ element: false, attribute: true, cssClass: false, comment: false });
  });

  it("should not add a restrict property if the docType is not 'directive'", function() {
    expect(tagDef.defaultFn({ docType: 'service' })).toBeUndefined();
  });
});

