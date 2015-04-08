var ngdocPackage = require('./');
var Dgeni = require('dgeni');
var mockLog = require('dgeni/lib/mocks/log');

describe('ngdoc package', function() {
  it("should be instance of Package", function() {
      expect(ngdocPackage instanceof Dgeni.Package).toBeTruthy();
  });

  function runDgeni(docs) {
    var testPackage = new Dgeni.Package('testPackage', [ngdocPackage])
      .factory('log', function() { return mockLog(false); })
      .processor('provideTestDocs', function() {
        return {
          $runBefore: ['computePathsProcessor'],
          $process: function() {
            return docs;
          }
        };
      })

      .config(function(readFilesProcessor, writeFilesProcessor, renderDocsProcessor, unescapeCommentsProcessor) {
        readFilesProcessor.$enabled = false;
        writeFilesProcessor.$enabled = false;
        renderDocsProcessor.$enabled = false;
        unescapeCommentsProcessor.$enabled = false;
      });

    return new Dgeni([testPackage]).generate();
  }


  it("should compute the path of components from their attributes", function(done) {
    var docTypes = ['service', 'provider', 'directive', 'input', 'function', 'filter', 'type'];
    var docs = docTypes.map(function(docType) {
      return { docType: docType, area: 'AREA', module: 'MODULE', name: 'NAME' };
    });

    runDgeni(docs).then(function(docs) {
      for(var i=0; i<docs.length; i++) {
        expect(docs[i].path).toEqual('AREA/MODULE/' + docs[i].docType + '/NAME');
        expect(docs[i].outputPath).toEqual('partials/AREA/MODULE/' + docs[i].docType + '/NAME.html');
      }
      done();
    }, function(err) {
      console.log(err);
      throw err;
    });
  });



  it("should compute the path of modules from their attributes", function(done) {
    var doc = { docType: 'module', area: 'AREA', name: 'MODULE' };

    runDgeni([doc]).then(function(docs) {
      expect(docs[0].path).toEqual('AREA/MODULE');
      expect(docs[0].outputPath).toEqual('partials/AREA/MODULE/index.html');
      done();
    }, function(err) {
      console.log(err);
      throw err;
    });
  });




  it("should compute the path of component groups from their attributes", function(done) {
    var groupTypes = ['service', 'provider', 'directive', 'input', 'function', 'filter', 'type'];
    var docs = groupTypes.map(function(groupType) {
      return { docType: 'componentGroup', area: 'AREA', groupType: groupType, moduleName: 'MODULE' };
    });

    runDgeni(docs).then(function(docs) {
      for(var i=0; i<docs.length; i++) {
        expect(docs[i].path).toEqual('AREA/MODULE/' + docs[i].groupType);
        expect(docs[i].outputPath).toEqual('partials/AREA/MODULE/' + docs[i].groupType + '/index.html');
      }
      done();
    }, function(err) {
      console.log(err);
      throw err;
    });
  });
});

