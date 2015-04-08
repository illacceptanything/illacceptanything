module.exports = function() {
  return {
    name: 'element',
    defaultFn: function(doc) {
      if ( doc.docType === 'directive' || doc.docType === 'input') {
        return'ANY';
      }
    }
  };
};