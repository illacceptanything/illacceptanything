var marked = require('marked');

/**
 * @dgService renderMarkdown
 * @description
 * Render the markdown in the given string as HTML.
 */
module.exports = function renderMarkdown(trimIndentation) {

  var renderer = new marked.Renderer();

  // remove the leading whitespace from the code block before it gets to the
  // markdown code render function
  renderer.code = function(code, string, language) {

    var trimmedCode = trimIndentation(code);
    var renderedCode = marked.Renderer.prototype.code.call(this, trimmedCode, string, language);

    // Bug in marked - forgets to add a final newline sometimes
    if ( !/\n$/.test(renderedCode) ) {
      renderedCode += '\n';
    }

    return renderedCode;
  };

  return function(content) {
    return marked(content, { renderer: renderer });
  };
};