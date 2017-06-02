import {DOM} from 'angular2/src/dom/dom_adapter';
import {List, ListWrapper} from 'angular2/src/facade/collection';
import {isBlank, isPresent} from 'angular2/src/facade/lang';

import * as viewModule from '../view';
import {Content} from './content_tag';

export class DestinationLightDom {}


class _Root {
  node;
  viewContainer;
  content;

  constructor(node, viewContainer, content) {
    this.node = node;
    this.viewContainer = viewContainer;
    this.content = content;
  }
}

// TODO: LightDom should implement DestinationLightDom
// once interfaces are supported
export class LightDom {
  // The light DOM of the element is enclosed inside the lightDomView
  lightDomView:viewModule.View;
  // The shadow DOM
  shadowDomView:viewModule.View;
  // The nodes of the light DOM
  nodes:List;
  roots:List<_Root>;

  constructor(lightDomView:viewModule.View, shadowDomView:viewModule.View, element) {
    this.lightDomView = lightDomView;
    this.shadowDomView = shadowDomView;
    this.nodes = DOM.childNodesAsList(element);

    this.roots = null;
  }

  redistribute() {
    var tags = this.contentTags();
    if (tags.length > 0) {
      redistributeNodes(tags, this.expandedDomNodes());
    }
  }

  contentTags(): List<Content> {
    return this._collectAllContentTags(this.shadowDomView, []);
  }

  // Collects the Content directives from the view and all its child views
  _collectAllContentTags(view: viewModule.View, acc:List<Content>):List<Content> {
    var contentTags = view.contentTags;
    var vcs = view.viewContainers;
    for (var i=0; i<vcs.length; i++) {
      var vc = vcs[i];
      var contentTag = contentTags[i];
      if (isPresent(contentTag)) {
        ListWrapper.push(acc, contentTag);  
      }
      if (isPresent(vc)) {
        ListWrapper.forEach(vc.contentTagContainers(), (view) => {
          this._collectAllContentTags(view, acc);
        });              
      }
    }
    return acc;
  }

  // Collects the nodes of the light DOM by merging:
  // - nodes from enclosed ViewContainers,
  // - nodes from enclosed content tags,
  // - plain DOM nodes
  expandedDomNodes():List {
    var res = [];
    
    var roots = this._roots();
    for (var i = 0; i < roots.length; ++i) {

      var root = roots[i];

      if (isPresent(root.viewContainer)) {
        res = ListWrapper.concat(res, root.viewContainer.nodes());
      } else if (isPresent(root.content)) {
        res = ListWrapper.concat(res, root.content.nodes());
      } else {
        ListWrapper.push(res, root.node);
      }
    }
    return res;
  }

  // Returns a list of Roots for all the nodes of the light DOM.
  // The Root object contains the DOM node and its corresponding injector (could be null).
  _roots() {
    if (isPresent(this.roots)) return this.roots;

    var viewContainers = this.lightDomView.viewContainers;
    var contentTags = this.lightDomView.contentTags;

    this.roots = ListWrapper.map(this.nodes, (n) => {
      var foundVc = null;
      var foundContentTag = null;
      for (var i=0; i<viewContainers.length; i++) {
        var vc = viewContainers[i];
        var contentTag = contentTags[i];
        if (isPresent(vc) && vc.templateElement === n) {
          foundVc = vc;
        }
        if (isPresent(contentTag) && contentTag.contentStartElement === n) {
          foundContentTag = contentTag;
        }
      }
      return new _Root(n, foundVc, foundContentTag);
    });

    return this.roots;
  }
}

// Projects the light DOM into the shadow DOM
function redistributeNodes(contents:List<Content>, nodes:List) {
  for (var i = 0; i < contents.length; ++i) {
    var content = contents[i];
    var select = content.select;
    var matchSelector = (n) => DOM.elementMatches(n, select);

    // Empty selector is identical to <content/>
    if (select.length === 0) {
      content.insert(nodes);
      ListWrapper.clear(nodes);
    } else {
      var matchingNodes = ListWrapper.filter(nodes, matchSelector);
      content.insert(matchingNodes);
      ListWrapper.removeAll(nodes, matchingNodes);
    }
  }
}
