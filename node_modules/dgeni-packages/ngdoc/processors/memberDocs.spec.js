var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("memberDocsProcessor", function() {
  var processor, aliasMap, mockLog;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    processor = injector.get('memberDocsProcessor');
    aliasMap = injector.get('aliasMap');
    mockLog = injector.get('log');
  });

  it("should remove docs that are members of container docs", function() {

    var doc1 = { id: 'module:ng.service:$log', aliases: ['$log', 'service:$log', 'ng.$log', 'module:ng.service:$log', 'ng.service:$log'] };
    var doc2 = { id: 'module:ngMock.service:$log', aliases: ['$log', 'service:$log', 'ngMock.$log', 'module:ngMock.service:$log', 'ngMock.service:$log'] };
    var doc3 = { id: 'ng.$log#warn' };
    var docs = [doc1, doc2, doc3];

    aliasMap.addDoc(doc1);
    aliasMap.addDoc(doc2);
    docs = processor.$process(docs);

    expect(docs).toEqual([doc1,doc2]);

  });

  it("should connect member docs to their container doc", function() {

    var doc1 = { id: 'module:ng.service:$log', aliases: ['$log', 'service:$log', 'ng.$log', 'module:ng.service:$log', 'ng.service:$log'] };
    var doc2 = { id: 'module:ngMock.service:$log', aliases: ['$log', 'service:$log', 'ngMock.$log', 'module:ngMock.service:$log', 'ngMock.service:$log'] };
    var doc3 = { id: 'ng.$log#warn', docType: 'method' };
    var docs = [doc1, doc2, doc3];

    aliasMap.addDoc(doc1);
    aliasMap.addDoc(doc2);
    docs = processor.$process(docs);

    expect(doc3.name).toEqual('warn');
    expect(doc3.memberof).toEqual('module:ng.service:$log');
    expect(doc1.methods).toEqual([doc3]);
    expect(doc2.methods).not.toEqual([doc3]);

  });

  it("should attempt to match the container by using the member's module", function() {
    var doc1 = { module: 'ng', id: 'module:ng.service:$log', aliases: ['$log', 'service:$log', 'ng.$log', 'module:ng.service:$log', 'ng.service:$log'] };
    var doc2 = { module: 'ngMock', id: 'module:ngMock.service:$log', aliases: ['$log', 'service:$log', 'ngMock.$log', 'module:ngMock.service:$log', 'ngMock.service:$log'] };
    var doc3 = { module: 'ngMock', id: '$log#warn', docType: 'method' };

    aliasMap.addDoc(doc1);
    aliasMap.addDoc(doc2);

    processor.$process([doc3]);
    expect(doc3.memberof).toEqual('module:ngMock.service:$log');
    expect(doc2.methods).toEqual([doc3]);
    expect(doc1.methods).not.toEqual([doc3]);

  });

  it("should warn if the container doc does not exist or is ambiguous", function() {

    var doc1 = { module: 'ng', id: 'module:ng.service:orderBy', aliases: ['orderBy', 'service:orderBy', 'ng.orderBy', 'module:ng.service:orderBy', 'ng.service:orderBy'] };
    var doc2 = { module: 'ng', id: 'module:ng.filter:orderBy', aliases: ['orderBy', 'filter:orderBy', 'ng.orderBy', 'module:ng.filter:orderBy', 'ng.service:orderBy'] };
    var doc3 = { module: 'ng', id: 'ng.$http#get', docType: 'method' };
    var doc4 = { module: 'ng', id: 'orderBy#doIt', docType: 'method' };

    aliasMap.addDoc(doc1);
    aliasMap.addDoc(doc2);

    processor.$process([doc3]);
    expect(mockLog.warn).toHaveBeenCalled();
    expect(mockLog.warn.calls.mostRecent().args[0]).toMatch(/Missing container document/);
    mockLog.warn.calls.reset();

    processor.$process([doc4]);
    expect(mockLog.warn).toHaveBeenCalled();
    expect(mockLog.warn.calls.mostRecent().args[0]).toMatch(/Ambiguous container document reference/);

  });
});