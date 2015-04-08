var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("parse-tags processor", function() {
  var processor;
  var tagDefinitions = [
    { name: 'id' },
    { name: 'description' },
    { name: 'param' },
    { name: 'other-tag', ignore: true }
  ];

  beforeEach(function() {

    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();

    processor = injector.get('parseTagsProcessor');
    processor.tagDefinitions = tagDefinitions;
  });

  it("should be run in the correct place", function() {
    expect(processor.$runAfter).toEqual([ 'parsing-tags' ]);
    expect(processor.$runBefore).toEqual([ 'tags-parsed' ]);
  });


  it("should only return tags that are not ignored", function() {
    var content = 'Some initial content\n@id some.id\n' +
                  '@description Some description\n@other-tag Some other tag\n' +
                  '@param some param\n@param some other param';
    var doc = {content: content, startingLine: 10};
    processor.$process([doc]);

    expect(doc.tags.tags[0]).toEqual(
      jasmine.objectContaining({ tagName: 'id', description: 'some.id', startingLine: 11 })
    );
      // Not that the description tag contains what appears to be another tag but it was ignored so
      // is consumed into the description tag!
    expect(doc.tags.tags[1]).toEqual(
      jasmine.objectContaining({ tagName: 'description', description: 'Some description\n@other-tag Some other tag', startingLine: 12})
    );
    expect(doc.tags.tags[2]).toEqual(
      jasmine.objectContaining({ tagName: 'param', description: 'some param', startingLine: 14 })
    );
    expect(doc.tags.tags[3]).toEqual(
      jasmine.objectContaining({ tagName: 'param', description: 'some other param', startingLine: 15 })
    );
  });

    it("should cope with tags that have no 'description'", function() {
      var content = '@id\n@description some description';
      var doc = { content: content, startingLine: 123 };
      processor.$process([doc]);
      expect(doc.tags.tags[0]).toEqual(jasmine.objectContaining({ tagName: 'id', description: '' }));
      expect(doc.tags.tags[1]).toEqual(jasmine.objectContaining({ tagName: 'description', description: 'some description' }));
    });

    it("should cope with empty content or no known tags", function() {
      expect(function() {
        processor.$process([{ content: '', startingLine: 123 }]);
      }).not.toThrow();

      expect(function() {
        processor.$process([{ content: '@unknownTag some text', startingLine: 123 }]);
      }).not.toThrow();
    });


    it("should ignore @tags inside back-ticked code blocks", function() {
      processor.tagDefinitions = [{ name: 'a' }, { name: 'b' }];
      var content =
      '@a some text\n\n' +
        '```\n' +
        '  some code\n' +
        '  @b not a tag\n' +
        '```\n\n' +
        'more text\n' +
        '@b is a tag';
      var doc = { content: content };
      processor.$process([doc]);
      expect(doc.tags.getTag('a').description).toEqual('some text\n\n' +
        '```\n' +
        '  some code\n' +
        '  @b not a tag\n' +
        '```\n\n' +
        'more text'
      );
      expect(doc.tags.getTags('b').length).toEqual(1);
      expect(doc.tags.getTag('b').description).toEqual('is a tag');
    });


    it("should cope with single line back-ticked code blocks", function() {
      processor.tagDefinitions = [{ name: 'a' }, { name: 'b' }];
      var content =
      '@a some text\n\n' +
        '```some single line of code @b not a tag```\n\n' +
        'some text outside a code block\n' +
        '```\n' +
        '  some code\n' +
        '  @b not a tag\n' +
        '```\n';

      var doc = { content: content };
      processor.$process([doc]);

      expect(doc.tags.getTag('a').description).toEqual('some text\n\n' +
        '```some single line of code @b not a tag```\n\n' +
        'some text outside a code block\n' +
        '```\n' +
        '  some code\n' +
        '  @b not a tag\n' +
        '```\n'
      );

      expect(doc.tags.getTags('b').length).toEqual(0);
    });


    it("should ignore doc if it has no content", function() {
      expect(function() {
        processor.$process([{}]);
      }).not.toThrow();
    });
});