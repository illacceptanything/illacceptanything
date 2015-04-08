import create from 'ember-metal/platform/create';
import merge from "ember-metal/merge";
import EmberError from "ember-metal/error";
import { addBeforeObserver } from 'ember-metal/observer';

import hasElement from "ember-views/views/states/has_element";
/**
@module ember
@submodule ember-views
*/

var inDOM = create(hasElement);

merge(inDOM, {
  enter(view) {
    // Register the view for event handling. This hash is used by
    // Ember.EventDispatcher to dispatch incoming events.
    if (!view.isVirtual) {
      view._register();
    }

    Ember.runInDebug(function() {
      addBeforeObserver(view, 'elementId', function() {
        throw new EmberError("Changing a view's elementId after creation is not allowed");
      });
    });
  },

  exit(view) {
    if (!this.isVirtual) {
      view._unregister();
    }
  },

  appendAttr(view, attrNode) {
    var _attrNodes = view._attrNodes;

    if (!_attrNodes.length) { _attrNodes = view._attrNodes = _attrNodes.slice(); }
    _attrNodes.push(attrNode);

    attrNode._parentView = view;
    view.renderer.appendAttrTo(attrNode, view.element, attrNode.attrName);

    view.propertyDidChange('childViews');

    return attrNode;
  }

});

export default inDOM;
