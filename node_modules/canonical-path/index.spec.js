var path = require('./index');
describe("canonical-path", function() {
  describe("normalize", function() {
    it("should return a normalized path only using forward slashes", function() {
      expect(path.normalize('a/c/../b')).toEqual('a/b');
      expect(path.normalize('a\\c\\..\\b')).toEqual('a/b');
    });
  });

  describe("join", function() {
    it("should join paths only using forward slashes", function() {
      expect(path.join('a/b', 'c/d')).toEqual('a/b/c/d');
      expect(path.join('a\\b', 'c\\d')).toEqual('a/b/c/d');
    });
  });

  describe("canonical", function() {
    it("should return a path with forward slashes", function() {
      expect(path.canonical('a'+path.sep+'b'+path.sep+'c')).toEqual('a/b/c');
    });
  });
});