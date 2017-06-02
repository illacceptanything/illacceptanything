/**
 * Pretty Keyboard Events v2.2.1
 * http://rightjs.org/plugins/keys
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
(function(RightJS) {
/**
 * The keys plugin initialization
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
RightJS.Keys = {
  version: '2.2.1'
};



/**
 * The Event level keyboard keycodes and stuff
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
RightJS.Event.Keys = {
  BACKSPACE:  8,
  TAB:        9,
  ENTER:     13,
  ESC:       27,
  SPACE:     32,
  PAGEUP:    33,
  PAGEDOWN:  34,
  END:       35,
  HOME:      36,
  LEFT:      37,
  UP:        38,
  RIGHT:     39,
  DOWN:      40,
  INSERT:    45,
  DELETE:    46
};


/**
 * Overloading the Element#on method to handle those fancy names
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
[RightJS.Document, RightJS.Element, RightJS.Window].each('include', {
  on: function() {
    var args = RightJS.$A(arguments), name = args[0];

    if (typeof(name) === 'string') {
      var key = name.split(/[\+\-\_ ]+/);
      key = (key[key.length - 1] || '').toUpperCase();

      if (key in RightJS.Event.Keys || /^[A-Z0-9]$/.test(key)) {
        var meta   = /(^|\+|\-| )(meta|alt)(\+|\-| )/i.test(name),
            ctrl   = /(^|\+|\-| )(ctl|ctrl)(\+|\-| )/i.test(name),
            shift  = /(^|\+|\-| )(shift)(\+|\-| )/i.test(name),
            code   = RightJS.Event.Keys[key] || key.charCodeAt(0),
            orig   = args.slice(1),
            method = orig.shift();

        if (typeof(method) === 'string') {
          method = this[method] || function() {};
        }

        // replacing the arguments
        args = ['keydown', function(event) {
          var raw = event._;
          if (
            (raw.keyCode === code) &&
            (!meta  || raw.metaKey || raw.altKey) &&
            (!ctrl  || raw.ctrlKey) &&
            (!shift || raw.shiftKey)
          ) {
            return method.call(this, [event].concat(orig));
          }
        }];
      }
    }

    return this.$super.apply(this, args);
  }
});

})(RightJS);