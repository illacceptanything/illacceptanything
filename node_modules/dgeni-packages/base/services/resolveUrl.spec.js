var mockPackage = require('../mocks/mockPackage');
var Dgeni = require('dgeni');

describe("resolveUrl", function() {
  var resolveUrl;

  beforeEach(function() {
    var dgeni = new Dgeni([mockPackage()]);
    var injector = dgeni.configureInjector();
    resolveUrl = injector.get('resolveUrl');
  });

  it("should calculate absolute paths", function() {
    expect(resolveUrl('currentPath/currentFile.html', '/absolutePath/absoluteFile.html', ''))
      .toEqual('/absolutePath/absoluteFile.html');
    expect(resolveUrl('currentPath/currentFile.html', '/absolutePath/absoluteFile.html', '/'))
      .toEqual('/absolutePath/absoluteFile.html');
    expect(resolveUrl('currentPath/currentFile.html', '/absolutePath/absoluteFile.html', '/base'))
      .toEqual('/absolutePath/absoluteFile.html');
    expect(resolveUrl('currentPath/currentFile.html', '/absolutePath/absoluteFile.html', '/base/'))
      .toEqual('/absolutePath/absoluteFile.html');
  });

  it('should use the base path when the path is relative and there is a base path', function() {
    expect(resolveUrl('currentPath/currentFile.html', 'relativePath/relativeFile.html', '/'))
      .toEqual('relativePath/relativeFile.html');
    expect(resolveUrl('currentPath/currentFile.html', 'relativePath/relativeFile.html', '/base'))
      .toEqual('base/relativePath/relativeFile.html');
    expect(resolveUrl('currentPath/currentFile.html', 'relativePath/relativeFile.html', '/base/'))
      .toEqual('base/relativePath/relativeFile.html');
  });

  it('should use the current directory when there is no base path', function() {
    expect(resolveUrl('currentPath/currentFile.html', 'relativePath/relativeFile.html', ''))
      .toEqual('currentPath/relativePath/relativeFile.html');
    expect(resolveUrl('onePath/currentPath/currentFile.html', 'relativePath/relativeFile.html', ''))
      .toEqual('onePath/currentPath/relativePath/relativeFile.html');
    expect(resolveUrl('currentFile.html', 'relativePath/relativeFile.html', ''))
      .toEqual('relativePath/relativeFile.html');
    expect(resolveUrl('', 'relativePath/relativeFile.html', ''))
      .toEqual('relativePath/relativeFile.html');
    expect(resolveUrl(null, 'relativePath/relativeFile.html', ''))
      .toEqual('relativePath/relativeFile.html');
  });

  it('should remove any query params', function() {
    expect(resolveUrl('currentPath/currentFile.html', '/absolutePath/absoluteFile.html?foo=bar', ''))
      .toEqual('/absolutePath/absoluteFile.html');
    expect(resolveUrl('currentPath/currentFile.html', '/absolutePath/absoluteFile.html#foo', ''))
      .toEqual('/absolutePath/absoluteFile.html#foo');
    expect(resolveUrl('currentPath/currentFile.html', '/absolutePath/absoluteFile.html?bar=baz#foo', ''))
      .toEqual('/absolutePath/absoluteFile.html#foo');
    expect(resolveUrl('onePath/currentPath/currentFile.html?foo=bar', 'relativePath/relativeFile.html', ''))
      .toEqual('onePath/currentPath/relativePath/relativeFile.html');
    expect(resolveUrl('onePath/currentPath/currentFile.html#foo', 'relativePath/relativeFile.html', ''))
      .toEqual('onePath/currentPath/relativePath/relativeFile.html');
    expect(resolveUrl('onePath/currentPath/currentFile.html', 'relativePath/relativeFile.html?foo=bar', ''))
      .toEqual('onePath/currentPath/relativePath/relativeFile.html');
    expect(resolveUrl('onePath/currentPath/currentFile.html', 'relativePath/relativeFile.html#foo', ''))
      .toEqual('onePath/currentPath/relativePath/relativeFile.html#foo');
    expect(resolveUrl('onePath/currentPath/currentFile.html', 'relativePath/relativeFile.html?bar=baz#foo', ''))
      .toEqual('onePath/currentPath/relativePath/relativeFile.html#foo');
  });

  it("should not remove filename if there is only a hash", function() {
    expect(resolveUrl('xx/yy', '#xyz', ''))
      .toEqual('xx/yy#xyz');
    expect(resolveUrl('xx', '#xyz', ''))
      .toEqual('xx#xyz');
  });

  it('should remove any /./ in the path', function() {
    expect(resolveUrl('currentFile.html', '/./absolutePath/./absoluteFile.html', ''))
      .toEqual('/absolutePath/absoluteFile.html');
  });

  // it('should remove any dangling / at the end', function() {
  //   expect(resolveUrl('currentFile.html', '/absolutePath/', ''))
  //     .toEqual('absolutePath');
  // });
});
