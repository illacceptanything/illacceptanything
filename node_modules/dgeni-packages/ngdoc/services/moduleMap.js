var StringMap = require('stringmap');

/**
 * @dgService moduleMap
 * @description
 * A collection of modules keyed on the module name
 */
module.exports = function moduleMap() {
  return new StringMap();
};