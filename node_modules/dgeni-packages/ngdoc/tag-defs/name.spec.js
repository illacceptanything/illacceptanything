var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

var tagDefFactory = require('./name');

describe("name tag-def", function() {
  var tagDef;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    tagDef = injector.invoke(tagDefFactory);
  });

  it("should update the inputType if docType is input", function() {
    var doc = { docType: 'input' };
    var tag = {};
    var value = tagDef.transforms(doc, tag, 'input[checkbox]');
    expect(value).toEqual('input[checkbox]');
    expect(doc.inputType).toEqual('checkbox');
  });

  it("should not update the inputType if docType is not input", function() {
    var doc = { docType: 'directive' };
    var tag = {};
    var value = tagDef.transforms(doc, tag, 'input[checkbox]');
    expect(value).toEqual('input[checkbox]');
    expect(doc.inputType).toBeUndefined();
  });

  it("should throw error if the docType is 'input' and the name is not a valid format", function() {
    var doc = { docType: 'input' };
    var tag = {};
    expect(function() {
      tagDef.transforms(doc, tag, 'invalidInputName');
    }).toThrow();
  });

});
