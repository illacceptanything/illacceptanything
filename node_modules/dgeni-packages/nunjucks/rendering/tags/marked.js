/**
 * @dgRenderTag marked
 * @description Convert a block of template text from markdown to HTML
 */
module.exports = function markedNunjucksTag(trimIndentation, renderMarkdown) {
  return {
    tags: ['marked'],

    parse: function(parser, nodes) {
      parser.advanceAfterBlockEnd();

      var content = parser.parseUntilBlocks("endmarked");
      var tag = new nodes.CallExtension(this, 'process', null, [content]);
      parser.advanceAfterBlockEnd();

      return tag;
    },

    process: function(context, content) {
      var contentString = content();
      var indent = trimIndentation.calcIndent(contentString);
      var trimmedString = trimIndentation.trimIndent(contentString, indent);
      var markedString = renderMarkdown(trimmedString);
      var reindentedString = trimIndentation.reindent(markedString, indent);
      return reindentedString;
    }
  };
};