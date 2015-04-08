/**
 * Use the whole tag as the value rather than using a tag property, such as `description`
 * @param  {Tag} tag The tag to process
 */
module.exports = function wholeTagTransform() {
  return function(doc, tag, value) {
    return tag;
  };
};