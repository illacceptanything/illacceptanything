var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("aliasMap", function() {
  var aliasMap;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    aliasMap = injector.get('aliasMap');
  });

  describe("addDoc", function() {
    it("should add the doc to an array for each alias", function() {
      var doc = { aliases: ['a', 'b', 'c'] };
      aliasMap.addDoc(doc);
      expect(aliasMap.getDocs('a')).toEqual([doc]);
      expect(aliasMap.getDocs('b')).toEqual([doc]);
      expect(aliasMap.getDocs('c')).toEqual([doc]);
    });

    it("should not add the doc if it has no aliases", function() {
      var doc = { };
      aliasMap.addDoc(doc);
      expect(aliasMap.getDocs('a')).toEqual([]);
      expect(aliasMap.getDocs('b')).toEqual([]);
      expect(aliasMap.getDocs('c')).toEqual([]);
    });
  });

  describe("getDocs", function() {
    it("should return an empty array if no doc matches the alias", function() {
      var doc = { aliases: ['a', 'b', 'c'] };
      expect(aliasMap.getDocs('d')).toEqual([]);
    });
  });

  describe("removeDoc", function() {
    it("should remove the doc from any parts of the aliasMap", function() {
      var doc1 = { aliases: ['a','b1'] };
      var doc2 = { aliases: ['a','b2'] };
      aliasMap.addDoc(doc1);
      aliasMap.addDoc(doc2);

      expect(aliasMap.getDocs('a')).toEqual([doc1, doc2]);
      expect(aliasMap.getDocs('b1')).toEqual([doc1]);
      expect(aliasMap.getDocs('b2')).toEqual([doc2]);

      aliasMap.removeDoc(doc1);

      expect(aliasMap.getDocs('a')).toEqual([doc2]);
      expect(aliasMap.getDocs('b1')).toEqual([]);
      expect(aliasMap.getDocs('b2')).toEqual([doc2]);

    });
  });
});