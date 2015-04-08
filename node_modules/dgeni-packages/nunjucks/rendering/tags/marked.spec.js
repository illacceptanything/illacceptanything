var Dgeni = require('dgeni');
var mockPackage = require('../../mocks/mockPackage');

describe("marked custom tag extension", function() {
  var extension;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();

    extension = injector.get('markedNunjucksTag');
  });

  it("should specify the tags to match", function() {
    expect(extension.tags).toEqual(['marked']);
  });

  describe("process", function() {

    it("should render the markdown and reindent", function() {
      var result = extension.process(null, function() {
        return '  ## heading 2\n\n' +
               '  some paragraph\n\n' +
               '    * a bullet point';
      });
      expect(result).toEqual(
        '  <h2 id="heading-2">heading 2</h2>\n' +
        '  <p>some paragraph</p>\n' +
        '  <ul>\n' +
        '  <li>a bullet point</li>\n' +
        '  </ul>\n' +
        '  '
      );
    });

  });

  describe("parse", function() {
    it("should interact correctly with the parser", function() {
      var log = [];
      var parserMock = {
        advanceAfterBlockEnd: function() { log.push('advanceAfterBlockEnd'); },
        parseUntilBlocks: function() { log.push('parseUntilBlocks'); return 'some content'; }
      };
      var nodesMock = {
        CallExtension: function() { log.push('CallExtension'); this.args = arguments; }
      };

      var tag = extension.parse(parserMock, nodesMock);

      expect(log).toEqual([
        'advanceAfterBlockEnd',
        'parseUntilBlocks',
        'CallExtension',
        'advanceAfterBlockEnd'
      ]);

      expect(tag.args[0]).toEqual(extension);
      expect(tag.args[1]).toEqual('process');
      expect(tag.args[3]).toEqual(['some content']);
    });
  });
});