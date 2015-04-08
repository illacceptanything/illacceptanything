/**
 * JSON support module v2.2.1
 * http://rightjs.org/plugins/json
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var JSON = function(RightJS, window) {
 
 /**
 * Initialization script
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
RightJS.JSON = window.JSON || {};

RightJS.JSON.version = '2.2.1';



/**
 * The generic JSON interface
 *
 * Credits:
 *   Based on the original JSON escaping implementation
 *     http://www.json.org/json2.js
 *
 * @copyright (C) 2009-2011 Nikolay V. Nemshilov
 */
var

JSON = RightJS.JSON,

// see the original JSON decoder implementation for descriptions http://www.json.org/json2.js
cx = /[\u0000\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g,
specials = {'\b': '\\b', '\t': '\\t', '\n': '\\n', '\f': '\\f', '\r': '\\r', '"' : '\\"', '\\': '\\\\'},
quotables = /[\\\"\x00-\x1f\x7f-\x9f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g;


// quotes the string
function quote(string) {
  return string.replace(quotables, function(chr) {
    return specials[chr] || '\\u' + ('0000' + chr.charCodeAt(0).toString(16)).slice(-4);
  });
}

// adds the leading zero symbol
function zerofy(num) {
  return (num < 10 ? '0' : '')+num;
}

/**
 * Checking if there is the JSON to String method
 */
if (!('stringify' in JSON)) {
  JSON.stringify = function(value) {
    if (value === null) {
      return 'null';
    } else if (value.toJSON) {
      return value.toJSON();
    } else {

      switch(typeof(value)) {
        case 'boolean': return String(value);
        case 'number':  return String(value+0);
        case 'string':  return '"'+ quote(value) + '"';
        case 'object':

          if (value instanceof Array) {
            return '['+value.map(JSON.stringify).join(',')+']';

          } else if (value instanceof Date) {
            return '"' + value.getUTCFullYear() + '-' +
              zerofy(value.getUTCMonth() + 1)   + '-' +
              zerofy(value.getUTCDate())        + 'T' +
              zerofy(value.getUTCHours())       + ':' +
              zerofy(value.getUTCMinutes())     + ':' +
              zerofy(value.getUTCSeconds())     + '.' +
              zerofy(value.getMilliseconds())   + 'Z' +
            '"';

          } else {
            var result = [];
            for (var key in value) {
              result.push(JSON.encode(key)+":"+JSON.encode(value[key]));
            }
            return '{'+result.join(',')+'}';
          }
      }
    }
  };
}

/**
 * Checking if there is the string to JSON method
 */
if (!('parse' in JSON)) {
  JSON.parse = function(string) {
    if (isString(string) && string) {
      // getting back the UTF-8 symbols
      string = string.replace(cx, function (a) {
        return '\\u' + ('0000' + a.charCodeAt(0).toString(16)).slice(-4);
      });

      // checking the JSON string consistency
      if (/^[\],:{}\s]*$/.test(string.replace(/\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})/g, '@')
        .replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g, ']')
        .replace(/(?:^|:|,)(?:\s*\[)+/g, ''))) {
          return new Function('return '+string)();
        }
    }

    throw "JSON parse error: "+string;
  };
}

RightJS.$alias(JSON, {
  encode: 'stringify',
  decode: 'parse'
});


/**
 * Wraps up the Cooke set/get methods so that the values
 * were automatically exported/imported into JSON strings
 * and it allowed transparent objects and arrays saving
 *
 * @copyright (C) 2009-2010 Nikolay V. Nemshilov
 */

if (RightJS.Cookie) {
  var old_set = RightJS.Cookie.prototype.set,
      old_get = RightJS.Cookie.prototype.get;

  RightJS.Cookie.include({
    set: function(value) {
      return old_set.call(this, JSON.stringify(value));
    },

    get: function() {
      return JSON.parse(old_get.call(this) || 'null');
    }
  });
}

 
 return JSON;
}(RightJS, window);