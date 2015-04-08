module.exports = function(createDocMessage) {
  return {
    name: 'memberof',
    defaultFn: function(doc) {
      if ( doc.docType === 'event' || doc.docType === 'property' || doc.docType === 'method' ) {
        throw new Error(createDocMessage('Missing tag "@memberof" for doc of type "'+ doc.docType, doc));
      }
    },
    transforms: function(doc, tag, value) {
      if ( !(doc.docType === 'event' || doc.docType === 'property' || doc.docType === 'method') ) {
        throw new Error(createDocMessage('"@'+ tag.name +'" tag found on non-'+ doc.docType +' document', doc));
      }
      return value;
    }
  };
};