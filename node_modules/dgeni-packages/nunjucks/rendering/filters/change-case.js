var changeCase = require('change-case');

var _ = require('lodash');

/**
 * @dgRenderFilter lowerCase
 * @description Convert the value to lowercase.
 */
/**
 * @dgRenderFilter pascalCase
 * @description Convert the value to PascalCase.
 */
/**
 * @dgRenderFilter upperCase
 * @description Convert the value to UPPERCASE.
 */
/**
 * @dgRenderFilter swapCase
 * @description Swap the case of the letters in value.
 */
/**
 * @dgRenderFilter pathCase
 * @description Convert the value to/a/path.
 */
/**
 * @dgRenderFilter snakeCase
 * @description Convert the value to snake-case.
 */
/**
 * @dgRenderFilter constantCase
 * @description Convert the value to CONSTANT_CASE.
 */
/**
 * @dgRenderFilter paramCase
 * @description Convert the value to a param-case (sometimes called dash-case).
 */
/**
 * @dgRenderFilter dashCase
 * @description Convert the value to a dash-case (sometimes called param-case).
 */
/**
 * @dgRenderFilter dotCase
 * @description Convert the value to dot.case.
 */
/**
 * @dgRenderFilter camelCase
 * @description Convert the value to camelCase.
 */
/**
 * @dgRenderFilter titleCase
 * @description Convert the value to Title Case.
 */
/**
 * @dgRenderFilter sentenceCase
 * @description Convert the value to Sentence case.
 */


var changers = [
  'lower',
  'pascal',
  'upper',
  'swap',
  'path',
  'snake',
  'constant',
  'param',
  'dot',
  'camel',
  'title',
  'sentence'
];

module.exports = _.map(changers, function(changer) {
  return {
    name: changer + 'Case',
    process: changeCase[changer]
  };
});

// Aliases
module.exports.push({
  name: 'dashCase',
  process: changeCase.paramCase
});