var examplesPackage = require('./index');
var ngdocPackage = require('../ngdoc/index');

var Dgeni = require('dgeni');
var mockLog = require('dgeni/lib/mocks/log');



describe('examples package', function() {
  it("should be instance of Package", function() {
      expect(examplesPackage instanceof Dgeni.Package).toBeTruthy();
  });


  function runDgeni(docs) {
    var testPackage = new Dgeni.Package('testPackage', [examplesPackage, ngdocPackage])
      .factory('log', function() { return mockLog(false); })
      .processor('provideTestDocs', function() {
        return {
          $runBefore: ['parseExamplesProcessor'],
          $process: function() {
            return docs;
          }
        };
      })

      .config(function(readFilesProcessor, writeFilesProcessor, renderDocsProcessor, unescapeCommentsProcessor, generateProtractorTestsProcessor) {
        readFilesProcessor.$enabled = false;
        writeFilesProcessor.$enabled = false;
        renderDocsProcessor.$enabled = false;
        unescapeCommentsProcessor.$enabled = false;
        generateProtractorTestsProcessor.$enabled = false;
      })

      .config(function(generateExamplesProcessor) {
        generateExamplesProcessor.deployments = [ {
          name: 'testDeployment',
          examples: {
            commonFiles: {
              scripts: [ '../../../dep1.js' ]
            },
            dependencyPath: '../../../'
          },
          scripts: [
            '../dep1.js',
            '../dep2.js'
          ],
          stylesheets: [
            'style1.css',
            'style2.css'
          ]
        }];
      });

    return new Dgeni([testPackage]).generate();
  }

  function processExample() {
    var doc = {
      content:
        '/** @ngdoc service\n' +
        ' * @description\n' +
        ' * <example name="testExample">\n' +
        ' *   <file name="app.js">some code</file>\n' +
        ' * </example>\n' +
        ' */',
      fileInfo: { relativePath: 'a.js', baseName: 'a' }
    };

    return runDgeni([doc]).then(null, function(err) {
      console.log("ERROR:", err);
    });
  }


  it("should compute the path of examples from their attributes", function(done) {
    processExample().then(function(docs) {

      expect(docs.length).toEqual(4);

      expect(docs[0].id).toEqual('example-testExample/app.js');
      expect(docs[0].path).toEqual('app.js');
      expect(docs[0].outputPath).toEqual('examples/example-testExample/app.js');

      expect(docs[1].id).toEqual('example-testExample-testDeployment');
      expect(docs[1].path).toEqual('examples/example-testExample');
      expect(docs[1].outputPath).toEqual('examples/example-testExample/index-testDeployment.html');

      expect(docs[2].id).toEqual('example-testExample-runnableExample');
      expect(docs[2].path).toEqual('examples/example-testExample');
      expect(docs[2].outputPath).toBeUndefined();

      expect(docs[3].id).toEqual('example-testExample/manifest.json');
      expect(docs[3].path).toBeUndefined();
      expect(docs[3].outputPath).toEqual('examples/example-testExample/manifest.json');

      done();
    });
  });
});
