var codeFilterFactory = require('./code');

describe("code custom filter", function() {

  var codeFilter, codeSpy;

  beforeEach(function() {
    codeSpy = jasmine.createSpy('code').and.callFake(function(value) { return '<code>' + value + '</code>'; });
    codeFilter = codeFilterFactory(codeSpy);
  });

  it("should have the name 'code'", function() {
    expect(codeFilter.name).toEqual('code');
  });


  it("should call the code utility", function() {
    codeFilter.process('function foo() { }');
    expect(codeSpy).toHaveBeenCalledWith('function foo() { }', true, undefined);
  });


  it("should pass the language to the code utility", function() {
    codeFilter.process('function foo() { }', 'js');
    expect(codeSpy).toHaveBeenCalledWith('function foo() { }', true, 'js');
  });
});