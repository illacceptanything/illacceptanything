var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("code utility", function() {
  var encodeCodeBlock;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    encodeCodeBlock = injector.get('encodeCodeBlock');
  });

  it("should wrap the string in code and pre tags", function() {
    expect(encodeCodeBlock('abc')).toEqual('<pre><code>abc</code></pre>');
  });

  it("should HTML encode the string", function() {
    expect(encodeCodeBlock('<div>&</div>')).toEqual('<pre><code>&lt;div&gt;&amp;&lt;/div&gt;</code></pre>');
  });

  it("should encode HTML entities", function() {
    expect(encodeCodeBlock('<div>&#10;</div>')).toEqual('<pre><code>&lt;div&gt;&amp;#10;&lt;/div&gt;</code></pre>');
  });

  describe("inline", function() {
    it("should only wrap in a code tag", function() {
      expect(encodeCodeBlock('abc', true)).toEqual('<code>abc</code>');
    });
  });

  describe("language", function() {
    it("should add a CSS class if a language is specified", function() {
      expect(encodeCodeBlock('abc', true, 'js')).toEqual('<code class="lang-js">abc</code>');
    });
  });
});