var filters = require('./change-case');
var _ = require('lodash');

var dashCase = _.find(filters, function(filter) { return filter.name === 'dashCase'; });

describe("dashCase custom filter", function() {
  it("should have the name 'dashCase'", function() {
    expect(dashCase.name).toEqual('dashCase');
  });
  it("should transform the content to dash-case", function() {
    expect(dashCase.process('fooBar')).toEqual('foo-bar');
  });
});

