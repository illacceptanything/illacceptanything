// Much of this code was inspired by or simply copied from the JSDOC project.
// See https://github.com/jsdoc3/jsdoc/blob/c9b0237c12144cfa48abe5fccd73ba2a1d46553a/lib/jsdoc/tag/type.js

var catharsis = require('catharsis');
var TYPE_EXPRESSION_START = /\{[^@]/;

/**
 * Extract a type expression from the tag text.
 *
 * @private
 * @param {Tag} tag The tag whose type should be extracted
 */
module.exports =  function extractTypeTransform() {
  return function(doc, tag, value) {
    var start, position, count, length, expression;

    start = value.search(TYPE_EXPRESSION_START);
    length = value.length;
    if (start !== -1) {
      // advance to the first character in the type expression
      position = start + 1;
      count = 1;

      while (position < length) {
        switch (value[position]) {
          case '\\':
            // backslash is an escape character, so skip the next character
            position++;
            break;
          case '{':
            count++;
            break;
          case '}':
            count--;
            break;
          default:
            // do nothing
        }

        if (count === 0) {
          break;
        }
        position++;
      }

      tag.typeExpression = value.slice(start+1, position).trim().replace('\\}', '}').replace('\\{', '{');

      try {
        tag.type = catharsis.parse(tag.typeExpression, {jsdoc: true});
        tag.typeList = getTypeStrings(tag.type);
        if ( tag.type.optional ) {
          tag.optional = true;
        }
        tag.description = (value.substring(0, start) + value.substring(position+1)).trim();
      } catch(x) {
        throw new Error('Error parsing the jsdoc type expression "{' + tag.typeExpression + '}"', x);
      }

      return tag.description;
    } else {
      return value;
    }
  };

  /** @private */
  function getTypeStrings(parsedType) {
    var types = [];

    var TYPES = catharsis.Types;
    var util = require('util');

    switch(parsedType.type) {
      case TYPES.AllLiteral:
        types.push('*');
        break;
      case TYPES.FunctionType:
        types.push(catharsis.stringify(parsedType));
        break;
      case TYPES.NameExpression:
        types.push(parsedType.name);
        break;
      case TYPES.NullLiteral:
        types.push('null');
        break;
      case TYPES.RecordType:
        types.push('Object');
        break;
      case TYPES.TypeApplication:
        types.push( catharsis.stringify(parsedType) );
        break;
      case TYPES.TypeUnion:
        parsedType.elements.forEach(function(element) {
          types = types.concat( getTypeStrings(element) );
        });
        break;
      case TYPES.UndefinedLiteral:
        types.push('undefined');
        break;
      case TYPES.UnknownLiteral:
        types.push('?');
        break;
      default:
        // this shouldn't happen
        throw new Error( util.format('unrecognized type %s in parsed type: %j', parsedType.type,
          parsedType) );
    }

    return types;
  }
};