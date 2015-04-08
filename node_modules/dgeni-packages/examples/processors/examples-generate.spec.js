var _ = require('lodash');
var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("examples-generate processor", function() {
  var templateFolder, deployments, docs, exampleMap;

  beforeEach(function() {

    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();

    docs = [{ file: 'a.b.js' }];

    files = {};

    files['index.html'] = { type: 'html', name: 'index.html', fileContents: 'index.html content' };
    files['app.js'] = { type: 'js', name: 'app.js', fileContents: 'app.js content' };
    files['app.css'] = { type: 'css', name: 'app.css', fileContents: 'app.css content' };
    files['app.spec.js'] = { type: 'spec', name: 'app.spec.js', fileContents: 'app.spec.js content' };

    exampleMap = injector.get('exampleMap');
    exampleMap.set('a.b.c', {
      id: 'a.b.c',
      doc: docs[0],
      outputFolder: 'examples',
      deps: 'dep1.js;dep2.js;http://example.com/dep3.js;https://example.com/dep4.js;dep5.css;http://example.com/dep6.css;https://example.com/dep7.css',
      files: files,
      deployments: {}
    });

    processor = injector.get('generateExamplesProcessor');
    processor.templateFolder = 'examples';
    processor.deployments = [
      {
        name: 'default',
        examples: { commonFiles: [], dependencyPath: '.' },
      },
      {
        name: 'other',
        examples: { commonFiles: { scripts: [ 'someFile.js', 'someOtherFile.js' ], stylesheets: ['someStyle.css', 'otherStyle.css']}, dependencyPath: '..' }
      }
    ];

    processor.$process(docs);

  });
  it("should add an exampleDoc for each example deployment", function() {
    var exampleDocs = _.filter(docs, { docType: 'example' });
    expect(exampleDocs.length).toBe(2);

    expect(exampleDocs[0]).toEqual(
      jasmine.objectContaining({ docType: 'example', id:'a.b.c', template: 'index.template.html'})
    );
    expect(exampleDocs[1]).toEqual(
      jasmine.objectContaining({ docType: 'example', id:'a.b.c-other', template: 'index.template.html'})
    );

    expect(exampleDocs[0].fileContents).toEqual('index.html content');
    expect(exampleDocs[1].fileContents).toEqual('index.html content');
  });

  it("should add a fileDoc for each of the example's files", function() {
    expect(_.find(docs, { id: 'a.b.c/app.js' })).toEqual(
      jasmine.objectContaining({ docType: 'example-file', template: 'template.js' })
    );
    expect(_.find(docs, { id: 'a.b.c/app.css' })).toEqual(
      jasmine.objectContaining({ docType: 'example-file', template: 'template.css' })
    );
    expect(_.find(docs, { id: 'a.b.c/app.spec.js' })).toEqual(
      jasmine.objectContaining({ docType: 'example-file', template: 'template.spec' })
    );
  });

  it("should add the dependencies to the exampleDoc scripts", function() {
    expect(_.find(docs, { id: 'a.b.c' }).scripts).toEqual([
      { path : 'dep1.js' },
      { path : 'dep2.js' },
      { path : 'http://example.com/dep3.js' },
      { path : 'https://example.com/dep4.js' },
      jasmine.objectContaining({ docType : 'example-file', id : 'a.b.c/app.js' })
    ]);

    expect(_.find(docs, { id: 'a.b.c-other' }).scripts).toEqual([
      { path: 'someFile.js' },
      { path: 'someOtherFile.js' },
      { path : '../dep1.js' },
      { path : '../dep2.js' },
      { path : 'http://example.com/dep3.js' },
      { path : 'https://example.com/dep4.js' },
      jasmine.objectContaining({ docType : 'example-file', id : 'a.b.c/app.js' })
    ]);
  });

  it("should add the dependencies to the exampleDoc stylesheets", function() {
    expect(_.find(docs, { id: 'a.b.c' }).stylesheets).toEqual([
      { path : 'dep5.css' },
      { path : 'http://example.com/dep6.css' },
      { path : 'https://example.com/dep7.css' },
      jasmine.objectContaining({ docType : 'example-file', id : 'a.b.c/app.css' })
    ]);

    expect(_.find(docs, { id: 'a.b.c-other' }).stylesheets).toEqual([
      { path: 'someStyle.css' },
      { path: 'otherStyle.css' },
      { path : '../dep5.css' },
      { path : 'http://example.com/dep6.css' },
      { path : 'https://example.com/dep7.css' },
      jasmine.objectContaining({ docType : 'example-file', id : 'a.b.c/app.css' })
    ]);
  });

  it("should add a runnableExampleDoc for each example", function() {
    var runnableExampleDocs = _.filter(docs, { docType: 'runnableExample' });
    expect(runnableExampleDocs.length).toEqual(1);
  });

  it("should add a manifest doc for each example", function() {
    var manifestDoc = _.filter(docs, { docType: 'example-file', template: 'manifest.template.json' })[0];
    expect(manifestDoc.id).toEqual('a.b.c/manifest.json');
    expect(manifestDoc.docType).toEqual('example-file');
    expect(manifestDoc.example).toEqual(exampleMap.get('a.b.c'));
    expect(manifestDoc.files).toEqual(['app.js', 'app.css', 'app.spec.js']);
  });
});
