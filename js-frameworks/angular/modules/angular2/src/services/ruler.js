import {Promise, PromiseWrapper} from 'angular2/src/facade/async';
import {DomAdapter} from 'angular2/src/dom/dom_adapter';
import {NgElement} from 'angular2/src/core/dom/element';

export class Rectangle {
  left;
  right;
  top;
  bottom;
  height;
  width;
  constructor(left, top, width, height) {
    this.left = left;
    this.right = left + width;
    this.top = top;
    this.bottom = top + height;
    this.height = height;
    this.width = width;
  }
}

export class Ruler {
  domAdapter: DomAdapter;
  constructor(domAdapter: DomAdapter) {
    this.domAdapter = domAdapter;
  }

  measure(el:NgElement): Promise<Rectangle> {
    var clntRect = this.domAdapter.getBoundingClientRect(el.domElement);

    //even if getBoundingClientRect is synchronous we use async API in preparation for further changes
    return PromiseWrapper.resolve(new Rectangle(clntRect.left, clntRect.top, clntRect.width, clntRect.height));
  }
}
