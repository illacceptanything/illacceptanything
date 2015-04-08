import {DOM} from 'angular2/src/dom/dom_adapter';
import {normalizeBlank} from 'angular2/src/facade/lang';

/**
 * @publicModule angular2/angular2
 */
export class NgElement {
  domElement;
  constructor(domElement) {
    this.domElement = domElement;
  }

  getAttribute(name:string) {
    return normalizeBlank(DOM.getAttribute(this.domElement, name));
  }
}