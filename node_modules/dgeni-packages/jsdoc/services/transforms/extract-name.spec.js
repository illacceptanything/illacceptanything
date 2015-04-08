var transformFactory = require('./extract-name');

describe("extract-name transform", function() {
  var doc, tag, value, transform;

  beforeEach(function() {
    doc = {};
    tag = {};
    transform = transformFactory();
  });

  it("should extract the name from the description", function() {

    value = '   paramName - Some description  \n Some more description';
    value = transform(doc, tag, value);

    expect(tag.name).toEqual('paramName');
    expect(value).toEqual('Some description  \n Some more description');
  });

  it("should extract an optional name", function() {
    value = '[someName]';
    value = transform(doc, tag, value);
    expect(tag.name).toEqual('someName');
    expect(tag.optional).toEqual(true);
    expect(value).toEqual('');
  });

  it("should extract a name and its default value", function() {
    value = '[someName=someDefault]';
    value = transform(doc, tag, value);
    expect(tag.name).toEqual('someName');
    expect(tag.optional).toEqual(true);
    expect(tag.defaultValue).toEqual('someDefault');
    expect(value).toEqual('');
  });

  it("should extract a param name alias", function() {
    value = 'paramName|aliasName some description';
    value = transform(doc, tag, value);
    expect(tag.name).toEqual('paramName');
    expect(tag.alias).toEqual('aliasName');
    expect(value).toEqual('some description');
  });

});