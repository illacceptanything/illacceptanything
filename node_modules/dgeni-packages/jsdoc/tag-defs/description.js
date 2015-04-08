module.exports = function() {
  return {
    name: 'description',
    transforms: function(doc, tag, value) {
      if ( doc.tags.description ) {
        value = doc.tags.description + '\n' + value;
      }
      return value;
    },
    defaultFn: function(doc) {
      return doc.tags.description;
    }
  };
};