function Tag(tagDef, tagName, description, lineNumber) {
  this.tagDef = tagDef;
  this.tagName = tagName;
  this.description = description;
  this.startingLine = lineNumber;
}

module.exports = Tag;