module.exports = function(extractTypeTransform, extractNameTransform, wholeTagTransform) {
  return {
    name: 'property',
    multi: true,
    docProperty: 'properties',
    transforms: [ extractTypeTransform, extractNameTransform, wholeTagTransform ]
  };
};