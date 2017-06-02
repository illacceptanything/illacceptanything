/**
 * Copyright 2013-2015, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * @providesModule ReactDOMOption
 */

'use strict';

var ReactBrowserComponentMixin = require('ReactBrowserComponentMixin');
var ReactClass = require('ReactClass');
var ReactDOMSelect = require('ReactDOMSelect');
var ReactElement = require('ReactElement');
var ReactInstanceMap = require('ReactInstanceMap');
var ReactPropTypes = require('ReactPropTypes');

var assign = require('Object.assign');
var warning = require('warning');

var option = ReactElement.createFactory('option');

var valueContextKey = ReactDOMSelect.valueContextKey;

/**
 * Implements an <option> native component that warns when `selected` is set.
 */
var ReactDOMOption = ReactClass.createClass({
  displayName: 'ReactDOMOption',
  tagName: 'OPTION',

  mixins: [ReactBrowserComponentMixin],

  getInitialState: function() {
    return {selected: null};
  },

  contextTypes: (function() {
    var obj = {};
    obj[valueContextKey] = ReactPropTypes.any;
    return obj;
  })(),

  componentWillMount: function() {
    // TODO (yungsters): Remove support for `selected` in <option>.
    if (__DEV__) {
      warning(
        this.props.selected == null,
        'Use the `defaultValue` or `value` props on <select> instead of ' +
        'setting `selected` on <option>.'
      );
    }

    // Look up whether this option is 'selected' via parent-based context
    var context = ReactInstanceMap.get(this)._context;
    var selectValue = context[valueContextKey];

    // If context key is null (e.g., no specified value or after initial mount)
    // or missing (e.g., for <datalist>) skip props
    if (selectValue != null) {
      var selected = false;
      if (Array.isArray(selectValue)) {
        // multiple
        for (var i = 0; i < selectValue.length; i++) {
          if ('' + selectValue[i] === '' + this.props.value) {
            selected = true;
            break;
          }
        }
      } else {
        selected = ('' + selectValue === '' + this.props.value);
      }
      this.setState({selected: selected});
    }
  },

  render: function() {
    var props = this.props;

    // Read state only from initial mount because <select> updates value
    // manually; we need the initial state only for server rendering
    if (this.state.selected != null) {
      props = assign({}, props, {selected: this.state.selected});
    }

    return option(props, this.props.children);
  }

});

module.exports = ReactDOMOption;
