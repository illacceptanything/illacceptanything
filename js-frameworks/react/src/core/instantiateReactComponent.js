/**
 * Copyright 2013-2015, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * @providesModule instantiateReactComponent
 * @typechecks static-only
 */

'use strict';

var ReactCompositeComponent = require('ReactCompositeComponent');
var ReactEmptyComponent = require('ReactEmptyComponent');
var ReactNativeComponent = require('ReactNativeComponent');

var assign = require('Object.assign');
var invariant = require('invariant');
var warning = require('warning');

// To avoid a cyclic dependency, we create the final class in this module
var ReactCompositeComponentWrapper = function() { };
assign(
  ReactCompositeComponentWrapper.prototype,
  ReactCompositeComponent.Mixin,
  {
    _instantiateReactComponent: instantiateReactComponent
  }
);

/**
 * Check if the type reference is a known internal type. I.e. not a user
 * provided composite type.
 *
 * @param {function} type
 * @return {boolean} Returns true if this is a valid internal type.
 */
function isInternalComponentType(type) {
  return (
    typeof type === 'function' &&
    typeof type.prototype.mountComponent === 'function' &&
    typeof type.prototype.receiveComponent === 'function'
  );
}

/**
 * Given a ReactNode, create an instance that will actually be mounted.
 *
 * @param {ReactNode} node
 * @param {*} parentCompositeType The composite type that resolved this.
 * @return {object} A new instance of the element's constructor.
 * @protected
 */
function instantiateReactComponent(node, parentCompositeType) {
  var instance;

  if (node === null || node === false) {
    node = ReactEmptyComponent.emptyElement;
  }

  if (typeof node === 'object') {
    var element = node;
    if (__DEV__) {
      warning(
        element && (typeof element.type === 'function' ||
                    typeof element.type === 'string'),
        'Only functions or strings can be mounted as React components.'
      );
    }

    // Special case string values
    if (parentCompositeType === element.type &&
        typeof element.type === 'string') {
      // Avoid recursion if the wrapper renders itself.
      instance = ReactNativeComponent.createInternalComponent(element);
      // All native components are currently wrapped in a composite so we're
      // safe to assume that this is what we should instantiate.
    } else if (isInternalComponentType(element.type)) {
      // This is temporarily available for custom components that are not string
      // represenations. I.e. ART. Once those are updated to use the string
      // representation, we can drop this code path.
      instance = new element.type(element);
    } else {
      instance = new ReactCompositeComponentWrapper();
    }
  } else if (typeof node === 'string' || typeof node === 'number') {
    instance = ReactNativeComponent.createInstanceForText(node);
  } else {
    invariant(
      false,
      'Encountered invalid React node of type %s',
      typeof node
    );
  }

  if (__DEV__) {
    warning(
      typeof instance.construct === 'function' &&
      typeof instance.mountComponent === 'function' &&
      typeof instance.receiveComponent === 'function' &&
      typeof instance.unmountComponent === 'function',
      'Only React Components can be mounted.'
    );
  }

  // Sets up the instance. This can probably just move into the constructor now.
  instance.construct(node);

  // These two fields are used by the DOM and ART diffing algorithms
  // respectively. Instead of using expandos on components, we should be
  // storing the state needed by the diffing algorithms elsewhere.
  instance._mountIndex = 0;
  instance._mountImage = null;

  if (__DEV__) {
    instance._isOwnerNecessary = false;
    instance._warnedAboutRefsInRender = false;
  }

  // Internal instances should fully constructed at this point, so they should
  // not get any new fields added to them at this point.
  if (__DEV__) {
    if (Object.preventExtensions) {
      Object.preventExtensions(instance);
    }
  }

  return instance;
}

module.exports = instantiateReactComponent;
