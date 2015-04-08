var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("extractLinks", function() {
  var urlExtractor;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    extractLinks = injector.get('extractLinks');
  });

  it("should extract the hrefs from anchors", function() {
    expect(extractLinks('<a href="foo">bar</a>').hrefs).toEqual(['foo']);
    expect(extractLinks('<a href="foo">bar</a><a href="man">shell</a>').hrefs).toEqual(['foo', 'man']);
    expect(extractLinks('<div href="foo">bar</div>').hrefs).toEqual([]);
  });

  it("should extract the names from anchors", function() {
    expect(extractLinks('<a name="foo">bar</a><a href="man">shell</a>').names).toEqual(['foo']);
    expect(extractLinks('<div name="foo">bar</div>').names).toEqual([]);
  });

  it("should extract the ids from elements", function() {
    expect(extractLinks('<a id="foo">bar</a><a href="man">shell</a>').names).toEqual(['foo']);
    expect(extractLinks('<div id="foo">bar</div>').names).toEqual(['foo']);
  });
});
