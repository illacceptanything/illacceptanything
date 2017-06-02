/**
 * Copyright 2013-2015, Facebook, Inc.
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
var ReactInstanceMap;
var ReactTestUtils;
var reactComponentExpect;

var TestComponent;

describe('ReactCompositeComponent-state', function() {

  beforeEach(function() {
    React = require('React');
    ReactInstanceMap = require('ReactInstanceMap');
    ReactTestUtils = require('ReactTestUtils');
    reactComponentExpect = require('reactComponentExpect');

    TestComponent = React.createClass({
      peekAtState: function(from, state) {
        state = state || this.state;
        this.props.stateListener(from, state && state.color);
      },

      peekAtCallback: function(from) {
        return () => this.peekAtState(from);
      },

      setFavoriteColor: function(nextColor) {
        this.setState(
          {color: nextColor},
          this.peekAtCallback('setFavoriteColor')
        );
      },

      getInitialState: function() {
        this.peekAtState('getInitialState');
        return {color: 'red'};
      },

      render: function() {
        this.peekAtState('render');
        return <div>{this.state.color}</div>;
      },

      componentWillMount: function() {
        this.peekAtState('componentWillMount-start');
        this.setState(function(state) {
          this.peekAtState('before-setState-sunrise', state);
        });
        this.setState(
          {color: 'sunrise'},
          this.peekAtCallback('setState-sunrise')
        );
        this.setState(function(state) {
          this.peekAtState('after-setState-sunrise', state);
        });
        this.peekAtState('componentWillMount-after-sunrise');
        this.setState(
          {color: 'orange'},
          this.peekAtCallback('setState-orange')
        );
        this.setState(function(state) {
          this.peekAtState('after-setState-orange', state);
        });
        this.peekAtState('componentWillMount-end');
      },

      componentDidMount: function() {
        this.peekAtState('componentDidMount-start');
        this.setState(
          {color: 'yellow'},
          this.peekAtCallback('setState-yellow')
        );
        this.peekAtState('componentDidMount-end');
      },

      componentWillReceiveProps: function(newProps) {
        this.peekAtState('componentWillReceiveProps-start');
        if (newProps.nextColor) {
          this.setState(function(state) {
            this.peekAtState('before-setState-receiveProps', state);
            return {color: newProps.nextColor};
          });
          this.replaceState({color: undefined});
          this.setState(
            function(state) {
              this.peekAtState('before-setState-again-receiveProps', state);
              return {color: newProps.nextColor};
            },
            this.peekAtCallback('setState-receiveProps')
          );
          this.setState(function(state) {
            this.peekAtState('after-setState-receiveProps', state);
          });
        }
        this.peekAtState('componentWillReceiveProps-end');
      },

      shouldComponentUpdate: function(nextProps, nextState) {
        this.peekAtState('shouldComponentUpdate-currentState');
        this.peekAtState('shouldComponentUpdate-nextState', nextState);
        return true;
      },

      componentWillUpdate: function(nextProps, nextState) {
        this.peekAtState('componentWillUpdate-currentState');
        this.peekAtState('componentWillUpdate-nextState', nextState);
      },

      componentDidUpdate: function(prevProps, prevState) {
        this.peekAtState('componentDidUpdate-currentState');
        this.peekAtState('componentDidUpdate-prevState', prevState);
      },

      componentWillUnmount: function() {
        this.peekAtState('componentWillUnmount');
      }
    });

  });

  it('should support setting state', function() {
    var container = document.createElement('div');
    document.documentElement.appendChild(container);

    var stateListener = mocks.getMockFunction();
    var instance = React.render(
      <TestComponent stateListener={stateListener} />,
      container,
      function peekAtInitialCallback() {
        this.peekAtState('initial-callback');
      }
    );
    instance.setProps(
      {nextColor: 'green'},
      instance.peekAtCallback('setProps')
    );
    instance.setFavoriteColor('blue');
    instance.forceUpdate(instance.peekAtCallback('forceUpdate'));

    React.unmountComponentAtNode(container);

    expect(stateListener.mock.calls).toEqual([
      // there is no state when getInitialState() is called
      ['getInitialState', null],
      ['componentWillMount-start', 'red'],
      // setState()'s only enqueue pending states.
      ['componentWillMount-after-sunrise', 'red'],
      ['componentWillMount-end', 'red'],
      // pending state queue is processed
      ['before-setState-sunrise', 'red'],
      ['after-setState-sunrise', 'sunrise'],
      ['after-setState-orange', 'orange'],
      // pending state has been applied
      ['render', 'orange'],
      ['componentDidMount-start', 'orange'],
      // setState-sunrise and setState-orange should be called here,
      // after the bug in #1740
      // componentDidMount() called setState({color:'yellow'}), which is async.
      // The update doesn't happen until the next flush.
      ['componentDidMount-end', 'orange'],
      ['shouldComponentUpdate-currentState', 'orange'],
      ['shouldComponentUpdate-nextState', 'yellow'],
      ['componentWillUpdate-currentState', 'orange'],
      ['componentWillUpdate-nextState', 'yellow'],
      ['render', 'yellow'],
      ['componentDidUpdate-currentState', 'yellow'],
      ['componentDidUpdate-prevState', 'orange'],
      ['setState-yellow', 'yellow'],
      ['initial-callback', 'yellow'],
      ['componentWillReceiveProps-start', 'yellow'],
      // setState({color:'green'}) only enqueues a pending state.
      ['componentWillReceiveProps-end', 'yellow'],
      // pending state queue is processed
      // before-setState-receiveProps never called, due to replaceState.
      ['before-setState-again-receiveProps', undefined],
      ['after-setState-receiveProps', 'green'],
      ['shouldComponentUpdate-currentState', 'yellow'],
      ['shouldComponentUpdate-nextState', 'green'],
      ['componentWillUpdate-currentState', 'yellow'],
      ['componentWillUpdate-nextState', 'green'],
      ['render', 'green'],
      ['componentDidUpdate-currentState', 'green'],
      ['componentDidUpdate-prevState', 'yellow'],
      ['setState-receiveProps', 'green'],
      ['setProps', 'green'],
      // setFavoriteColor('blue')
      ['shouldComponentUpdate-currentState', 'green'],
      ['shouldComponentUpdate-nextState', 'blue'],
      ['componentWillUpdate-currentState', 'green'],
      ['componentWillUpdate-nextState', 'blue'],
      ['render', 'blue'],
      ['componentDidUpdate-currentState', 'blue'],
      ['componentDidUpdate-prevState', 'green'],
      ['setFavoriteColor', 'blue'],
      // forceUpdate()
      ['componentWillUpdate-currentState', 'blue'],
      ['componentWillUpdate-nextState', 'blue'],
      ['render', 'blue'],
      ['componentDidUpdate-currentState', 'blue'],
      ['componentDidUpdate-prevState', 'blue'],
      ['forceUpdate', 'blue'],
      // unmountComponent()
      // state is available within `componentWillUnmount()`
      ['componentWillUnmount', 'blue']
    ]);
  });
});
