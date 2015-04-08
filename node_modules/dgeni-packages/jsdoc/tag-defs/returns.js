module.exports = function(extractTypeTransform, wholeTagTransform) {
  return {
    name: 'returns',
    aliases: ['return'],
    transforms: [ extractTypeTransform, wholeTagTransform ]
  };
};