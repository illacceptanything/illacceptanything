var _ = require('lodash');

/**
 * Trim excess whitespace from the value
 */
module.exports = function trimWhitespaceTransform() {
  return function(doc, tag, value) {
    if ( _.isString(value) ) {
      return value.trim();
    }
    return value;
  };
};