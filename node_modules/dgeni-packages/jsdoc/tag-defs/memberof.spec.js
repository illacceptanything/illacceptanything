var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');
var tagDefFactory = require('./memberof');

describe("memberof tag-def", function() {
  var tagDef;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    tagDef = injector.invoke(tagDefFactory);
  });

  describe('transforms', function() {
    it("should throw an exception if the docType is not 'event', 'method' or 'property'", function() {
      expect(function() {
        tagDef.transforms({ docType: 'unknown'});
      }).toThrowError();

      expect(function() {
        tagDef.transforms({ docType: 'event'});
      }).not.toThrowError();

      expect(function() {
        tagDef.transforms({ docType: 'method'});
      }).not.toThrowError();

      expect(function() {
        tagDef.transforms({ docType: 'property'});
      }).not.toThrowError();
    });
  });


  describe("defaultFn", function() {
    it("should throw an exception if the docType is 'event', 'method' or 'property'", function() {
      expect(function() {
        tagDef.defaultFn({ docType: 'unknown'});
      }).not.toThrowError();

      expect(function() {
        tagDef.defaultFn({ docType: 'event'});
      }).toThrowError();

      expect(function() {
        tagDef.defaultFn({ docType: 'method'});
      }).toThrowError();

      expect(function() {
        tagDef.defaultFn({ docType: 'property'});
      }).toThrowError();
    });
  });
});