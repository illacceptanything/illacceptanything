module.exports = function(extractTypeTransform, extractNameTransform, wholeTagTransform) {
  return {
    name: 'param',
    multi: true,
    docProperty: 'params',
    transforms: [ extractTypeTransform, extractNameTransform, wholeTagTransform ]
  };
};