/**
@module ember
@submodule ember-htmlbars
*/

/**
  @class Helper
  @namespace Ember.HTMLBars
*/
function Helper(helper) {
  this.helperFunction = helper;

  this.isHelper = true;
  this.isHTMLBars = true;
}

export default Helper;
