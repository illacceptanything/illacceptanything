var StringMap = require('stringmap');

/**
 * @dgService aliasMap
 * @description
 * A map of id aliases to docs
 */
module.exports = function aliasMap() {
  var map = new StringMap();

  return {
    /**
     * Add a new document to the map associating it with each of its potential aliases
     * @param {Object} doc The document to add to the map
     */
    addDoc: function(doc) {

      if ( !doc.aliases ) return;

      // We store references to this doc under all its id aliases in the map
      // This map will be used to match references to docs
      doc.aliases.forEach(function(alias) {

        // Try to get a list of docs that match this alias
        var matchedDocs = map.get(alias) || [];
        matchedDocs.push(doc);
        map.set(alias, matchedDocs);

      });

    },

    /**
     * Remove a document from the map, including entries for each of its aliases
     * @param  {Object} doc The document to remove from the map
     */
    removeDoc: function(doc) {

      doc.aliases.forEach(function(alias) {

        var matchedDocs = map.get(alias);
        if ( matchedDocs ) {
          // We have an array of docs so we need to remove the culprit
          var index = matchedDocs.indexOf(doc);
          if ( index !== -1 ) {
            matchedDocs.splice(index, 1);
          }
        }

      });
    },

    /**
     * Get the documents associated with the given alias
     * @param  {string} alias The alias to search for
     * @return {Array}  An array containing all matched docs
     */
    getDocs: function(alias) {
      return map.get(alias) || [];
    }
  };

};