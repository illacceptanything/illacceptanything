/**
 * @dgRenderFilter json
 * @description Convert the object to a JSON string
 */
module.exports = {
  name: 'json',
  process: function(obj) {
    return JSON.stringify(obj, null, '  ');
  }
};