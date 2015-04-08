var mockPackage = require('./mocks/mockPackage');
var Dgeni = require('dgeni');

describe('jsdoc package', function() {
  it("should be instance of Package", function() {
      expect(require('./') instanceof Dgeni.Package).toBeTruthy();
  });

  describe("computePathsProcessor", function() {

    function runDgeni(docs) {
      var testPackage = new Dgeni.Package('testPackage', [mockPackage()])

        .processor('provideTestDocs', function() {
          return {
            $runBefore: ['files-read'],
            $process: function() {
              return docs;
            }
          };
        })

        .config(function(readFilesProcessor, writeFilesProcessor, computeIdsProcessor, renderDocsProcessor, unescapeCommentsProcessor) {
          readFilesProcessor.$enabled = false;
          writeFilesProcessor.$enabled = false;
          renderDocsProcessor.$enabled = false;
          unescapeCommentsProcessor.$enabled = false;
        });

      return new Dgeni([testPackage]).generate();
    }


    it("should compute the id and path of the document from its name or codeName", function(done) {
      var doc1 = { docType: 'js', fileInfo: { relativePath: 'a/b/c/foo.ngdoc', baseName: 'foo' }, name: 'fooName', codeName: 'fooCodeName', content: 'some content' };
      var doc2 = { docType: 'js', fileInfo: { relativePath: 'x/y/z/index.html', baseName: 'index' }, codeName: 'xyz', content: 'some content' };

        runDgeni([doc1,doc2]).then(function(docs) {
          expect(doc1.id).toEqual('fooName');
          expect(doc2.id).toEqual('xyz');
          expect(doc1.path).toEqual('fooName');
          expect(doc1.outputPath).toEqual('fooName.html');
          expect(doc2.path).toEqual('xyz');
          expect(doc2.outputPath).toEqual('xyz.html');
          done();
        }).catch(function(error) {
          console.log(error);
          done();
        });
    });


    it("should compute the id and path of the document from its file name", function(done) {
      var doc1 = { docType: 'js', fileInfo: { relativePath: 'a/b/c/foo.ngdoc', baseName: 'foo' }, content: 'some content'  };
      var doc2 = { docType: 'js', fileInfo: { relativePath: 'x/y/z/index.html', baseName: 'index' }, content: 'some content'  };

        runDgeni([doc1,doc2]).then(function(docs) {
          expect(doc1.path).toEqual('a/b/c/foo');
          expect(doc1.outputPath).toEqual('a/b/c/foo.html');
          expect(doc2.path).toEqual('x/y/z');
          expect(doc2.outputPath).toEqual('x/y/z.html');
          done();
        }).catch(function(error) {
          console.log(error);
          done();
        });
    });
  });
});