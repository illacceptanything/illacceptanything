module.exports = function unknownTagTransform() {
  return function(doc, tag, value) {
    if ( !tag.tagDef ) {
      tag.errors = tag.errors || [];
      tag.errors.push('Unknown tag: ' + tag.tagName);
    }
  };
};