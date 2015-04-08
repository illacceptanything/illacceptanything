var htmlparser = require("htmlparser2");

/**
 * @dgService extractLinks
 * @description
 * Extracts the links and references from a given html
 * @param {String} html The html
 */
module.exports = function extractLinks() {
  return function(html) {
    var result = {hrefs: [], names: []};
    var parser = new htmlparser.Parser({
      onopentag: function(name, attribs) {

        // Parse anchor elements, extracting href and name
        if (name === 'a') {
          if (attribs.href) {
            result.hrefs.push(attribs.href);
          }
          if (attribs.name) {
            result.names.push(attribs.name);
          }
        }

        // Extract id from other elements
        if(attribs.id) {
          result.names.push(attribs.id);
        }
      }
    }, {
      decodeEntities: true
    });
    parser.write(html);
    parser.end();
    return result;
  };
};
