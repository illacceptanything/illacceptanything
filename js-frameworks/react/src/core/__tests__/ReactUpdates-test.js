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

var React;
var ReactTestUtils;
var ReactUpdates;

describe('ReactUpdates', function() {
  beforeEach(function() {
    React = require('React');
    ReactTestUtils = require('ReactTestUtils');
    ReactUpdates = require('ReactUpdates');
  });

  it('should batch state when updating state twice', function() {
    var updateCount = 0;
    var Component = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      componentDidUpdate: function() {
        updateCount++;
      },
      render: function() {
        return <div>{this.state.x}</div>;
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Component />);
    expect(instance.state.x).toBe(0);

    ReactUpdates.batchedUpdates(function() {
      instance.setState({x: 1});
      instance.setState({x: 2});
      expect(instance.state.x).toBe(0);
      expect(updateCount).toBe(0);
    });

    expect(instance.state.x).toBe(2);
    expect(updateCount).toBe(1);
  });

  it('should batch state when updating two different state keys', function() {
    var updateCount = 0;
    var Component = React.createClass({
      getInitialState: function() {
        return {x: 0, y: 0};
      },
      componentDidUpdate: function() {
        updateCount++;
      },
      render: function() {
        return <div>({this.state.x}, {this.state.y})</div>;
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Component />);
    expect(instance.state.x).toBe(0);
    expect(instance.state.y).toBe(0);

    ReactUpdates.batchedUpdates(function() {
      instance.setState({x: 1});
      instance.setState({y: 2});
      expect(instance.state.x).toBe(0);
      expect(instance.state.y).toBe(0);
      expect(updateCount).toBe(0);
    });

    expect(instance.state.x).toBe(1);
    expect(instance.state.y).toBe(2);
    expect(updateCount).toBe(1);
  });

  it('should batch state and props together', function() {
    var updateCount = 0;
    var Component = React.createClass({
      getInitialState: function() {
        return {y: 0};
      },
      componentDidUpdate: function() {
        updateCount++;
      },
      render: function() {
        return <div>({this.props.x}, {this.state.y})</div>;
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Component x={0} />);
    expect(instance.props.x).toBe(0);
    expect(instance.state.y).toBe(0);

    ReactUpdates.batchedUpdates(function() {
      instance.setProps({x: 1});
      instance.setState({y: 2});
      expect(instance.props.x).toBe(0);
      expect(instance.state.y).toBe(0);
      expect(updateCount).toBe(0);
    });

    expect(instance.props.x).toBe(1);
    expect(instance.state.y).toBe(2);
    expect(updateCount).toBe(1);
  });

  it('should batch parent/child state updates together', function() {
    var parentUpdateCount = 0;
    var Parent = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      componentDidUpdate: function() {
        parentUpdateCount++;
      },
      render: function() {
        return <div><Child ref="child" x={this.state.x} /></div>;
      }
    });
    var childUpdateCount = 0;
    var Child = React.createClass({
      getInitialState: function() {
        return {y: 0};
      },
      componentDidUpdate: function() {
        childUpdateCount++;
      },
      render: function() {
        return <div>{this.props.x + this.state.y}</div>;
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Parent />);
    var child = instance.refs.child;
    expect(instance.state.x).toBe(0);
    expect(child.state.y).toBe(0);

    ReactUpdates.batchedUpdates(function() {
      instance.setState({x: 1});
      child.setState({y: 2});
      expect(instance.state.x).toBe(0);
      expect(child.state.y).toBe(0);
      expect(parentUpdateCount).toBe(0);
      expect(childUpdateCount).toBe(0);
    });

    expect(instance.state.x).toBe(1);
    expect(child.state.y).toBe(2);
    expect(parentUpdateCount).toBe(1);
    expect(childUpdateCount).toBe(1);
  });

  it('should batch child/parent state updates together', function() {
    var parentUpdateCount = 0;
    var Parent = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      componentDidUpdate: function() {
        parentUpdateCount++;
      },
      render: function() {
        return <div><Child ref="child" x={this.state.x} /></div>;
      }
    });
    var childUpdateCount = 0;
    var Child = React.createClass({
      getInitialState: function() {
        return {y: 0};
      },
      componentDidUpdate: function() {
        childUpdateCount++;
      },
      render: function() {
        return <div>{this.props.x + this.state.y}</div>;
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Parent />);
    var child = instance.refs.child;
    expect(instance.state.x).toBe(0);
    expect(child.state.y).toBe(0);

    ReactUpdates.batchedUpdates(function() {
      child.setState({y: 2});
      instance.setState({x: 1});
      expect(instance.state.x).toBe(0);
      expect(child.state.y).toBe(0);
      expect(parentUpdateCount).toBe(0);
      expect(childUpdateCount).toBe(0);
    });

    expect(instance.state.x).toBe(1);
    expect(child.state.y).toBe(2);
    expect(parentUpdateCount).toBe(1);

    // Batching reduces the number of updates here to 1.
    expect(childUpdateCount).toBe(1);
  });

  it('should support chained state updates', function() {
    var updateCount = 0;
    var Component = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      componentDidUpdate: function() {
        updateCount++;
      },
      render: function() {
        return <div>{this.state.x}</div>;
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Component />);
    expect(instance.state.x).toBe(0);

    var innerCallbackRun = false;
    ReactUpdates.batchedUpdates(function() {
      instance.setState({x: 1}, function() {
        instance.setState({x: 2}, function() {
          expect(this).toBe(instance);
          innerCallbackRun = true;
          expect(instance.state.x).toBe(2);
          expect(updateCount).toBe(2);
        });
        expect(instance.state.x).toBe(1);
        expect(updateCount).toBe(1);
      });
      expect(instance.state.x).toBe(0);
      expect(updateCount).toBe(0);
    });

    expect(innerCallbackRun).toBeTruthy();
    expect(instance.state.x).toBe(2);
    expect(updateCount).toBe(2);
  });

  it('should batch forceUpdate together', function() {
    var shouldUpdateCount = 0;
    var updateCount = 0;
    var Component = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      shouldComponentUpdate: function() {
        shouldUpdateCount++;
      },
      componentDidUpdate: function() {
        updateCount++;
      },
      render: function() {
        return <div>{this.state.x}</div>;
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Component />);
    expect(instance.state.x).toBe(0);

    var callbacksRun = 0;
    ReactUpdates.batchedUpdates(function() {
      instance.setState({x: 1}, function() {
        callbacksRun++;
      });
      instance.forceUpdate(function() {
        callbacksRun++;
      });
      expect(instance.state.x).toBe(0);
      expect(updateCount).toBe(0);
    });

    expect(callbacksRun).toBe(2);
    // shouldComponentUpdate shouldn't be called since we're forcing
    expect(shouldUpdateCount).toBe(0);
    expect(instance.state.x).toBe(1);
    expect(updateCount).toBe(1);
  });

  it('should update children even if parent blocks updates', function() {
    var parentRenderCount = 0;
    var childRenderCount = 0;

    var Parent = React.createClass({
      shouldComponentUpdate: function() {
        return false;
      },

      render: function() {
        parentRenderCount++;
        return <Child ref="child" />;
      }
    });

    var Child = React.createClass({
      render: function() {
        childRenderCount++;
        return <div />;
      }
    });

    expect(parentRenderCount).toBe(0);
    expect(childRenderCount).toBe(0);

    var instance = <Parent />;
    instance = ReactTestUtils.renderIntoDocument(instance);

    expect(parentRenderCount).toBe(1);
    expect(childRenderCount).toBe(1);

    ReactUpdates.batchedUpdates(function() {
      instance.setState({x: 1});
    });

    expect(parentRenderCount).toBe(1);
    expect(childRenderCount).toBe(1);

    ReactUpdates.batchedUpdates(function() {
      instance.refs.child.setState({x: 1});
    });

    expect(parentRenderCount).toBe(1);
    expect(childRenderCount).toBe(2);
  });

  it('should not reconcile children passed via props', function() {
    var numMiddleRenders = 0;
    var numBottomRenders = 0;

    var Top = React.createClass({
      render: function() {
        return <Middle><Bottom /></Middle>;
      }
    });

    var Middle = React.createClass({
      componentDidMount: function() {
        this.forceUpdate();
      },

      render: function() {
        numMiddleRenders++;
        return <div>{this.props.children}</div>;
      }
    });

    var Bottom = React.createClass({
      render: function() {
        numBottomRenders++;
        return <span />;
      }
    });

    ReactTestUtils.renderIntoDocument(<Top />);
    expect(numMiddleRenders).toBe(2);
    expect(numBottomRenders).toBe(1);
  });

  it('should flow updates correctly', function() {
    var willUpdates = [];
    var didUpdates = [];

    var UpdateLoggingMixin = {
      componentWillUpdate: function() {
        willUpdates.push(this.constructor.displayName);
      },
      componentDidUpdate: function() {
        didUpdates.push(this.constructor.displayName);
      }
    };

    var Box = React.createClass({
      mixins: [UpdateLoggingMixin],

      render: function() {
        return <div ref="boxDiv">{this.props.children}</div>;
      }
    });

    var Child = React.createClass({
      mixins: [UpdateLoggingMixin],

      render: function() {
        return <span ref="span">child</span>;
      }
    });

    var Switcher = React.createClass({
      mixins: [UpdateLoggingMixin],

      getInitialState: function() {
        return {tabKey: 'hello'};
      },

      render: function() {
        var child = this.props.children;

        return (
          <Box ref="box">
            <div
              ref="switcherDiv"
              style={{
                display: this.state.tabKey === child.key ? '' : 'none'
            }}>
              {child}
            </div>
          </Box>
        );
      }
    });

    var App = React.createClass({
      mixins: [UpdateLoggingMixin],

      render: function() {
        return (
          <Switcher ref="switcher">
            <Child key="hello" ref="child" />
          </Switcher>
        );
      }
    });

    var root = <App />;
    root = ReactTestUtils.renderIntoDocument(root);

    function expectUpdates(desiredWillUpdates, desiredDidUpdates) {
      expect(willUpdates).toEqual(desiredWillUpdates);
      expect(didUpdates).toEqual(desiredDidUpdates);
      willUpdates.length = 0;
      didUpdates.length = 0;
    }

    function triggerUpdate(c) {
      c.setState({x: 1});
    }

    function testUpdates(components, desiredWillUpdates, desiredDidUpdates) {
      var i;

      ReactUpdates.batchedUpdates(function() {
        for (i = 0; i < components.length; i++) {
          triggerUpdate(components[i]);
        }
      });

      expectUpdates(desiredWillUpdates, desiredDidUpdates);

      // Try them in reverse order

      ReactUpdates.batchedUpdates(function() {
        for (i = components.length - 1; i >= 0; i--) {
          triggerUpdate(components[i]);
        }
      });

      expectUpdates(desiredWillUpdates, desiredDidUpdates);
    }

    testUpdates(
      [root.refs.switcher.refs.box, root.refs.switcher],
      // Owner-child relationships have inverse will and did
      ['Switcher', 'Box'],
      ['Box', 'Switcher']
    );

    testUpdates(
      [root.refs.child, root.refs.switcher.refs.box],
      // Not owner-child so reconcile independently
      ['Box', 'Child'],
      ['Box', 'Child']
    );

    testUpdates(
      [root.refs.child, root.refs.switcher],
      // Switcher owns Box and Child, Box does not own Child
      ['Switcher', 'Box', 'Child'],
      ['Box', 'Switcher', 'Child']
    );
  });

  it('should share reconcile transaction across different roots', function() {
    var ReconcileTransaction = ReactUpdates.ReactReconcileTransaction;
    spyOn(ReconcileTransaction, 'getPooled').andCallThrough();

    var Component = React.createClass({
      render: function() {
        return <div>{this.props.text}</div>;
      }
    });

    var containerA = document.createElement('div');
    var containerB = document.createElement('div');

    // Initial renders aren't batched together yet...
    ReactUpdates.batchedUpdates(function() {
      React.render(<Component text="A1" />, containerA);
      React.render(<Component text="B1" />, containerB);
    });
    expect(ReconcileTransaction.getPooled.calls.length).toBe(2);

    // ...but updates are! Here only one more transaction is used, which means
    // we only have to initialize and close the wrappers once.
    ReactUpdates.batchedUpdates(function() {
      React.render(<Component text="A2" />, containerA);
      React.render(<Component text="B2" />, containerB);
    });
    expect(ReconcileTransaction.getPooled.calls.length).toBe(3);
  });

  it('should queue mount-ready handlers across different roots', function() {
    // We'll define two components A and B, then update both of them. When A's
    // componentDidUpdate handlers is called, B's DOM should already have been
    // updated.

    var a;
    var b;

    var aUpdated = false;

    var A = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      componentDidUpdate: function() {
        expect(React.findDOMNode(b).textContent).toBe("B1");
        aUpdated = true;
      },
      render: function() {
        return <div>A{this.state.x}</div>;
      }
    });

    var B = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      render: function() {
        return <div>B{this.state.x}</div>;
      }
    });

    a = ReactTestUtils.renderIntoDocument(<A />);
    b = ReactTestUtils.renderIntoDocument(<B />);

    ReactUpdates.batchedUpdates(function() {
      a.setState({x: 1});
      b.setState({x: 1});
    });

    expect(aUpdated).toBe(true);
  });

  it('should flush updates in the correct order', function() {
    var updates = [];
    var Outer = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      render: function() {
        updates.push('Outer-render-' + this.state.x);
        return <div><Inner x={this.state.x} ref="inner" /></div>;
      },
      componentDidUpdate: function() {
        var x = this.state.x;
        updates.push('Outer-didUpdate-' + x);
        updates.push('Inner-setState-' + x);
        this.refs.inner.setState({x: x}, function() {
          updates.push('Inner-callback-' + x);
        });
      }
    });
    var Inner = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      render: function() {
        updates.push('Inner-render-' + this.props.x + '-' + this.state.x);
        return <div />;
      },
      componentDidUpdate: function() {
        updates.push('Inner-didUpdate-' + this.props.x + '-' + this.state.x);
      }
    });

    var instance = ReactTestUtils.renderIntoDocument(<Outer />);

    updates.push('Outer-setState-1');
    instance.setState({x: 1}, function() {
      updates.push('Outer-callback-1');
      updates.push('Outer-setState-2');
      instance.setState({x: 2}, function() {
        updates.push('Outer-callback-2');
      });
    });

    expect(updates).toEqual([
      'Outer-render-0',
        'Inner-render-0-0',

      'Outer-setState-1',
        'Outer-render-1',
          'Inner-render-1-0',
          'Inner-didUpdate-1-0',
        'Outer-didUpdate-1',
          'Inner-setState-1',
            'Inner-render-1-1',
            'Inner-didUpdate-1-1',
          'Inner-callback-1',
      'Outer-callback-1',

      'Outer-setState-2',
        'Outer-render-2',
          'Inner-render-2-1',
          'Inner-didUpdate-2-1',
        'Outer-didUpdate-2',
          'Inner-setState-2',
            'Inner-render-2-2',
            'Inner-didUpdate-2-2',
          'Inner-callback-2',
      'Outer-callback-2'
    ]);
  });

  it('should flush updates in the correct order across roots', function() {
    var instances = [];
    var updates = [];

    var MockComponent = React.createClass({
      render: function() {
        updates.push(this.props.depth);
        return <div />;
      },
      componentDidMount: function() {
        instances.push(this);
        if (this.props.depth < this.props.count) {
          React.render(
            <MockComponent
              depth={this.props.depth + 1}
              count={this.props.count}
            />,
            React.findDOMNode(this)
          );
        }
      }
    });

    ReactTestUtils.renderIntoDocument(<MockComponent depth={0} count={2} />);

    expect(updates).toEqual([0, 1, 2]);

    ReactUpdates.batchedUpdates(function() {
      // Simulate update on each component from top to bottom.
      instances.forEach(function(instance) {
        instance.forceUpdate();
      });
    });

    expect(updates).toEqual([0, 1, 2, 0, 1, 2]);
  });

  it('should queue nested updates', function() {
    // See https://github.com/facebook/react/issues/1147

    var X = React.createClass({
      getInitialState: function() {
        return {s: 0};
      },
      render: function() {
        if (this.state.s === 0) {
          return <div>
            <span>0</span>
          </div>;
        } else {
          return <div>1</div>;
        }
      },
      go: function() {
        this.setState({s: 1});
        this.setState({s: 0});
        this.setState({s: 1});
      }
    });

    var Y = React.createClass({
      render: function() {
        return <div>
          <Z />
        </div>;
      }
    });

    var Z = React.createClass({
      render: function() { return <div />; },
      componentWillUpdate: function() {
        x.go();
      }
    });

    var x;
    var y;

    x = ReactTestUtils.renderIntoDocument(<X />);
    y = ReactTestUtils.renderIntoDocument(<Y />);
    expect(React.findDOMNode(x).textContent).toBe('0');

    y.forceUpdate();
    expect(React.findDOMNode(x).textContent).toBe('1');
  });

  it('should queue updates from during mount', function() {
    // See https://github.com/facebook/react/issues/1353
    var a;

    var A = React.createClass({
      getInitialState: function() {
        return {x: 0};
      },
      componentWillMount: function() {
        a = this;
      },
      render: function() {
        return <div>A{this.state.x}</div>;
      }
    });

    var B = React.createClass({
      componentWillMount: function() {
        a.setState({x: 1});
      },
      render: function() {
        return <div />;
      }
    });

    ReactUpdates.batchedUpdates(function() {
      ReactTestUtils.renderIntoDocument(
        <div>
          <A />
          <B />
        </div>
      );
    });

    expect(a.state.x).toBe(1);
    expect(React.findDOMNode(a).textContent).toBe('A1');
  });

  it('calls componentWillReceiveProps setState callback properly', function() {
    var callbackCount = 0;
    var A = React.createClass({
      getInitialState: function() {
        return {x: this.props.x};
      },
      componentWillReceiveProps: function(nextProps) {
        var newX = nextProps.x;
        this.setState({x: newX}, function() {
          // State should have updated by the time this callback gets called
          expect(this.state.x).toBe(newX);
          callbackCount++;
        });
      },
      render: function() {
        return <div>{this.state.x}</div>;
      }
    });

    var container = document.createElement('div');
    React.render(<A x={1} />, container);
    React.render(<A x={2} />, container);
    expect(callbackCount).toBe(1);
  });

  it('calls asap callbacks properly', function() {
    var callbackCount = 0;
    var A = React.createClass({
      render: function() {
        return <div />;
      },
      componentDidUpdate: function() {
        var component = this;
        ReactUpdates.asap(function() {
          expect(this).toBe(component);
          callbackCount++;
          ReactUpdates.asap(function() {
            callbackCount++;
          });
          expect(callbackCount).toBe(1);
        }, this);
        expect(callbackCount).toBe(0);
      }
    });

    var component = ReactTestUtils.renderIntoDocument(<A />);
    component.forceUpdate();
    expect(callbackCount).toBe(2);
  });

  it('calls asap callbacks with queued updates', function() {
    var log = [];
    var A = React.createClass({
      getInitialState: () => ({updates: 0}),
      render: function() {
        log.push('render-' + this.state.updates);
        return <div />;
      },
      componentDidUpdate: function() {
        if (this.state.updates === 1) {
          ReactUpdates.asap(function() {
            this.setState({updates: 2}, function() {
              ReactUpdates.asap(function() {
                log.push('asap-1.2');
              });
              log.push('setState-cb');
            });
            log.push('asap-1.1');
          }, this);
        } else if (this.state.updates === 2) {
          ReactUpdates.asap(function() {
            log.push('asap-2');
          });
        }
        log.push('didUpdate-' + this.state.updates);
      }
    });

    var component = ReactTestUtils.renderIntoDocument(<A />);
    component.setState({updates: 1});
    expect(log).toEqual([
      'render-0',
      // We do the first update...
      'render-1',
      'didUpdate-1',
      // ...which calls asap and enqueues a second update...
      'asap-1.1',
      // ...which runs and enqueues the asap-2 log in its didUpdate...
      'render-2',
      'didUpdate-2',
      // ...and runs the setState callback, which enqueues the log for
      // asap-1.2.
      'setState-cb',
      'asap-2',
      'asap-1.2'
    ]);
  });

  it('does not call render after a component as been deleted', function() {

    var renderCount = 0;
    var componentB = null;

    var B = React.createClass({
      getInitialState: function() {
        return {updates: 0};
      },
      componentDidMount: function() {
        componentB = this;
      },
      render: function() {
        renderCount++;
        return <div />;
      }
    });

    var A = React.createClass({
      getInitialState: function() {
        return {showB: true};
      },
      render: function() {
        return this.state.showB ? <B /> : <div />;
      }
    });

    var component = ReactTestUtils.renderIntoDocument(<A />);

    ReactUpdates.batchedUpdates(function() {
      // B will have scheduled an update but the batching should ensure that its
      // update never fires.
      componentB.setState({updates: 1});
      component.setState({showB: false});
    });

    expect(renderCount).toBe(1);
  });

});
