var codeNameProcessorFactory = require('./code-name');
var jsParser = require('esprima');

var mockLog = jasmine.createSpyObj('log', ['error', 'warn', 'info', 'debug', 'silly']);

describe('code-name doc processor', function() {
  it("should understand CallExpressions", function() {
    var processor = codeNameProcessorFactory(mockLog);
    var ast = jsParser.parse('(function foo() { })()');
    var doc = { codeNode: ast };
    processor.$process([doc]);
    expect(doc.codeName).toEqual('foo');
  });

  it("should understand ArrayExpressions", function() {
    var processor = codeNameProcessorFactory(mockLog);
    var ast = jsParser.parse("$CompileProvider.$inject = ['$provide', '$$sanitizeUriProvider'];");
    var doc = { codeNode: ast };
    processor.$process([doc]);
    expect(doc.codeName).toEqual('$inject');
  });

});