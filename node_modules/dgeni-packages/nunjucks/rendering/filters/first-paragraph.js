/**
 * @dgRenderFilter firstParagraph
 * @description Extract the first paragraph from the value, breaking on the first double newline
 */
module.exports = {
  name: 'firstParagraph',
  process: function(str) {
    if (!str) return str;

    str = str
      .split("\n\n")[0];
    return str;
  }
};