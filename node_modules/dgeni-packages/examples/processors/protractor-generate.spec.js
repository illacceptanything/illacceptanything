var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

var _ = require('lodash');

describe("generateExamplesProcessor", function() {

  var processor, exampleMap;

  beforeEach(function() {

    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();

    processor = injector.get('generateProtractorTestsProcessor');
    processor.templateFolder = 'examples';
    processor.deployments = [
      {
        name: 'default',
        examples: { commonFiles: [], dependencyPath: '.' },
      },
      {
        name: 'other',
        examples: { commonFiles: { scripts: [ 'someFile.js', 'someOtherFile.js' ], }, dependencyPath: '..' }
      }
    ];

    exampleMap = injector.get('exampleMap');
  });


  it("should add the configured basePath to each doc", function() {

    exampleMap.set('x', {
      id: 'x',
      doc: {},
      files: {
        'app.scenario.js': { type: 'protractor', name: 'app.scenario.js', contents: '...' }
      },
      deployments: {}
    });
    var docs = [];
    processor.$process(docs);
    expect(docs[0].basePath).toEqual('');
    expect(docs[1].basePath).toEqual('');

    processor.basePath = 'a/b/';
    processor.$process(docs);

    expect(docs[2].basePath).toEqual('a/b/');
    expect(docs[3].basePath).toEqual('a/b/');

  });


  it("should add a protractor doc for each example-deployment pair in the example", function() {

    docs = [
      { file: 'a.b.c.js' },
      { file: 'x.y.z.js' }
    ];

    exampleMap.set('a.b.c', {
      id: 'a.b.c',
      doc: docs[0],
//      outputFolder: 'examples',
//      deps: 'dep1.js;dep2.js',
      files: {
        'index.html': { type: 'html', name: 'index.html', fileContents: 'index.html content' },
        'app.scenario.js': { type: 'protractor', name: 'app.scenario.js', fileContents: 'app.scenario.js content' }
      },
      deployments: {}
    });

    exampleMap.set('x.y.z', {
      id: 'x.y.z',
      doc: docs[1],
      // outputFolder: 'examples',
      // deps: 'dep1.js;dep2.js',
      files: {
        'index.html': { type: 'html', name: 'index.html', fileContents: 'index.html content' },
        'app.scenario.js': { type: 'protractor', name: 'app.scenario.js', fileContents: 'app.scenario.js content' }
      },
      deployments: {},
      'ng-app-included': true
    });

    processor.$process(docs);

    expect(_.filter(docs, { docType: 'e2e-test' })).toEqual([
      jasmine.objectContaining({
        docType: 'e2e-test',
        id: 'protractorTest-a.b.c-' + processor.deployments[0].name,
        example: exampleMap.get('a.b.c'),
        deployment: processor.deployments[0],
        template: 'protractorTests.template.js',
        innerTest: 'app.scenario.js content'
      }),
      jasmine.objectContaining({
        docType: 'e2e-test',
        id: 'protractorTest-a.b.c-' + processor.deployments[1].name,
        example: exampleMap.get('a.b.c'),
        deployment: processor.deployments[1],
        template: 'protractorTests.template.js',
        innerTest: 'app.scenario.js content'
      }),
      jasmine.objectContaining({
        docType: 'e2e-test',
        id: 'protractorTest-x.y.z-' + processor.deployments[0].name,
        example: exampleMap.get('x.y.z'),
        deployment: processor.deployments[0],
        template: 'protractorTests.template.js',
        innerTest: 'app.scenario.js content',
        'ng-app-included': true
      }),
      jasmine.objectContaining({
        docType: 'e2e-test',
        id: 'protractorTest-x.y.z-' + processor.deployments[1].name,
        example: exampleMap.get('x.y.z'),
        deployment: processor.deployments[1],
        template: 'protractorTests.template.js',
        innerTest: 'app.scenario.js content',
        'ng-app-included': true
      })
    ]);
  });
});
