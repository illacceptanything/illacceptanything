module.exports = function() {
  return {
    name: 'restrict',
    defaultFn: function(doc) {
      if ( doc.docType === 'directive' || doc.docType === 'input' ) {
        return { element: false, attribute: true, cssClass: false, comment: false };
      }
    },
    transforms: function(doc, tag, value) {
      value = value || '';
      return {
        element: value.indexOf('E') !== -1,
        attribute: value.indexOf('A') !== -1,
        cssClass: value.indexOf('C') !== -1,
        comment: value.indexOf('M') !== -1
      };
    }
  };
};