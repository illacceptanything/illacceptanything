/**
 * @dgRenderFilter marked
 * @description Convert the value, as markdown, into HTML using the marked library
 */
module.exports = function markedNunjucksFilter(renderMarkdown) {
  return {
    name: 'marked',
    process: function(str) {
      var output = str && renderMarkdown(str);
      return output;
    }
  };
};