/**
 * Copyright 2013-2014, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * @emails react-core
 */

'use strict';

var mocks = require('mocks');

var React;
var ReactTestUtils;

describe('ReactCompositeComponentNestedState-state', function() {

  beforeEach(function() {
    React = require('React');
    ReactTestUtils = require('ReactTestUtils');
  });

  it('should provide up to date values for props', function() {
    var ParentComponent = React.createClass({
      getInitialState: function() {
        return {color: 'blue'};
      },

      handleColor: function(color) {
        this.props.logger('parent-handleColor', this.state.color);
        this.setState({color: color}, function() {
          this.props.logger('parent-after-setState', this.state.color);
        });
      },

      render: function() {
        this.props.logger('parent-render', this.state.color);
        return <ChildComponent
          logger={this.props.logger}
          color={this.state.color}
          onSelectColor={this.handleColor}
        />;
      }
    });

    var ChildComponent = React.createClass({
      getInitialState: function() {
        this.props.logger('getInitialState', this.props.color);
        return {hue: 'dark ' + this.props.color};
      },

      handleHue: function(shade, color) {
        this.props.logger('handleHue', this.state.hue, this.props.color);
        this.props.onSelectColor(color);
        this.setState(function(state, props) {
          this.props.logger('setState-this', this.state.hue, this.props.color);
          this.props.logger('setState-args', state.hue, props.color);
          return {hue: shade + ' ' + props.color}
        }, function() {
          this.props.logger('after-setState', this.state.hue, this.props.color);
        });
      },

      render: function() {
        this.props.logger('render', this.state.hue, this.props.color);
        return <div>
          <button onClick={this.handleHue.bind(this, 'dark', 'blue')}>
            Dark Blue
          </button>
          <button onClick={this.handleHue.bind(this, 'light', 'blue')}>
            Light Blue
          </button>
          <button onClick={this.handleHue.bind(this, 'dark', 'green')}>
            Dark Green
          </button>
          <button onClick={this.handleHue.bind(this, 'light', 'green')}>
            Light Green
          </button>
        </div>
      }
    });

    var container = document.createElement('div');
    document.documentElement.appendChild(container);

    var logger = mocks.getMockFunction();

    var instance = React.render(
      <ParentComponent logger={logger} />,
      container
    );

    // click "light green"
    ReactTestUtils.Simulate.click(
      container.childNodes[0].childNodes[3]
    );

    expect(logger.mock.calls).toEqual([
      ['parent-render', 'blue'],
      ['getInitialState', 'blue'],
      ['render', 'dark blue', 'blue'],
      ['handleHue', 'dark blue', 'blue'],
      ['parent-handleColor', 'blue'],
      ['parent-render', 'green'],
      ['setState-this', 'dark blue', 'blue'],
      ['setState-args', 'dark blue', 'green'],
      ['render', 'light green', 'green'],
      ['parent-after-setState', 'green'],
      ['after-setState', 'light green', 'green']
    ]);
  });
});
