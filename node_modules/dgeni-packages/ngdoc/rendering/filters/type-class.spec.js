var filterFactory = require('./type-class');

describe("type-class filter", function() {
  it("should call getTypeClass", function() {
    var getTypeClassSpy = jasmine.createSpy('getTypeClass');
    var filter = filterFactory(getTypeClassSpy);

    filter.process('object');
    expect(getTypeClassSpy).toHaveBeenCalled();
  });
});