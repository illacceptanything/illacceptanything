var tagDefFactories = require('./');

describe("jsdoc tagdefs", function() {
  it("should contain an array of tagDef factory functions", function() {
    expect(tagDefFactories).toEqual(jasmine.any(Array));
    expect(tagDefFactories.length).toEqual(22);
    tagDefFactories.forEach(function(factory) {
      expect(factory).toEqual(jasmine.any(Function));
    });
  });
});