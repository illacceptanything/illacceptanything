var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

var Tag = require('../lib/Tag');
var TagCollection = require('../lib/TagCollection');


function createDoc(tags) {
  return {
    fileInfo: {
      filePath: 'some/file.js'
    },
    tags: new TagCollection(tags)
  };
}

function createProcessor(tagDefs) {
  return factory(mockLog, createTagDefContainer(tagDefs), createDocMessageFactory());
}

describe("extractTagsProcessor", function() {

  var parseTagsProcessor, processor, mockLog;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    parseTagsProcessor = injector.get('parseTagsProcessor');
    processor = injector.get('extractTagsProcessor');
    mockLog = injector.get('log');
  });

  it("should log a warning if the doc contains bad tags", function() {

      var doc = createDoc([]);
      doc.tags.badTags = [ {
        name: 'bad1',
        description: 'bad tag 1',
        typeExpression: 'string',
        errors: [
          'first bad thing',
          'second bad thing'
        ]
      }];

      processor.$process([doc]);
      expect(mockLog.warn).toHaveBeenCalledWith('Invalid tags found - doc\n' +
        'Line: undefined: @undefined {string} bad1 bad tag 1...\n' +
        '    * first bad thing\n' +
        '    * second bad thing\n\n');
  });

  describe('default tag-def', function() {
    it("should the extract the description property to a property with the name of the tagDef", function() {
      var tagDef = { name: 'a' };
      parseTagsProcessor.tagDefinitions = [tagDef];

      var tag = new Tag(tagDef, 'a', 'some content', 123);
      var doc = createDoc([tag]);

      processor.$process([doc]);
      expect(doc.a).toEqual('some content');
    });
  });

  describe("tag-defs with docProperty", function() {
    it("should assign the extracted value to the docProperty", function() {
      var tagDef = { name: 'a', docProperty: 'b' };
      parseTagsProcessor.tagDefinitions = [tagDef];

      var tag = new Tag(tagDef, 'a', 'some content', 123);
      var doc = createDoc([tag]);

      processor.$process([doc]);
      expect(doc.a).toBeUndefined();
      expect(doc.b).toEqual('some content');

    });
  });

  describe("tag-defs with multi", function() {
    it("should assign the extracted value(s) to an array on the doc", function() {
      var tagDef = { name: 'a', multi: true };
      parseTagsProcessor.tagDefinitions = [tagDef];

      var tag1 = new Tag(tagDef, 'a', 'some content', 123);
      var tag2 = new Tag(tagDef, 'a', 'some other content', 256);
      var docA = createDoc([tag1]);
      var docB = createDoc([tag1, tag2]);

      processor.$process([docA]);
      expect(docA.a).toEqual(['some content']);

      processor.$process([docB]);
      expect(docB.a).toEqual(['some content', 'some other content']);
    });
  });

  describe("tag-defs with required", function() {
    it("should throw an error if the tag is missing", function() {
      var tagDef = { name: 'a', required: true };
      parseTagsProcessor.tagDefinitions = [tagDef];

      var doc = createDoc([]);
      expect(function() {
        processor.$process([doc]);
      }).toThrow();
    });
  });

  describe("tag-defs with tagProperty", function() {
    it("should assign the specified tag property to the document", function() {

      var tagDef = { name: 'a', tagProperty: 'b' };
      parseTagsProcessor.tagDefinitions = [tagDef];

      var tag = new Tag(tagDef, 'a', 'some content', 123);
      tag.b = 'special value';
      var doc = createDoc([tag]);

      processor.$process([doc]);
      expect(doc.a).toEqual('special value');

    });
  });

  describe("tag-defs with defaultFn", function() {

    it("should run the defaultFn if the tag is missing", function() {
      var defaultFn = jasmine.createSpy('defaultFn').and.returnValue('default value');
      var tagDef = { name: 'a', defaultFn: defaultFn };
      parseTagsProcessor.tagDefinitions = [tagDef];

      var doc = createDoc([]);

      processor.$process([doc]);
      expect(doc.a).toEqual('default value');
      expect(defaultFn).toHaveBeenCalled();
    });

    describe("and mult", function() {

      it("should run the defaultFn if the tag is missing", function() {
        var defaultFn = jasmine.createSpy('defaultFn').and.returnValue('default value');
        var tagDef = { name: 'a', defaultFn: defaultFn, multi: true };

        parseTagsProcessor.tagDefinitions = [tagDef];
        var doc = createDoc([]);

        processor.$process([doc]);
        expect(doc.a).toEqual(['default value']);
        expect(defaultFn).toHaveBeenCalled();
      });

    });

  });


  describe("transforms", function() {

    describe("(single)", function() {
      it("should apply the transform to the extracted value", function() {
        function addA(doc, tag, value) { return value + '*A*'; }
        var tagDef = { name: 'a', transforms: addA };

        parseTagsProcessor.tagDefinitions = [tagDef];

        var tag = new Tag(tagDef, 'a', 'some content', 123);
        var doc = createDoc([tag]);

        processor.$process([doc]);
        expect(doc.a).toEqual('some content*A*');

      });

      it("should allow changes to tag and doc", function() {
        function transform(doc, tag, value) { doc.x = 'x'; tag.y = 'y'; return value; }
        var tagDef = { name: 'a', transforms: transform };

        parseTagsProcessor.tagDefinitions = [tagDef];

        var tag = new Tag(tagDef, 'a', 'some content', 123);
        var doc = createDoc([tag]);

        processor.$process([doc]);
        expect(doc.a).toEqual('some content');
        expect(doc.x).toEqual('x');
        expect(tag.y).toEqual('y');
      });
    });


    describe("(multiple)", function() {
      it("should apply the transforms to the extracted value", function() {
        function addA(doc, tag, value) { return value + '*A*'; }
        function addB(doc, tag, value) { return value + '*B*'; }
        var tagDef = { name: 'a', transforms: [ addA, addB ] };

        parseTagsProcessor.tagDefinitions = [tagDef];

        var tag = new Tag(tagDef, 'a', 'some content', 123);
        var doc = createDoc([tag]);

        processor.$process([doc]);
        expect(doc.a).toEqual('some content*A**B*');

      });

      it("should allow changes to tag and doc", function() {
        function transform1(doc, tag, value) { doc.x = 'x'; return value; }
        function transform2(doc, tag, value) { tag.y = 'y'; return value; }
        var tagDef = { name: 'a', transforms: [transform1, transform2] };

        parseTagsProcessor.tagDefinitions = [tagDef];

        var tag = new Tag(tagDef, 'a', 'some content', 123);
        var doc = createDoc([tag]);

        processor.$process([doc]);
        expect(doc.a).toEqual('some content');
        expect(doc.x).toEqual('x');
        expect(tag.y).toEqual('y');
      });
    });
  });

  describe("default transforms", function() {

    it("should apply the default transformations to all tags", function() {
      var tagDef1 = { name: 'a' };
      var tagDef2 = { name: 'b' };
      function addA(doc, tag, value) { return value + '*A*'; }

      parseTagsProcessor.tagDefinitions = [tagDef1, tagDef2];
      processor.defaultTagTransforms = [addA];

      var tag1 = new Tag(tagDef1, 'a', 'some content', 123);
      var tag2 = new Tag(tagDef2, 'b', 'some other content', 123);
      var doc = createDoc([tag1, tag2]);

      processor.$process([doc]);
      expect(doc.a).toEqual('some content*A*');
      expect(doc.b).toEqual('some other content*A*');

    });


    it("should apply the default transformations after tag specific transforms", function() {
      function addA(doc, tag, value) { return value + '*A*'; }
      function addB(doc, tag, value) { return value + '*B*'; }
      var tagDef1 = { name: 'a', transforms: addA };

      parseTagsProcessor.tagDefinitions = [tagDef1];
      processor.defaultTagTransforms = [addB];

      var tag = new Tag(tagDef1, 'a', 'some content', 123);
      var doc = createDoc([tag]);

      processor.$process([doc]);
      expect(doc.a).toEqual('some content*A**B*');

    });
  });

});