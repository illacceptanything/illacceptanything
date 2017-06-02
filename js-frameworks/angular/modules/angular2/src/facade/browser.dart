/**
 * Dart version of browser APIs. This library depends on 'dart:html' and
 * therefore can only run in the browser.
 */

import 'dart:js' show context;

export 'dart:html' show
  document,
  location,
  window,
  Element,
  Node;

final _gc = context['gc'];

void gc() {
  if (_gc != null) {
    _gc.apply(const []);
  }
}
