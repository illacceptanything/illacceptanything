module.exports = function(extractTypeTransform, wholeTagTransform) {
  return {
    name: 'type',
    transforms: [ extractTypeTransform, wholeTagTransform ]
  };
};