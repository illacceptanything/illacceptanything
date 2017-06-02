/**
 * The old browsers support module for RightJS
 *
 * Released under the terms of the MIT license
 * Visit http://rightjs.org for more details
 *
 * Copyright (C) 2008-2012 Nikolay Nemshilov
 */
(function(RightJS) {
/**
 * Old IE browser hacks
 *
 *   Keep them in one place so they were more compact
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
if (RightJS.Browser.OLD && RightJS.Browser.IE) {
  // loads DOM element extensions for selected elements
  window.$ = RightJS.$ = (function(old_function) {
    return function(id) {
      var element = old_function(id);

      // old IE browses match both, ID and NAME
      if (element && element instanceof RightJS.Element &&
        RightJS.isString(id) && element._.id !== id
      ) {
        element = RightJS.$(document).first('#'+ id);
      }

      return element;
    };
  })(RightJS.$);
}


/**
 * Making the 'submit' and 'change' events bubble under IE browsers
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */

/**
 * Tests if there is the event support
 *
 * @param String event name
 * @retrun Boolean check result
 */
function event_support_for(name, tag) {
  var e = document.createElement(tag);
  e.setAttribute(name, ';');
  return isFunction(e[name]);
}

if (!event_support_for('onsubmit', 'form')) {
  /**
   * Emulates the 'submit' event bubbling for IE browsers
   *
   * @param raw dom-event
   * @return void
   */
  var submit_boobler = function(raw_event) {
    var event = $(raw_event), element = event.target._,
        type = element.type, form = element.form, parent;

    if (form && (parent = $(form).parent()) && (
      (raw_event.keyCode === 13   && (type === 'text'   || type === 'password')) ||
      (raw_event.type === 'click' && (type === 'submit' || type === 'image'))
    )) {
      event.type   = 'submit';
      event.target = $(form);
      parent.fire(event);
    }
  };

  document.attachEvent('onclick',    submit_boobler);
  document.attachEvent('onkeypress', submit_boobler);
}

if (!event_support_for('onchange', 'input')) {

  var get_input_value = function(target) {
    var element = target._,
        type    = element.type;

    return type === 'radio' || type === 'checkbox' ?
      element.checked : target.getValue();
  },

  /**
   * Emulates the 'change' event bubbling
   *
   * @param Event wrapped dom-event
   * @param Input wrapped input element
   * @return void
   */
  change_boobler = function(event, target) {
    var parent  = target.parent(),
        value   = get_input_value(target);

    if (parent && ''+target._prev_value !== ''+value) {
      target._prev_value = value; // saving the value so it didn't fire up again
      event.type = 'change';
      parent.fire(event);
    }
  },

  /**
   * Catches the input field changes
   *
   * @param raw dom-event
   * @return void
   */
  catch_inputs_access = function(raw_event) {
    var event  = $(raw_event),
        target = event.target,
        type   = target._.type,
        tag    = target._.tagName,
        input_is_radio = (type === 'radio' || type === 'checkbox');

    if (
      (event.type === 'click' && (input_is_radio || tag === 'SELECT')) ||
      (event.type === 'keydown' && (
        (event.keyCode == 13 && (tag !== 'TEXTAREA')) ||
        type === 'select-multiple'
      ))
    ) {
      change_boobler(event, target);
    }
  },

  /**
   * Catch inputs blur
   *
   * @param raw dom-event
   * @return void
   */
  catch_input_left = function(raw_event) {
    var event  = $(raw_event),
        target = event.target;

    if (target instanceof Input) {
      change_boobler(event, target);
    }
  };

  document.attachEvent('onclick',    catch_inputs_access);
  document.attachEvent('onkeydown',  catch_inputs_access);
  document.attachEvent('onfocusout', catch_input_left);

  /**
   * storing the input element previous value, so we could figure out
   * if it was changed later on
   */
  document.attachEvent('onbeforeactivate', function(event) {
    var element = $(event).target;

    if (element instanceof Input) {
      element._prev_value = get_input_value(element);
    }
  });
}


/**
 * Konqueror browser fixes
 *
 * Copyright (C) 2009-2011 Nikolay V. Nemshilov
 */

/**
 * manual position calculator, it works for Konqueror and also
 * for old versions of Opera and FF
 */
if (!RightJS.$E('p')._.getBoundingClientRect) {
  RightJS.Element.include({
    position: function() {
      var element  = this._,
          top      = element.offsetTop,
          left     = element.offsetLeft,
          parent   = element.offsetParent;

      while (parent) {
        top  += parent.offsetTop;
        left += parent.offsetLeft;

        parent = parent.offsetParent;
      }

      return {x: left, y: top};
    }
  });
}


/**
 * The manual css-selector feature implementation
 *
 * Credits:
 *   - Sizzle    (http://sizzlejs.org)      Copyright (C) John Resig
 *   - MooTools  (http://mootools.net)      Copyright (C) Valerio Proietti
 *
 * Copyright (C) 2009-2011 Nikolay V. Nemshilov
 */
var has_native_css_selector = !!document.querySelector,
    needs_css_engine_patch  = !has_native_css_selector;

if (RightJS.Browser.IE8L) {
  needs_css_engine_patch = true;
}

if (needs_css_engine_patch) {
  /**
   * The token searchers collection
   */
  var search = {
    // search for any descendant nodes
    ' ': function(element, tag) {
      return RightJS.$A(element.getElementsByTagName(tag));
    },

    // search for immidate descendant nodes
    '>': function(element, tag) {
      var result = [], node = element.firstChild;
      while (node) {
        if (tag === '*' || node.tagName === tag) {
          result.push(node);
        }
        node = node.nextSibling;
      }
      return result;
    },

    // search for immiate sibling nodes
    '+': function(element, tag) {
      while ((element = element.nextSibling)) {
        if (element.tagName) {
          return (tag === '*' || element.tagName === tag) ? [element] : [];
        }
      }
      return [];
    },

    // search for late sibling nodes
    '~': function(element, tag) {
      var result = [];
      while ((element = element.nextSibling)) {
        if (tag === '*' || element.tagName === tag) {
          result.push(element);
        }
      }
      return result;
    }
  },


  /**
   * Collection of pseudo selector matchers
   */
  pseudos = {
    not: function(node, css_rule) {
      return node.nodeType === 1 && !RightJS.$(node).match(css_rule);
    },

    checked: function(node) {
      return node.checked === true;
    },

    enabled: function(node) {
      return node.disabled === false;
    },

    disabled: function(node) {
      return node.disabled === true;
    },

    selected: function(node) {
      return node.selected === true;
    },

    empty: function(node) {
      return !node.firstChild;
    },

    'first-child': function(node, node_name) {
      while ((node = node.previousSibling)) {
        if (node.nodeType === 1 && (node_name === null || node.nodeName === node_name)) {
          return false;
        }
      }
      return true;
    },

    'first-of-type': function(node) {
      return pseudos['first-child'](node, node.nodeName);
    },

    'last-child': function(node, node_name) {
      while ((node = node.nextSibling)) {
        if (node.nodeType === 1 && (node_name === null || node.nodeName === node_name)) {
          return false;
        }
      }
      return true;
    },

    'last-of-type': function(node) {
      return pseudos['last-child'](node, node.nodeName);
    },

    'only-child': function(node, node_name) {
      return pseudos['first-child'](node, node_name) &&
        pseudos['last-child'](node, node_name);
    },

    'only-of-type': function(node) {
      return pseudos['only-child'](node, node.nodeName);
    },

    'nth-child': function(node, number, node_name, reverse) {
      var index = 1, a = number[0], b = number[1];

      while ((node = (reverse === true) ? node.nextSibling : node.previousSibling)) {
        if (node.nodeType === 1 && (node_name === undefined || node.nodeName === node_name)) {
          index++;
        }
      }

      return (b === undefined ? (index === a) : ((index - b) % a === 0 && (index - b) / a >= 0));
    },

    'nth-of-type': function(node, number) {
      return pseudos['nth-child'](node, number, node.nodeName);
    },

    'nth-last-child': function(node, number) {
      return pseudos['nth-child'](node, number, undefined, true);
    },

    'nth-last-of-type': function(node, number) {
      return pseudos['nth-child'](node, number, node.nodeName, true);
    }
  },

  // the regexps collection
  chunker   = /((?:\((?:\([^()]+\)|[^()]+)+\)|\[(?:\[[^\[\]]*\]|['"][^'"]*['"]|[^\[\]'"]+)+\]|\\.|[^ >+~,(\[\\]+)+|[>+~])(\s*,\s*)?/g,
  id_re     = /#([\w\-_]+)/,
  tag_re    = /^[\w\*]+/,
  class_re  = /\.([\w\-\._]+)/,
  pseudo_re = /:([\w\-]+)(\((.+?)\))*$/,
  attrs_re  = /\[((?:[\w\-]*:)?[\w\-]+)\s*(?:([!\^$*~|]?=)\s*((['"])([^\4]*?)\4|([^'"][^\]]*?)))?\]/,

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

  /**
   * Builds an atom matcher
   *
   * @param String atom definition
   * @return Object atom matcher
   */
  atoms_cache = {},
  build_atom = function(in_atom) {
    if (!atoms_cache[in_atom]) {
      var id, tag, classes, classes_length, attrs, pseudo,
        values_of_pseudo, match, func, desc = {}, atom = in_atom;

      // grabbing the attributes
      while((match = atom.match(attrs_re))) {
        attrs = attrs || {};
        attrs[match[1]] = { o: match[2] || '', v: match[5] || match[6] || '' };
        atom = atom.replace(match[0], '');
      }

      // extracting the pseudos
      if ((match = atom.match(pseudo_re))) {
        pseudo = match[1];
        values_of_pseudo = match[3] === '' ? null : match[3];

        if (pseudo.startsWith('nth')) {
          // preparsing the nth-child pseoudo numbers
          values_of_pseudo = values_of_pseudo.toLowerCase();

          if (values_of_pseudo === 'n') {
            // no need in the pseudo then
            pseudo = null;
            values_of_pseudo = null;
          } else {
            if (values_of_pseudo === 'odd')  { values_of_pseudo = '2n+1'; }
            if (values_of_pseudo === 'even') { values_of_pseudo = '2n';   }

            var m = /^([+\-]?\d*)?n([+\-]?\d*)?$/.exec(values_of_pseudo);
            if (m) {
              values_of_pseudo = [
                m[1] === '-' ? -1 : parseInt(m[1], 10) || 1,
                parseInt(m[2], 10) || 0
              ];
            } else {
              values_of_pseudo = [parseInt(values_of_pseudo, 10), undefined];
            }
          }
        }

        atom = atom.replace(match[0], '');
      }

      // getting all the other options
      id      = (atom.match(id_re)    || [1, null])[1];
      tag     = (atom.match(tag_re)   || '*').toString().toUpperCase();
      classes = (atom.match(class_re) || [1, ''])[1].split('.').without('');
      classes_length = classes.length;

      desc.tag = tag;

      if (id || classes.length || attrs || pseudo) {
        // optimizing a bit the values for quiker runtime checks
        id      = id     || false;
        attrs   = attrs  || false;
        pseudo  = pseudo in pseudos ? pseudos[pseudo] : false;
        classes = classes_length ? classes : false;

        desc.filter = function(elements) {
          var node, result = [], i=0, j=0, l = elements.length, failed;
          for (; i < l; i++) {
            node   = elements[i];

            //////////////////////////////////////////////
            // ID check
            //
            if (id !== false && node.id !== id) {
              continue;
            }

            //////////////////////////////////////////////
            // Class names check
            if (classes !== false) {
              var names = node.className.split(' '),
                  names_length = names.length,
                  x = 0; failed = false;

              for (; x < classes_length; x++) {
                for (var y=0, found = false; y < names_length; y++) {
                  if (classes[x] === names[y]) {
                    found = true;
                    break;
                  }
                }

                if (!found) {
                  failed = true;
                  break;
                }
              }

              if (failed) { continue; }
            }

            ///////////////////////////////////////////////
            // Attributes check
            if (attrs !== false) {
              var key, attr, operand, value; failed = false;
              for (key in attrs) {
                attr = key === 'class' ? node.className : (node.getAttribute(key) || '');
                operand = attrs[key].o;
                value   = attrs[key].v;

                if (
                  (operand === ''   && (key === 'class'|| key === 'lang' ?
                    (attr === '') : (node.getAttributeNode(key) === null))) ||
                  (operand === '='  && attr !== value) ||
                  (operand === '*=' && attr.indexOf(value) === -1) ||
                  (operand === '^=' && attr.indexOf(value) !== 0)  ||
                  (operand === '$=' && attr.substring(attr.length - value.length) !== value) ||
                  (operand === '~=' && attr.split(' ').indexOf(value) === -1) ||
                  (operand === '|=' && attr.split('-').indexOf(value) === -1)
                ) { failed = true; break; }
              }

              if (failed) { continue; }
            }

            ///////////////////////////////////////////////
            // Pseudo selectors check
            if (pseudo !== false) {
              if (!pseudo(node, values_of_pseudo)) {
                continue;
              }
            }

            result[j++] = node;
          }
          return result;
        };
      }

      atoms_cache[in_atom] = desc;
    }

    return atoms_cache[in_atom];
  },

  /**
   * Builds a single selector out of a simple rule chunk
   *
   * @param Array of a single rule tokens
   * @return Function selector
   */
  tokens_cache = {},
  build_selector = function(rule) {
    var rule_key = rule.join('');
    if (!tokens_cache[rule_key]) {
      for (var i=0; i < rule.length; i++) {
        rule[i][1] = build_atom(rule[i][1]);
      }

      // creates a list of uniq nodes
      var _uid = RightJS.$uid;
      var uniq = function(elements) {
        var uniq = [], uids = [], uid;
        for (var i=0, length = elements.length; i < length; i++) {
          uid = _uid(elements[i]);
          if (!uids[uid]) {
            uniq.push(elements[i]);
            uids[uid] = true;
          }
        }

        return uniq;
      };

      // performs the actual search of subnodes
      var find_subnodes = function(element, atom) {
        var result = search[atom[0]](element, atom[1].tag);
        return atom[1].filter ? atom[1].filter(result) : result;
      };

      // building the actual selector function
      tokens_cache[rule_key] = function(element) {
        var founds, sub_founds;

        for (var i=0, i_length = rule.length; i < i_length; i++) {
          if (i === 0) {
            founds = find_subnodes(element, rule[i]);

          } else {
            if (i > 1) { founds = uniq(founds); }

            for (var j=0; j < founds.length; j++) {
              sub_founds = find_subnodes(founds[j], rule[i]);

              sub_founds.unshift(1); // <- nuke the parent node out of the list
              sub_founds.unshift(j); // <- position to insert the subresult

              founds.splice.apply(founds, sub_founds);

              j += sub_founds.length - 3;
            }
          }
        }

        return rule.length > 1 ? uniq(founds) : founds;
      };
    }
    return tokens_cache[rule_key];
  },


  /**
   * Builds the list of selectors for the css_rule
   *
   * @param String raw css-rule
   * @return Array of selectors
   */
  selectors_cache = {}, chunks_cache = {},
  split_rule_to_selectors = function(css_rule) {
    if (!selectors_cache[css_rule]) {
      chunker.lastIndex = 0;

      var rules = [], rule = [], rel = ' ', m, token;
      while ((m = chunker.exec(css_rule))) {
        token = m[1];

        if (token === '+' || token === '>' || token === '~') {
          rel = token;
        } else {
          rule.push([rel, token]);
          rel = ' ';
        }

        if (m[2]) {
          rules.push(build_selector(rule));
          rule = [];
        }
      }
      rules.push(build_selector(rule));

      selectors_cache[css_rule] = rules;
    }
    return selectors_cache[css_rule];
  },


  /**
   * The top level method, it just goes throught the css-rule chunks
   * collect and merge the results that's it
   *
   * @param Element context
   * @param String raw css-rule
   * @return Array search result
   */
  select_all = function(element, css_rule) {
    var selectors = split_rule_to_selectors(css_rule), result = [];
    for (var i=0, length = selectors.length; i < length; i++) {
      result = result.concat(selectors[i](element));
    }

    return result;
  },


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

  // the previous dom-selection methods replacement
  dom_extension = {
    first: function(css_rule) {
      return this.find(css_rule)[0];
    },

    find: function(css_rule, raw) {
      var result, rule = css_rule || '*', element = this._, tag = element.tagName;

      if (has_native_css_selector) {
        try { // trying to reuse native css-engine under IE8
          result = $A(element.querySelectorAll(rule));
        } catch(e) { // if it fails use our own engine
          result = select_all(element, rule);
        }
      } else {
        result = select_all(element, rule);
      }

      return raw === true ? result : result.map(RightJS.$);
    }
  };

  // hooking up the rightjs wrappers with the new methods
  RightJS.$ext(RightJS.Element.prototype, dom_extension);
  RightJS.$ext(RightJS.Document.prototype, dom_extension);
}
})(RightJS);