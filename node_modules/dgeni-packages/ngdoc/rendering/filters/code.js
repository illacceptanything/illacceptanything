module.exports = function(encodeCodeBlock) {
  return {
    name: 'code',
    process: function(str, lang) {
      return encodeCodeBlock(str, true, lang);
    }
  };
};