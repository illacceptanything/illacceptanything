var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("unescapeCommentsProcessor", function() {
  it("should convert HTML encoded comments back to their original form", function() {

    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    var processor = injector.get('unescapeCommentsProcessor');

    var doc = {
      renderedContent: 'Some text containing /&amp;#42; a comment &amp;#42;/\nSome text containing /&amp;#42; a comment &amp;#42;/'
    };
    processor.$process([doc]);
    expect(doc.renderedContent).toEqual('Some text containing /* a comment */\nSome text containing /* a comment */');
  });
});