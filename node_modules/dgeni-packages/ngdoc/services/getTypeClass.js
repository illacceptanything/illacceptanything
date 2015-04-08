/**
 * @dgService getTypeClass
 * @description
 * Get a CSS class string for the given type string
 */
module.exports = function getTypeClass() {
  return function(typeStr) {
    var typeClass = typeStr.toLowerCase().match(/^[-\w]+/) || [];
    typeClass = typeClass[0] ? typeClass[0] : 'object';
    return 'label type-hint type-hint-' + typeClass;
  };
};