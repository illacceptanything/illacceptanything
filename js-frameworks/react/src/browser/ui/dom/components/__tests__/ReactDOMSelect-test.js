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

/*jshint evil:true */

var mocks = require('mocks');

describe('ReactDOMSelect', function() {
  var React;
  var ReactLink;
  var ReactTestUtils;

  beforeEach(function() {
    React = require('React');
    ReactLink = require('ReactLink');
    ReactTestUtils = require('ReactTestUtils');
  });

  it('should allow setting `defaultValue`', function() {
    var stub =
      <select defaultValue="giraffe">
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.value).toBe('giraffe');

    // Changing `defaultValue` should do nothing.
    stub.setProps({defaultValue: 'gorilla'});
    expect(node.value).toEqual('giraffe');
  });

  it('should not throw with `defaultValue` and without children', function() {
    var stub = <select defaultValue="dummy"></select>;

    expect(() => {
      ReactTestUtils.renderIntoDocument(stub);
    }).not.toThrow();
  });

  it('should not control when using `defaultValue`', function() {
    var stub =
      <select defaultValue="giraffe">
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.value).toBe('giraffe');

    node.value = 'monkey';
    stub.forceUpdate();
    // Uncontrolled selects shouldn't change the value after first mounting
    expect(node.value).toEqual('monkey');
  });

  it('should allow setting `defaultValue` with multiple', function() {
    var stub =
      <select multiple={true} defaultValue={['giraffe', 'gorilla']}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla

    // Changing `defaultValue` should do nothing.
    stub.setProps({defaultValue: ['monkey']});

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla
  });

  it('should allow setting `value`', function() {
    var stub =
      <select value="giraffe">
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.value).toBe('giraffe');

    // Changing the `value` prop should change the selected option.
    stub.setProps({value: 'gorilla'});
    expect(node.value).toEqual('gorilla');
  });

  it('should not throw with `value` and without children', function() {
    var stub = <select value="dummy"></select>;

    expect(() => {
      ReactTestUtils.renderIntoDocument(stub);
    }).not.toThrow();
  });

  it('should allow setting `value` with multiple', function() {
    var stub =
      <select multiple={true} value={['giraffe', 'gorilla']}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla

    // Changing the `value` prop should change the selected options.
    stub.setProps({value: ['monkey']});

    expect(node.options[0].selected).toBe(true);  // monkey
    expect(node.options[1].selected).toBe(false);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla
  });

  it('should not select other options automatically', function() {
    var stub =
      <select multiple={true} value={['12']}>
        <option value="1">one</option>
        <option value="2">two</option>
        <option value="12">twelve</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // one
    expect(node.options[1].selected).toBe(false);  // two
    expect(node.options[2].selected).toBe(true);  // twelve
  });

  it('should reset child options selected when they are changed and `value` is set', function() {
    var stub = <select multiple={true} value={["a", "b"]} />;
    stub = ReactTestUtils.renderIntoDocument(stub);

    stub.setProps({
      children: [
        <option value="a">a</option>,
        <option value="b">b</option>,
        <option value="c">c</option>
      ]
    });

    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(true);  // a
    expect(node.options[1].selected).toBe(true);  // b
    expect(node.options[2].selected).toBe(false);  // c
  });

  it('should allow setting `value` with `objectToString`', function() {
    var objectToString = {
      animal: "giraffe",
      toString: function() {
        return this.animal;
      }
    };

    var stub =
      <select multiple={true} value={[objectToString]}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla

    // Changing the `value` prop should change the selected options.
    objectToString.animal = "monkey";
    stub.forceUpdate();

    expect(node.options[0].selected).toBe(true);  // monkey
    expect(node.options[1].selected).toBe(false);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla
  });

  it('should allow switching to multiple', function() {
    var stub =
      <select defaultValue="giraffe">
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla

    // When making it multiple, giraffe and gorilla should be selected
    stub.setProps({multiple: true, defaultValue: ['giraffe', 'gorilla']});

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla
  });

  it('should allow switching from multiple', function() {
    var stub =
      <select multiple={true} defaultValue={['giraffe', 'gorilla']}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla

    // When removing multiple, defaultValue is applied again, being omitted
    // means that "monkey" will be selected
    stub.setProps({multiple: false, defaultValue: 'gorilla'});

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(false);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla
  });

  it('should remember value when switching to uncontrolled', function() {
    var stub =
      <select value={'giraffe'}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla

    stub.setProps({value: null});

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla
  });

  it('should remember updated value when switching to uncontrolled', function() {
    var stub =
      <select value={'giraffe'}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    stub.setProps({value: 'gorilla'});

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(false);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla

    stub.setProps({value: null});

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(false);  // giraffe
    expect(node.options[2].selected).toBe(true);  // gorilla
  });

  it('should support ReactLink', function() {
    var link = new ReactLink('giraffe', mocks.getMockFunction());
    var stub =
      <select valueLink={link}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    stub = ReactTestUtils.renderIntoDocument(stub);
    var node = React.findDOMNode(stub);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla
    expect(link.requestChange.mock.calls.length).toBe(0);

    node.options[1].selected = false;
    node.options[2].selected = true;
    ReactTestUtils.Simulate.change(node);

    expect(link.requestChange.mock.calls.length).toBe(1);
    expect(link.requestChange.mock.calls[0][0]).toEqual('gorilla');

  });

  it('should support server-side rendering', function() {
    var stub =
      <select value="giraffe">
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    var markup = React.renderToString(stub);
    expect(markup).toContain('<option value="giraffe" selected=');
    expect(markup).not.toContain('<option value="monkey" selected=');
    expect(markup).not.toContain('<option value="gorilla" selected=');
  });

  it('should support server-side rendering with defaultValue', function() {
    var stub =
      <select defaultValue="giraffe">
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    var markup = React.renderToString(stub);
    expect(markup).toContain('<option value="giraffe" selected=');
    expect(markup).not.toContain('<option value="monkey" selected=');
    expect(markup).not.toContain('<option value="gorilla" selected=');
  });

  it('should support server-side rendering with multiple', function() {
    var stub =
      <select multiple={true} value={['giraffe', 'gorilla']}>
        <option value="monkey">A monkey!</option>
        <option value="giraffe">A giraffe!</option>
        <option value="gorilla">A gorilla!</option>
      </select>;
    var markup = React.renderToString(stub);
    expect(markup).toContain('<option value="giraffe" selected=');
    expect(markup).toContain('<option value="gorilla" selected=');
    expect(markup).not.toContain('<option value="monkey" selected=');
  });

  it('should not control defaultValue if readding options', function() {
    var container = document.createElement('div');

    var select = React.render(
      <select multiple={true} defaultValue={['giraffe']}>
        <option key="monkey" value="monkey">A monkey!</option>
        <option key="giraffe" value="giraffe">A giraffe!</option>
        <option key="gorilla" value="gorilla">A gorilla!</option>
      </select>,
      container
    );
    var node = React.findDOMNode(select);

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(true);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla

    React.render(
      <select multiple={true} defaultValue={['giraffe']}>
        <option key="monkey" value="monkey">A monkey!</option>
        <option key="gorilla" value="gorilla">A gorilla!</option>
      </select>,
      container
    );

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(false);  // gorilla

    React.render(
      <select multiple={true} defaultValue={['giraffe']}>
        <option key="monkey" value="monkey">A monkey!</option>
        <option key="giraffe" value="giraffe">A giraffe!</option>
        <option key="gorilla" value="gorilla">A gorilla!</option>
      </select>,
      container
    );

    expect(node.options[0].selected).toBe(false);  // monkey
    expect(node.options[1].selected).toBe(false);  // giraffe
    expect(node.options[2].selected).toBe(false);  // gorilla
  });
});
