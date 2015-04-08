var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

var getLinkInfo, aliasMap, getAliases;

describe("getLinkInfo", function() {
  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();

    aliasMap = injector.get('aliasMap');
    getAliases = injector.get('getAliases');
    getLinkInfo = injector.get('getLinkInfo');
  });

  it("should lookup urls against the docs", function() {
    var doc = { id: 'module:ng.directive:ngClick', name: 'ngClick', path: 'api/ng/directive/ngClick' };
    doc.aliases = getAliases(doc);
    aliasMap.addDoc(doc);

    expect(getLinkInfo('ngClick')).toEqual({
      type: 'doc',
      valid: true,
      url: 'api/ng/directive/ngClick',
      title: '<code>ngClick</code>'
    });

    expect(getLinkInfo('ngClick', 'Click Event')).toEqual({
      type: 'doc',
      valid: true,
      url: 'api/ng/directive/ngClick',
      title: 'Click Event'
    });

    expect(getLinkInfo('ngClick#some-header', 'Click Event')).toEqual({
      type: 'doc',
      valid: true,
      url: 'api/ng/directive/ngClick#some-header',
      title: 'Click Event'
    });

  });

  it("should error if there are multiple docs with the same name", function() {
    var doc1 = { id: 'module:ng.directive:ngClick', name: 'ngClick', path: 'api/ng/directive/ngClick' };
    doc1.aliases = getAliases(doc1);
    aliasMap.addDoc(doc1);

    var doc2 = { id: 'module:ngTouch.directive:ngClick', name: 'ngClick', path: 'api/ngTouch/directive/ngClick' };
    doc2.aliases = getAliases(doc2);
    aliasMap.addDoc(doc2);

    expect(getLinkInfo('ngClick').error).toMatch(/Ambiguous link:/);
  });

  it("should error if no docs match the link", function() {
    expect(getLinkInfo('ngClick').error).toEqual('Invalid link (does not match any doc): "ngClick"');
  });

  it("should not error if the link is a URL or starts with a hash", function() {
    expect(getLinkInfo('some/path').error).toBeUndefined();
    expect(getLinkInfo('some/path').title).toEqual('path');
    expect(getLinkInfo('#fragment').error).toBeUndefined();
    expect(getLinkInfo('#fragment').title).toEqual('fragment');
  });

  it("should filter ambiguous documents by area before failing", function() {
    var doc1 = { id: 'module:ng.directive:ngClick', name: 'ngClick', path: 'api/ng/directive/ngClick', area: 'api' };
    doc1.aliases = getAliases(doc1);
    aliasMap.addDoc(doc1);

    var doc2 = { id: 'ngClick', name: 'ngClick', path: 'guide/ngClick', area: 'guide' };
    doc2.aliases = getAliases(doc2);
    aliasMap.addDoc(doc2);

    expect(getLinkInfo('ngClick', 'ngClick Guide', doc2)).toEqual({
      type: 'doc',
      valid: true,
      url: 'guide/ngClick',
      title: 'ngClick Guide'
    });
  });
});