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

var EventPluginHub;
var EventConstants;
var EventPropagators;
var ReactInstanceHandles;
var ResponderEventPlugin;
var SyntheticEvent;
var EventPluginUtils;

var GRANDPARENT_ID = '.0';
var PARENT_ID = '.0.0';
var CHILD_ID = '.0.0.0';
var CHILD_ID2 = '.0.0.1';

var topLevelTypes;
var responderEventTypes;

var touch = function(nodeHandle, i) {
  return {target: nodeHandle, identifier: i};
};

/**
 * @param {NodeHandle} nodeHandle @see NodeHandle. Handle of target.
 * @param {Array<Touch>} touches All active touches.
 * @param {Array<Touch>} changedTouches Only the touches that have changed.
 * @return {TouchEvent} Model of a touch event that is compliant with responder
 * system plugin.
 */
var touchEvent = function(nodeHandle, touches, changedTouches) {
  return {
    target: nodeHandle,
    changedTouches: changedTouches,
    touches: touches
  };
};

var subsequence = function(arr, indices) {
  var ret = [];
  for (var i = 0; i < indices.length; i++) {
    var index = indices[i];
    ret.push(arr[index]);
  }
  return ret;
};

var antiSubsequence = function(arr, indices) {
  var ret = [];
  for (var i = 0; i < arr.length; i++) {
    if (indices.indexOf(i) === -1) {
      ret.push(arr[i]);
    }
  }
  return ret;
};

/**
 * Helper for creating touch test config data.
 * @param allTouchHandles
 */
var _touchConfig =
  function(topType, targetNodeHandle, allTouchHandles, changedIndices) {
  var allTouchObjects = allTouchHandles.map(touch);
  var changedTouchObjects = subsequence(allTouchObjects, changedIndices);
  var activeTouchObjects =
    topType === 'topTouchStart' ? allTouchObjects :
    topType === 'topTouchMove' ? allTouchObjects :
    topType === 'topTouchEnd' ? antiSubsequence(allTouchObjects, changedIndices) :
    topType === 'topTouchCancel' ? antiSubsequence(allTouchObjects, changedIndices) :
    null;

  return {
    nativeEvent: touchEvent(
      targetNodeHandle,
      activeTouchObjects,
      changedTouchObjects
    ),
    topLevelType: topType,
    target: targetNodeHandle,
    targetID: targetNodeHandle,
  };
};

/**
 * Creates test data for touch events using environment agnostic "node
 * handles".
 *
 * @param {NodeHandle} nodeHandle Environment agnostic handle to DOM node.
 * @param {Array<NodeHandle>} allTouchHandles Encoding of all "touches" in the
 * form of a mapping from integer (touch `identifier`) to touch target. This is
 * encoded in array form. Because of this, it is possible for two separate
 * touches (meaning two separate indices) to have the same touch target ID -
 * this corresponds to real world cases where two separate unique touches have
 * the same target. These touches don't just represent all active touches,
 * rather it also includes any touches that are not active, but are in the
 * process of being removed.
 * @param {Array<NodeHandle>} changedIndices Indices of `allTouchHandles` that
 * have changed.
 * @return {object} Config data used by test cases for extracting responder
 * events.
 */
var startConfig = function(nodeHandle, allTouchHandles, changedIndices) {
  return _touchConfig(
    topLevelTypes.topTouchStart,
    nodeHandle,
    allTouchHandles,
    changedIndices
  );
};

/**
 * @see `startConfig`
 */
var moveConfig = function(nodeHandle, allTouchHandles, changedIndices) {
  return _touchConfig(
    topLevelTypes.topTouchMove,
    nodeHandle,
    allTouchHandles,
    changedIndices
  );
};

/**
 * @see `startConfig`
 */
var endConfig = function(nodeHandle, allTouchHandles, changedIndices) {
  return _touchConfig(
    topLevelTypes.topTouchEnd,
    nodeHandle,
    allTouchHandles,
    changedIndices
  );
};

/**
 * Test config for events that aren't negotiation related, but rather result of
 * a negotiation.
 *
 * Returns object of the form:
 *
 *     {
 *       responderReject: {
 *         // Whatever "readableIDToID" was passed in.
 *         grandParent: {order: NA, assertEvent: null, returnVal: blah},
 *         ...
 *         child: {order: NA, assertEvent: null, returnVal: blah},
 *       }
 *       responderGrant: {
 *         grandParent: {order: NA, assertEvent: null, returnVal: blah},
 *         ...
 *         child: {order: NA, assertEvent: null, returnVal: blah}
 *       }
 *       ...
 *     }
 *
 * After this is created, a test case would configure specific event orderings
 * and optional assertions. Anything left with an `order` of `NA` will be
 * required to never be invoked (the test runner will make sure it throws if
 * ever invoked).
 *
 */
var NA = -1;
var oneEventLoopTestConfig = function(readableIDToID) {
  var ret = {
    // Negotiation
    scrollShouldSetResponder: {bubbled:  {}, captured: {}},
    startShouldSetResponder: {bubbled:  {}, captured: {}},
    moveShouldSetResponder: {bubbled:  {}, captured: {}},
    responderTerminationRequest: {},

    // Non-negotiation
    responderReject:      {}, // These do not bubble capture.
    responderGrant:     {},
    responderStart:     {},
    responderMove:      {},
    responderTerminate: {},
    responderEnd:       {},
    responderRelease:   {},
  };
  for (var eventName in ret) {
    for (var readableNodeName in readableIDToID) {
      if (ret[eventName].bubbled) {
        // Two phase
        ret[eventName].bubbled[readableNodeName] =
          {order: NA, assertEvent: null, returnVal: undefined};
        ret[eventName].captured[readableNodeName] =
          {order: NA, assertEvent: null, returnVal: undefined};
      } else {
        ret[eventName][readableNodeName] =
          {order: NA, assertEvent: null, returnVal: undefined};
      }
    }
  }
  return ret;
};

/**
 * @param {object} sequence See `oneEventLoopTestConfig`.
 */
var registerTestHandlers = function(eventTestConfig, readableIDToID) {
  var runs = {dispatchCount: 0};
  var neverFire = function (readableID, registrationName) {
    runs.dispatchCount++;
    expect('').toBe(
      'Event type: ' + registrationName +
      '\nShould never occur on:' + readableID +
      '\nFor event test config:\n' + JSON.stringify(eventTestConfig) + '\n'
    );
  };
  var registerOneEventType = function(registrationName, eventTypeTestConfig) {
    for (var readableID in eventTypeTestConfig) {
      var nodeConfig = eventTypeTestConfig[readableID];
      var id = readableIDToID[readableID];
      var handler = nodeConfig.order === NA ? neverFire.bind(null, readableID, registrationName) :
        function(readableID, registrationName, nodeConfig, e) {
          expect(
            readableID + '->' + registrationName + ' index:' + runs.dispatchCount++
          ).toBe(
            readableID + '->' + registrationName + ' index:' + nodeConfig.order
          );
          nodeConfig.assertEvent && nodeConfig.assertEvent(e);
          return nodeConfig.returnVal;
        }.bind(null, readableID, registrationName, nodeConfig);
      EventPluginHub.putListener(id, registrationName, handler);
    }
  };
  for (var eventName in eventTestConfig) {
    var oneEventTypeTestConfig = eventTestConfig[eventName];
    var hasTwoPhase = !!oneEventTypeTestConfig.bubbled;
    if (hasTwoPhase) {
      registerOneEventType(
        ResponderEventPlugin.eventTypes[eventName].phasedRegistrationNames.bubbled,
        oneEventTypeTestConfig.bubbled
      );
      registerOneEventType(
        ResponderEventPlugin.eventTypes[eventName].phasedRegistrationNames.captured,
        oneEventTypeTestConfig.captured
      );
    } else {
      registerOneEventType(
        ResponderEventPlugin.eventTypes[eventName].registrationName,
        oneEventTypeTestConfig
      );
    }
  }
  return runs;
};




var run = function(config, hierarchyConfig, nativeEventConfig) {
  var max = NA;
  var searchForMax = function(nodeConfig) {
    for (var readableID in nodeConfig) {
      var order = nodeConfig[readableID].order;
      max = order > max ? order : max;
    }
  };
  for (var eventName in config) {
    var eventConfig = config[eventName];
    if (eventConfig.bubbled) {
      searchForMax(eventConfig.bubbled);
      searchForMax(eventConfig.captured);
    } else {
      searchForMax(eventConfig);
    }
  }

  // Register the handlers
  var runData = registerTestHandlers(config, hierarchyConfig);

  // Trigger the event
  var extractedEvents = ResponderEventPlugin.extractEvents(
    nativeEventConfig.topLevelType,
    nativeEventConfig.target,
    nativeEventConfig.targetID,
    nativeEventConfig.nativeEvent
  );

  // At this point the negotiation events have been dispatched as part of the
  // extraction process, but not the side effectful events. Below, we dispatch
  // side effectful events.
  EventPluginHub.enqueueEvents(extractedEvents);
  EventPluginHub.processEventQueue();

  // Ensure that every event that declared an `order`, was actually dispatched.
  expect(
    'number of events dispatched:' + runData.dispatchCount
  ).toBe(
    'number of events dispatched:' + (max + 1)
  ); // +1 for extra ++
};

var three = {
  grandParent: GRANDPARENT_ID,
  parent: PARENT_ID,
  child: CHILD_ID,
};

var siblings = {
  parent: PARENT_ID,
  childOne: CHILD_ID,
  childTwo: CHILD_ID2,
};

describe('ResponderEventPlugin', function() {
  beforeEach(function() {
    require('mock-modules').dumpCache();

    EventConstants = require('EventConstants');
    EventPluginHub = require('EventPluginHub');
    EventPluginUtils = require('EventPluginUtils');
    EventPropagators = require('EventPropagators');
    ReactInstanceHandles = require('ReactInstanceHandles');
    ResponderEventPlugin = require('ResponderEventPlugin');
    SyntheticEvent = require('SyntheticEvent');

    EventPluginHub.injection.injectInstanceHandle(ReactInstanceHandles);

    // Only needed because SyntheticEvent supports the `currentTarget`
    // property.
    EventPluginUtils.injection.injectMount({
      getNode: function(id) {
        return id;
      },
      getID: function(nodeHandle) {
        return nodeHandle;
      }
    });

    topLevelTypes = EventConstants.topLevelTypes;
    responderEventTypes = ResponderEventPlugin.eventTypes;
  });

  it('should do nothing when no one wants to respond', function() {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent =      {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child =       {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child =        {order: 3, returnVal: false};
    config.startShouldSetResponder.bubbled.parent =       {order: 4, returnVal: false};
    config.startShouldSetResponder.bubbled.grandParent =  {order: 5, returnVal: false};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);

    // Now no handlers should be called on `touchEnd`.
    config = oneEventLoopTestConfig(three);
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });


  /**
   * Simple Start Granting
   * --------------------
   */


  it('should grant responder grandParent while capturing', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: true};
    config.responderGrant.grandParent = {order: 1};
    config.responderStart.grandParent = {order: 2};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(GRANDPARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.grandParent = {order: 0};
    config.responderRelease.grandParent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder parent while capturing', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: true};
    config.responderGrant.parent = {order: 2};
    config.responderStart.parent = {order: 3};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(PARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.parent = {order: 0};
    config.responderRelease.parent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder child while capturing', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: true};
    config.responderGrant.child = {order: 3};
    config.responderStart.child = {order: 4};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.child = {order: 0};
    config.responderRelease.child = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder child while bubbling', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child = {order: 3, returnVal: true};
    config.responderGrant.child = {order: 4};
    config.responderStart.child = {order: 5};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.child = {order: 0};
    config.responderRelease.child = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder parent while bubbling', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child = {order: 3, returnVal: false};
    config.startShouldSetResponder.bubbled.parent = {order: 4, returnVal: true};
    config.responderGrant.parent = {order: 5};
    config.responderStart.parent = {order: 6};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(PARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.parent = {order: 0};
    config.responderRelease.parent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder grandParent while bubbling', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child = {order: 3, returnVal: false};
    config.startShouldSetResponder.bubbled.parent = {order: 4, returnVal: false};
    config.startShouldSetResponder.bubbled.grandParent = {order: 5, returnVal: true};
    config.responderGrant.grandParent = {order: 6};
    config.responderStart.grandParent = {order: 7};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(GRANDPARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.grandParent = {order: 0};
    config.responderRelease.grandParent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });



  /**
   * Simple Move Granting
   * --------------------
   */

  it('should grant responder grandParent while capturing move', () => {
    var config = oneEventLoopTestConfig(three);

    config.startShouldSetResponder.captured.grandParent = {order: 0};
    config.startShouldSetResponder.captured.parent = {order: 1};
    config.startShouldSetResponder.captured.child = {order: 2};
    config.startShouldSetResponder.bubbled.child = {order: 3};
    config.startShouldSetResponder.bubbled.parent = {order: 4};
    config.startShouldSetResponder.bubbled.grandParent = {order: 5};
    run(config, three, startConfig(three.child, [three.child], [0]));

    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: true};
    config.responderGrant.grandParent = {order: 1};
    config.responderMove.grandParent = {order: 2};
    run(config, three, moveConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(GRANDPARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.grandParent = {order: 0};
    config.responderRelease.grandParent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder parent while capturing move', () => {
    var config = oneEventLoopTestConfig(three);

    config.startShouldSetResponder.captured.grandParent = {order: 0};
    config.startShouldSetResponder.captured.parent = {order: 1};
    config.startShouldSetResponder.captured.child = {order: 2};
    config.startShouldSetResponder.bubbled.child = {order: 3};
    config.startShouldSetResponder.bubbled.parent = {order: 4};
    config.startShouldSetResponder.bubbled.grandParent = {order: 5};
    run(config, three, startConfig(three.child, [three.child], [0]));

    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.captured.parent = {order: 1, returnVal: true};
    config.responderGrant.parent = {order: 2};
    config.responderMove.parent = {order: 3};
    run(config, three, moveConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(PARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.parent = {order: 0};
    config.responderRelease.parent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder child while capturing move', () => {
    var config = oneEventLoopTestConfig(three);

    config.startShouldSetResponder.captured.grandParent = {order: 0};
    config.startShouldSetResponder.captured.parent = {order: 1};
    config.startShouldSetResponder.captured.child = {order: 2};
    config.startShouldSetResponder.bubbled.child = {order: 3};
    config.startShouldSetResponder.bubbled.parent = {order: 4};
    config.startShouldSetResponder.bubbled.grandParent = {order: 5};
    run(config, three, startConfig(three.child, [three.child], [0]));

    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.moveShouldSetResponder.captured.child = {order: 2, returnVal: true};
    config.responderGrant.child = {order: 3};
    config.responderMove.child = {order: 4};
    run(config, three, moveConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.child = {order: 0};
    config.responderRelease.child = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder child while bubbling move', () => {
    var config = oneEventLoopTestConfig(three);

    config.startShouldSetResponder.captured.grandParent = {order: 0};
    config.startShouldSetResponder.captured.parent = {order: 1};
    config.startShouldSetResponder.captured.child = {order: 2};
    config.startShouldSetResponder.bubbled.child = {order: 3};
    config.startShouldSetResponder.bubbled.parent = {order: 4};
    config.startShouldSetResponder.bubbled.grandParent = {order: 5};
    run(config, three, startConfig(three.child, [three.child], [0]));

    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.moveShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.moveShouldSetResponder.bubbled.child = {order: 3, returnVal: true};
    config.responderGrant.child = {order: 4};
    config.responderMove.child = {order: 5};
    run(config, three, moveConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.child = {order: 0};
    config.responderRelease.child = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder parent while bubbling move', () => {
    var config = oneEventLoopTestConfig(three);

    config.startShouldSetResponder.captured.grandParent = {order: 0};
    config.startShouldSetResponder.captured.parent = {order: 1};
    config.startShouldSetResponder.captured.child = {order: 2};
    config.startShouldSetResponder.bubbled.child = {order: 3};
    config.startShouldSetResponder.bubbled.parent = {order: 4};
    config.startShouldSetResponder.bubbled.grandParent = {order: 5};
    run(config, three, startConfig(three.child, [three.child], [0]));

    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.moveShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.moveShouldSetResponder.bubbled.child = {order: 3, returnVal: false};
    config.moveShouldSetResponder.bubbled.parent = {order: 4, returnVal: true};
    config.responderGrant.parent = {order: 5};
    config.responderMove.parent = {order: 6};
    run(config, three, moveConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(PARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.parent = {order: 0};
    config.responderRelease.parent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should grant responder grandParent while bubbling move', () => {
    var config = oneEventLoopTestConfig(three);

    config.startShouldSetResponder.captured.grandParent = {order: 0};
    config.startShouldSetResponder.captured.parent = {order: 1};
    config.startShouldSetResponder.captured.child = {order: 2};
    config.startShouldSetResponder.bubbled.child = {order: 3};
    config.startShouldSetResponder.bubbled.parent = {order: 4};
    config.startShouldSetResponder.bubbled.grandParent = {order: 5};
    run(config, three, startConfig(three.child, [three.child], [0]));

    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.moveShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.moveShouldSetResponder.bubbled.child = {order: 3, returnVal: false};
    config.moveShouldSetResponder.bubbled.parent = {order: 4, returnVal: false};
    config.moveShouldSetResponder.bubbled.grandParent = {order: 5, returnVal: true};
    config.responderGrant.grandParent = {order: 6};
    config.responderMove.grandParent = {order: 7};
    run(config, three, moveConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(GRANDPARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.grandParent = {order: 0};
    config.responderRelease.grandParent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });


  /**
   * Common ancestor tests
   * ---------------------
   */

  it('should bubble negotiation to first common ancestor of responder', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: true};
    config.responderGrant.parent = {order: 2};
    config.responderStart.parent = {order: 3};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(PARENT_ID);

    // While `PARENT_ID` is still responder, we create new handlers that verify
    // the ordering of propagation, restarting the count at `0`.
    config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};

    config.startShouldSetResponder.bubbled.grandParent = {order: 1, returnVal: false};
    config.responderStart.parent = {order: 2};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(PARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.parent = {order: 0};
    config.responderRelease.parent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  it('should bubble negotiation to first common ancestor of responder then transfer', () => {
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: true};
    config.responderGrant.parent = {order: 2};
    config.responderStart.parent = {order: 3};
    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(PARENT_ID);

    config = oneEventLoopTestConfig(three);

    // Parent is responder, and responder is transfered by a second touch start
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: true};
    config.responderTerminationRequest.parent = {order: 1, returnVal: true};
    config.responderGrant.grandParent = {order: 2};
    config.responderTerminate.parent = {order: 3};
    config.responderStart.grandParent = {order: 4};
    run(config, three, startConfig(three.child, [three.child, three.child], [1]));
    expect(ResponderEventPlugin.getResponderID()).toBe(GRANDPARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.grandParent = {order: 0};
                                      // one remains\ /one ended \
    run(config, three, endConfig(three.child, [three.child, three.child], [1]));
    expect(ResponderEventPlugin.getResponderID()).toBe(GRANDPARENT_ID);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.grandParent = {order: 0};
    config.responderRelease.grandParent = {order: 1};
    run(config, three, endConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });

  /**
   * If nothing is responder, then the negotiation should propagate directly to
   * the deepest target in the second touch.
   */
  it('should negotiate with deepest target on second touch if nothing is responder', () => {
    // Initially nothing wants to become the responder
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.bubbled.parent = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.grandParent = {order: 3, returnVal: false};

    run(config, three, startConfig(three.parent, [three.parent], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);

    config = oneEventLoopTestConfig(three);

    // Now child wants to become responder. Negotiation should bubble as deep
    // as the target is because we don't find first common ancestor (with
    // current responder) because there is no current responder.
    // (Even if this is the second active touch).
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child = {order: 3, returnVal: true};
    config.responderGrant.child = {order: 4};
    config.responderStart.child = {order: 5};
    //                                     /  Two active touches  \  /one of them new\
    run(config, three, startConfig(three.child, [three.parent, three.child], [1]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);


    // Now we remove the original first touch, keeping the second touch that
    // started within the current responder (child). Nothing changes because
    // there's still touches that started inside of the current responder.
    config = oneEventLoopTestConfig(three);
    config.responderEnd.child = {order: 0};
    //                                      / one ended\  /one remains \
    run(config, three, endConfig(three.child, [three.parent, three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    // Okay, now let's add back that first touch (nothing should change) and
    // then we'll try peeling back the touches in the opposite order to make
    // sure that first removing the second touch instantly causes responder to
    // be released.
    config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.bubbled.parent = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.grandParent = {order: 3, returnVal: false};
    // Interesting: child still gets moves even though touch target is parent!
    // Current responder gets a `responderStart` for any touch while responder.
    config.responderStart.child = {order: 4};
    //                                           /  Two active touches  \  /one of them new\
    run(config, three, startConfig(three.parent, [three.child, three.parent], [1]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);


    // Now, move that new touch that had no effect, and did not start within
    // the current responder.
    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.moveShouldSetResponder.bubbled.parent = {order: 2, returnVal: false};
    config.moveShouldSetResponder.bubbled.grandParent = {order: 3, returnVal: false};
    // Interesting: child still gets moves even though touch target is parent!
    // Current responder gets a `responderMove` for any touch while responder.
    config.responderMove.child = {order: 4};
    //                                     /  Two active touches  \  /one of them moved\
    run(config, three, moveConfig(three.parent, [three.child, three.parent], [1]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);


    config = oneEventLoopTestConfig(three);
    config.responderEnd.child = {order: 0};
    config.responderRelease.child = {order: 1};
    //                                        /child end \ /parent remain\
    run(config, three, endConfig(three.child, [three.child, three.parent], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });


  /**
   * If nothing is responder, then the negotiation should propagate directly to
   * the deepest target in the second touch.
   */
  it('should negotiate until first common ancestor when there are siblings', () => {
    // Initially nothing wants to become the responder
    var config = oneEventLoopTestConfig(siblings);
    config.startShouldSetResponder.captured.parent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.childOne = {order: 1, returnVal: false};
    config.startShouldSetResponder.bubbled.childOne = {order: 2, returnVal: true};
    config.responderGrant.childOne = {order: 3};
    config.responderStart.childOne = {order: 4};

    run(config, siblings, startConfig(siblings.childOne, [siblings.childOne], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(siblings.childOne);

    // If the touch target is the sibling item, the negotiation should only
    // propagate to first common ancestor of current responder and sibling (so
    // the parent).
    config = oneEventLoopTestConfig(siblings);
    config.startShouldSetResponder.captured.parent = {order: 0, returnVal: false};
    config.startShouldSetResponder.bubbled.parent = {order: 1, returnVal: false};
    config.responderStart.childOne = {order: 2};

    var touchConfig =
      startConfig(siblings.childTwo, [siblings.childOne, siblings.childTwo], [1]);
    run(config, siblings, touchConfig);
    expect(ResponderEventPlugin.getResponderID()).toBe(siblings.childOne);


    // move childOne
    config = oneEventLoopTestConfig(siblings);
    config.moveShouldSetResponder.captured.parent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.bubbled.parent = {order: 1, returnVal: false};
    config.responderMove.childOne = {order: 2};
    run(config, siblings, moveConfig(siblings.childOne, [siblings.childOne, siblings.childTwo], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(siblings.childOne);

    // move childTwo: Only negotiates to `parent`.
    config = oneEventLoopTestConfig(siblings);
    config.moveShouldSetResponder.captured.parent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.bubbled.parent = {order: 1, returnVal: false};
    config.responderMove.childOne = {order: 2};
    run(config, siblings, moveConfig(siblings.childTwo, [siblings.childOne, siblings.childTwo], [1]));
    expect(ResponderEventPlugin.getResponderID()).toBe(siblings.childOne);

  });


  it('should notify of being rejected. responderStart/Move happens on current responder', () => {
    // Initially nothing wants to become the responder
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child = {order: 3, returnVal: true};
    config.responderGrant.child = {order: 4};
    config.responderStart.child = {order: 5};

    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    // Suppose parent wants to become responder on move, and is rejected
    config = oneEventLoopTestConfig(three);
    config.moveShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.moveShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.moveShouldSetResponder.bubbled.parent = {order: 2, returnVal: true};
    config.responderTerminationRequest.child = {order: 3, returnVal: false};
    config.responderReject.parent = {order: 4};
    // The start/move should occur on the original responder if new one is rejected
    config.responderMove.child = {order: 5};

    var touchConfig =
      moveConfig(three.child, [three.child], [0]);
    run(config, three, touchConfig);
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.bubbled.parent = {order: 2, returnVal: true};
    config.responderTerminationRequest.child = {order: 3, returnVal: false};
    config.responderReject.parent = {order: 4};
    // The start/move should occur on the original responder if new one is rejected
    config.responderStart.child = {order: 5};

    touchConfig =
      startConfig(three.child, [three.child, three.child], [1]);
    run(config, three, touchConfig);
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

  });


  it('should negotiate scroll', () => {
    // Initially nothing wants to become the responder
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child = {order: 3, returnVal: true};
    config.responderGrant.child = {order: 4};
    config.responderStart.child = {order: 5};

    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    // If the touch target is the sibling item, the negotiation should only
    // propagate to first common ancestor of current responder and sibling (so
    // the parent).
    config = oneEventLoopTestConfig(three);
    config.scrollShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.scrollShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.scrollShouldSetResponder.bubbled.parent = {order: 2, returnVal: true};
    config.responderTerminationRequest.child = {order: 3, returnVal: false};
    config.responderReject.parent = {order: 4};

    run(config, three, {
      topLevelType: topLevelTypes.topScroll,
      target: three.parent,
      targetID: three.parent,
      nativeEvent: {}
    });
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);


    // Now lets let the scroll take control this time.
    config = oneEventLoopTestConfig(three);
    config.scrollShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.scrollShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.scrollShouldSetResponder.bubbled.parent = {order: 2, returnVal: true};
    config.responderTerminationRequest.child = {order: 3, returnVal: true};
    config.responderGrant.parent = {order: 4};
    config.responderTerminate.child = {order: 5};

    run(config, three, {
      topLevelType: topLevelTypes.topScroll,
      target: three.parent,
      targetID: three.parent,
      nativeEvent: {}
    });
    expect(ResponderEventPlugin.getResponderID()).toBe(three.parent);


  });

  it('should cancel correctly', () => {
    // Initially our child becomes responder
    var config = oneEventLoopTestConfig(three);
    config.startShouldSetResponder.captured.grandParent = {order: 0, returnVal: false};
    config.startShouldSetResponder.captured.parent = {order: 1, returnVal: false};
    config.startShouldSetResponder.captured.child = {order: 2, returnVal: false};
    config.startShouldSetResponder.bubbled.child = {order: 3, returnVal: true};
    config.responderGrant.child = {order: 4};
    config.responderStart.child = {order: 5};

    run(config, three, startConfig(three.child, [three.child], [0]));
    expect(ResponderEventPlugin.getResponderID()).toBe(three.child);

    config = oneEventLoopTestConfig(three);
    config.responderEnd.child = {order: 0};
    config.responderTerminate.child = {order: 1};

    var nativeEvent = _touchConfig(
      topLevelTypes.topTouchCancel,
      three.child,
      [three.child],
      [0]
    );
    run(config, three, nativeEvent);
    expect(ResponderEventPlugin.getResponderID()).toBe(null);
  });
});
