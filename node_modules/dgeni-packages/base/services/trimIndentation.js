var _ = require('lodash');

var isEmpty = RegExp.prototype.test.bind(/^\s*$/);

function calcIndent(text) {
  var MAX_INDENT = 9999;
  var lines = text.split('\n');
  var minIndent = MAX_INDENT;
  var emptyLinesRemoved = false;

  // ignore leading empty lines
  while(isEmpty(lines[0])) {
    lines.shift();
    emptyLinesRemoved = true;
  }

  if(lines.length) {

    // ignore first line if it has no indentation and there is more than one line
    // this is because sometimes our text starts in the middle of a line of other
    // text that is indented and so doesn't appear to have an indent when it really does.
    var ignoreLine = (lines[0][0] != ' '  && lines.length > 1);
    if ( ignoreLine && !emptyLinesRemoved ) {
      lines.shift();
    }

    lines.forEach(function(line){
      if ( !isEmpty(line) ) {
        var indent = line.match(/^\s*/)[0].length;
        minIndent = Math.min(minIndent, indent);
      }
    });

  }

  return minIndent;
}

function reindent(text, indent) {
  var lines = text.split('\n');
  var indentedLines = [];
  var indentStr = new Array(indent + 1).join(' ');
  _.forEach(lines, function(line) {
    indentedLines.push(indentStr + line);
  });
  return indentedLines.join('\n');
}

function trimIndent(text, indent) {
  var lines = text.split('\n');
  var indentRegExp = new RegExp('^\\s{0,' + indent + '}');

  // remove the indentation
  for ( var i = 0; i < lines.length; i++) {
    lines[i] = lines[i].replace(indentRegExp, '');
  }

  // remove leading lines
  while (isEmpty(lines[0])) { lines.shift(); }

  // remove trailing
  while (isEmpty(lines[lines.length - 1])) { lines.pop(); }

  return lines.join('\n');
}

// The primary export is a function that does the intentation trimming
module.exports = function trimIndentation() {
  var trimIndentationImpl = function(text) {
      return trimIndent(text, calcIndent(text));
  };
  trimIndentationImpl.calcIndent = calcIndent;
  trimIndentationImpl.trimIndent = trimIndent;
  trimIndentationImpl.reindent = reindent;
  return trimIndentationImpl;
};