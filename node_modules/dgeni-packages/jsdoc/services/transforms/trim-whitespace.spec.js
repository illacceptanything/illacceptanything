var transformFactory = require('./trim-whitespace');

describe("trim-whitespace", function() {

  var transform;

  beforeEach(function() {
    transform = transformFactory();
  });

  it("should trim newlines and whitespace from the end of the description", function() {
    expect(transform({}, {}, 'myId\n\nsome other text  \n  \n')).toEqual('myId\n\nsome other text');
  });

  it("should not do anything if the value is not a string", function() {
    var someNonStringObject = {};
    expect(transform({}, {}, someNonStringObject)).toEqual(someNonStringObject);
  });

});
