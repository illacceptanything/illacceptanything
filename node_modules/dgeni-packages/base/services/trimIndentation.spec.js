var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("trimIndentation", function() {
  var trimIndentation;
  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    trimIndentation = injector.get('trimIndentation');
  });
  it("should trim simple leading white-space from a single line of text", function() {
    expect(trimIndentation('   abc  ')).toEqual('abc  ');
  });
  it("should trim excess indentation from multi-line text ", function() {
    expect(trimIndentation('abc\n     xyz\n     123\n\n')).toEqual('abc\nxyz\n123');
    expect(trimIndentation('  abc\n     xyz\n     123\n\n')).toEqual('abc\n   xyz\n   123');
    expect(trimIndentation(' abc\n  xyz\n   123\n\n')).toEqual('abc\n xyz\n  123');
  });
  it("should remove leading empty lines", function() {
    expect(trimIndentation('\n\n\nabc')).toEqual('abc');
    expect(trimIndentation('\n\n\n   abc')).toEqual('abc');
  });
  it("should remove trailing empty lines", function() {
    expect(trimIndentation('abc\n\n\n')).toEqual('abc');
  });

  it("should not trim indentation if more than the first line is not indented", function() {
    expect(trimIndentation(
      '.ng-hide {\n' +
      '  /&#42; this is just another form of hiding an element &#42;/\n' +
      '  display:block!important;\n' +
      '  position:absolute;\n' +
      '  top:-9999px;\n' +
      '  left:-9999px;\n' +
      '}')).toEqual(
      '.ng-hide {\n' +
      '  /&#42; this is just another form of hiding an element &#42;/\n' +
      '  display:block!important;\n' +
      '  position:absolute;\n' +
      '  top:-9999px;\n' +
      '  left:-9999px;\n' +
      '}');
  });

  it("should cope with an empty code block", function() {
    expect(trimIndentation('\n\n')).toEqual('');
  });

  describe("calcIndent", function() {
    it("should calculate simple leading white-space from a single line of text", function() {
      expect(trimIndentation.calcIndent('   abc  ')).toEqual(3);
    });
    it("should trim excess indentation from multi-line text ", function() {
      expect(trimIndentation.calcIndent('abc\n     xyz\n     123\n\n')).toEqual(5);
      expect(trimIndentation.calcIndent('  abc\n     xyz\n     123\n\n')).toEqual(2);
      expect(trimIndentation.calcIndent(' abc\n  xyz\n   123\n\n')).toEqual(1);
    });
    it("should cope with an empty code block", function() {
      expect(trimIndentation.calcIndent('\n\n')).toEqual(9999);
    });
  });

  describe("reindent", function() {
    it("should add whitespace to the start of each line", function() {
      expect(trimIndentation.reindent('abc\n  xyz', 4)).toEqual('    abc\n      xyz');
    });
  });
});