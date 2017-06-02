/**
 * RightJS-UI Colorpicker v2.2.3
 * http://rightjs.org/ui/colorpicker
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var Colorpicker = RightJS.Colorpicker = (function(document, Math, parseInt, RightJS) {
/**
 * This module defines the basic widgets constructor
 * it creates an abstract proxy with the common functionality
 * which then we reuse and override in the actual widgets
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */

/**
 * The widget units constructor
 *
 * @param String tag-name or Object methods
 * @param Object methods
 * @return Widget wrapper
 */
function Widget(tag_name, methods) {
  if (!methods) {
    methods = tag_name;
    tag_name = 'DIV';
  }

  /**
   * An Abstract Widget Unit
   *
   * Copyright (C) 2010 Nikolay Nemshilov
   */
  var AbstractWidget = new RightJS.Class(RightJS.Element.Wrappers[tag_name] || RightJS.Element, {
    /**
     * The common constructor
     *
     * @param Object options
     * @param String optional tag name
     * @return void
     */
    initialize: function(key, options) {
      this.key = key;
      var args = [{'class': 'rui-' + key}];

      // those two have different constructors
      if (!(this instanceof RightJS.Input || this instanceof RightJS.Form)) {
        args.unshift(tag_name);
      }
      this.$super.apply(this, args);

      if (RightJS.isString(options)) {
        options = RightJS.$(options);
      }

      // if the options is another element then
      // try to dynamically rewrap it with our widget
      if (options instanceof RightJS.Element) {
        this._ = options._;
        if ('$listeners' in options) {
          options.$listeners = options.$listeners;
        }
        options = {};
      }
      this.setOptions(options, this);

      return (RightJS.Wrapper.Cache[RightJS.$uid(this._)] = this);
    },

  // protected

    /**
     * Catches the options
     *
     * @param Object user-options
     * @param Element element with contextual options
     * @return void
     */
    setOptions: function(options, element) {
      if (element) {
        options = RightJS.Object.merge(options, new Function("return "+(
          element.get('data-'+ this.key) || '{}'
        ))());
      }

      if (options) {
        RightJS.Options.setOptions.call(this, RightJS.Object.merge(this.options, options));
      }

      return this;
    }
  });

  /**
   * Creating the actual widget class
   *
   */
  var Klass = new RightJS.Class(AbstractWidget, methods);

  // creating the widget related shortcuts
  RightJS.Observer.createShortcuts(Klass.prototype, Klass.EVENTS || RightJS([]));

  return Klass;
}


/**
 * A shared button unit.
 * NOTE: we use the DIV units instead of INPUTS
 *       so those buttons didn't interfere with
 *       the user's tab-index on his page
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Button = new RightJS.Class(RightJS.Element, {
  /**
   * Constructor
   *
   * @param String caption
   * @param Object options
   * @return void
   */
  initialize: function(caption, options) {
    this.$super('div', options);
    this._.innerHTML = caption;
    this.addClass('rui-button');
    this.on('selectstart', 'stopEvent');
  },

  /**
   * Disasbles the button
   *
   * @return Button this
   */
  disable: function() {
    return this.addClass('rui-button-disabled');
  },

  /**
   * Enables the button
   *
   * @return Button this
   */
  enable: function() {
    return this.removeClass('rui-button-disabled');
  },

  /**
   * Checks if the button is disabled
   *
   * @return Button this
   */
  disabled: function() {
    return this.hasClass('rui-button-disabled');
  },

  /**
   * Checks if the button is enabled
   *
   * @return Button this
   */
  enabled: function() {
    return !this.disabled();
  },

  /**
   * Overloading the method, so it fired the events
   * only when the button is active
   *
   * @return Button this
   */
  fire: function() {
    if (this.enabled()) {
      this.$super.apply(this, arguments);
    }
    return this;
  }
});


/**
 * A shared module that toggles a widget visibility status
 * in a uniformed way according to the options settings
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var Toggler = {
  /**
   * Shows the element
   *
   * @param String fx-name
   * @param Object fx-options
   * @return Element this
   */
  show: function(fx_name, fx_options) {
    this.constructor.current = this;
    return Toggler_toggle(this, 'show', fx_name, fx_options);
  },

  /**
   * Hides the element
   *
   * @param String fx-name
   * @param Object fx-options
   * @return Element this
   */
  hide: function(fx_name, fx_options) {
    this.constructor.current = null;
    return Toggler_toggle(this, 'hide', fx_name, fx_options);
  },

  /**
   * Toggles the widget at the given element
   *
   * @param Element the related element
   * @param String position right/bottom (bottom is the default)
   * @param Boolean marker if the element should be resized to the element size
   * @return Widget this
   */
  showAt: function(element, where, resize) {
    this.hide(null).shownAt = element = RightJS.$(element);

    // moves this element at the given one
    Toggler_re_position.call(this, element, where, resize);

    return this.show();
  },

  /**
   * Toggles the widget at the given element
   *
   * @param Element the related element
   * @param String position top/left/right/bottom (bottom is the default)
   * @param Boolean marker if the element should be resized to the element size
   * @return Widget this
   */
  toggleAt: function(element, where, resize) {
    return this.hidden() ? this.showAt(element, where, resize) : this.hide();
  }
};


/**
 * toggles the element's state according to the current settings
 *
 * @param event String 'show' or 'hide' the event name
 * @param String an optional fx-name
 * @param Object an optional fx-options hash
 * @return void
 */
function Toggler_toggle(element, event, fx_name, fx_options) {
  if ((event === 'hide' && element.visible()) || (event === 'show' && element.hidden())) {
    if (RightJS.Fx) {
      element.___fx = true;

      if (fx_name === undefined) {
        fx_name = element.options.fxName;

        if (fx_options === undefined) {
          fx_options = {
            duration: element.options.fxDuration,
            onFinish: function() {
              element.___fx = false;
              element.fire(event);
            }
          };

          // hide on double time
          if (event === 'hide') {
            fx_options.duration = (RightJS.Fx.Durations[fx_options.duration] ||
              fx_options.duration) / 2;
          }
        }
      }
    } else {
      // manually trigger the event if no fx were specified
      element.___fx = false;
      if (!fx_name) { element.fire(event); }
    }

    return element.$super(fx_name, fx_options);
  } else {
    return element;
  }
}

/**
 * Relatively positions the current element
 * against the specified one
 *
 * NOTE: this function is called in a context
 *       of another element
 *
 * @param Element the target element
 * @param String position 'right' or 'bottom'
 * @param Boolean if `true` then the element size will be adjusted
 * @return void
 */
function Toggler_re_position(element, where, resize) {
  var anchor = this.reAnchor || (this.reAnchor =
        new RightJS.Element('div', {'class': 'rui-re-anchor'}))
        .insert(this),

      pos  = anchor.insertTo(element, 'after').position(),
      dims = element.dimensions(), target = this,

      border_top    = parseInt(element.getStyle('borderTopWidth')),
      border_left   = parseInt(element.getStyle('borderLeftWidth')),
      border_right  = parseInt(element.getStyle('borderRightWidth')),
      border_bottom = parseInt(element.getStyle('borderBottomWidth')),

      top    = dims.top    - pos.y       + border_top,
      left   = dims.left   - pos.x       + border_left,
      width  = dims.width  - border_left - border_right,
      height = dims.height - border_top  - border_bottom;

  // making the element to appear so we could read it's sizes
  target.setStyle('visibility:hidden').show(null);

  if (where === 'right') {
    left += width - target.size().x;
  } else {  // bottom
    top  += height;
  }

  target.moveTo(left, top);

  if (resize) {
    if (where === 'left' || where === 'right') {
      target.setHeight(height);
    } else {
      target.setWidth(width);
    }
  }

  // rolling the invisibility back
  target.setStyle('visibility:visible').hide(null);
}

/**
 * A shared module that provides for the widgets an ability
 * to be assigned to an input element and work in pair with it
 *
 * NOTE: this module works in pair with the 'RePosition' module!
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var Assignable = {
  /**
   * Assigns the widget to serve the given input element
   *
   * Basically it puts the references of the current widget
   * to the input and trigger objects so they could be recognized
   * later, and it also synchronizes the changes between the input
   * element and the widget
   *
   * @param {Element} input field
   * @param {Element} optional trigger
   * @return Widget this
   */
  assignTo: function(input, trigger) {
    input   = RightJS.$(input);
    trigger = RightJS.$(trigger);

    if (trigger) {
      trigger[this.key] = this;
      trigger.assignedInput = input;
    } else {
      input[this.key] = this;
    }

    var on_change = RightJS(function() {
      if (this.visible() && (!this.showAt || this.shownAt === input)) {
        this.setValue(input.value());
      }
    }).bind(this);

    input.on({
      keyup:  on_change,
      change: on_change
    });

    this.onChange(function() {
      if (!this.showAt || this.shownAt === input) {
        input.setValue(this.getValue());
      }
    });

    return this;
  }
};


/**
 * The initialization files list
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */

var R = RightJS,
    $ = RightJS.$,
    $w = RightJS.$w,
    $$ = RightJS.$$,
    $E = RightJS.$E,
    $A = RightJS.$A,
    isArray = RightJS.isArray,
    Class   = RightJS.Class,
    Element = RightJS.Element,
    Input   = RightJS.Input;








/**
 * The basic file for Colorpicker
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var Colorpicker = new Widget({
  include: [Toggler, Assignable],

  extend: {
    version: '2.2.3',

    EVENTS: $w('change show hide done'),

    Options: {
      format:       'hex',   // hex or rgb

      update:       null,    // an element to update with the color text
      updateBg:     null,    // an element to update it's background color
      updateBorder: null,    // an element to update it's border color
      updateColor:  null,    // an element to update it's text color
      trigger:      null,    // a trigger element for the popup

      fxName:       'fade',  // popup displaying fx
      fxDuration:   'short',

      cssRule:      '*[data-colorpicker]'
    },

    i18n: {
      Done: 'Done'
    },

    // hides all the popup colorpickers on the page
    hideAll: function() {
      $$('div.rui-colorpicker').each(function(picker) {
        if (picker instanceof Colorpicker && !picker.inlined()) {
          picker.hide();
        }
      });
    }
  },

  /**
   * basic constructor
   *
   * @param Object options
   */
  initialize: function(options) {
    this
      .$super('colorpicker', options)
      .addClass('rui-panel')
      .insert([
        this.field    = new Field(),
        this.colors   = new Colors(),
        this.controls = new Controls()
      ])
      .on({
        mousedown:  this.startTrack,
        touchstart: this.startTrack,

        keyup: this.recalc,
        blur:  this.update,
        focus: this.cancelTimer,

        done:  this.done
      });

    // hooking up the elements to update
    if (this.options.update)   { this.assignTo(this.options.update, this.options.trigger); }
    if (this.options.updateBg) { this.updateBg(this.options.updateBg); }
    if (this.options.updateBorder) { this.updateBorder(this.options.updateBorder); }
    if (this.options.updateColor) { this.updateColor(this.options.updateColor); }

    // setting up the initial values
    this.tint   = R([1, 0, 0]);
    this.satur  = 0;
    this.bright = 1;
    this.color  = R([255, 255, 255]);

    this.recalc().update();
  },

  /**
   * Sets the color of the widget
   *
   * @param mixed value, Array or HEX or RGB value
   * @return Colorpicker this
   */
  setValue: function(value) {
    var color = isArray(value) ? value : this.toColor(value);
    if (color && color.length === 3) {

      // normalizing the data
      color = color.map(function(value) {
        return this.bound(parseInt(''+value), 0, 255);
      }, this);

      this.color = color;
      this.color2tint().update();

      // reupdating the popup-state a bit later when we have the sizes
      if (!this.colors.size().y) {
        this.update.bind(this).delay(20);
      }
    }
    return this;
  },

  /**
   * Returns the value of the widget
   * formatted according to the options
   *
   * @param Boolean if you need a clean RGB values array
   * @return mixed value
   */
  getValue: function(array) {
    return array ? this.color : this[this.options.format === 'rgb' ? 'toRgb' : 'toHex']();
  },

  /**
   * Assigns the colorpicer to automatically update
   * given element's background on changes
   *
   * @param mixed element reference
   * @return Colorpicker this
   */
  updateBg: function(element_ref) {
    var element = $(element_ref);
    if (element) {
      this.onChange(R(function(color) {
        element._.style.backgroundColor = this.toRgb();
      }).bind(this));
    }
    return this;
  },

    /**
   * Assigns the colorpicer to automatically update
   * given element's text color on changes
   *
   * @param mixed element reference
   * @return Colorpicker this
   */
    updateColor: function(element_ref) {
    var element = $(element_ref);
    if (element) {
      this.onChange(R(function(color) {
        element._.style.color = this.toRgb();
      }).bind(this));
    }
    return this;
  },

    /**
   * Assigns the colorpicer to automatically update
   * given element's border color on changes
   *
   * @param mixed element reference
   * @return Colorpicker this
   */
  updateBorder: function(element_ref) {
    var element = $(element_ref);
    if (element) {
      this.onChange(R(function(color) {
        element._.style.borderColor = this.toRgb();
      }).bind(this));
    }
    return this;
  },


  /**
   * Inlines the widget into the given element
   *
   * @param Element reference
   * @param String optional position
   * @return Colorpicker this
   */
  insertTo: function(element, position) {
    return this
      .$super(element, position)
      .addClass('rui-colorpicker-inline');
  },

  /**
   * Checks if that's an inlined version of the widget
   *
   * @return Boolean check result
   */
  inlined: function() {
    return this.hasClass('rui-colorpicker-inline');
  },

  /**
   * Finalizes the action
   *
   * @return Colorpicker this
   */
  done: function() {
    if (!this.inlined()) {
      this.hide();
    }
    return this;
  },

// protected

  // catching up the user options
  setOptions: function(user_options) {
    user_options = user_options || {};
    this.$super(user_options, $(user_options.trigger || user_options.update));
  },

  // updates the preview and pointer positions
  update: function() {
    this.field._.style.backgroundColor   = 'rgb('+ this.tint.map(function(c) { return Math.round(c*255); }) +')';

    // updating the input fields
    var color = this.color, controls = this.controls;

    controls.preview._.style.backgroundColor = controls.display._.value = this.toHex();

    controls.rDisplay._.value = color[0];
    controls.gDisplay._.value = color[1];
    controls.bDisplay._.value = color[2];

    // adjusting the field pointer position
    var pointer = this.field.pointer._.style,
      field = this.field.size(),
      top  = field.y - this.bright * field.y - 2,
      left = this.satur * field.x - 2;

    pointer.top  = this.bound(top,  0, field.y - 5) + 'px';
    pointer.left = this.bound(left, 0, field.x - 5) + 'px';

    // adjusting the ting pointer position
    var tint = this.tint, position;
    field = this.colors.size();

    if (tint[1] == 0) { // the red-blue section
      position = tint[0] == 1 ? tint[2] : (2 - tint[0]);
    } else if (tint[0] == 0) { // the blue-green section
      position = 2 + (tint[2] == 1 ? tint[1] : (2 - tint[2]));
    } else { // the green-red section
      position = 4 + (tint[1] == 1 ? tint[0] : (2 - tint[1]));
    }

    position = position / 6 * field.y;

    this.colors.pointer._.style.top = this.bound(position, 0, field.y - 4) + 'px';

    // tracking the color change events
    if (this.prevColor !== ''+this.color) {
      this.fire('change', {value: this.color});
      this.prevColor = ''+ this.color;
    }

    return this;
  },

  // recalculates the state after the input field changes
  recalc: function(event) {
    if (event) {
      var field = event.target, value = field._.value, color = $A(this.color), changed=false;

      if (field === this.controls.display && /#\w{6}/.test(value)) {
        // using the hex values
        changed = color = this.toColor(value);
      } else if (/^\d+$/.test(value)) {
        // using the rgb values
        color[field._.cIndex] = value;
        changed  = true;
      }

      if (changed) { this.setValue(color); }

    } else {
      this.tint2color();
    }

    return this;
  },

  // starts the mousemoves tracking
  startTrack: function(event) {
    this.stopTrack();
    this.cancelTimer();

    if (event.target === this.field.pointer) {
      event.target = this.field;
    } else if (event.target === this.colors.pointer) {
      event.target = this.colors;
    }

    if (event.target === this.field || event.target === this.colors) {
      event.stop();
      Colorpicker.tracking = this;
      event.target.tracking = true;
      this.trackMove(event); // jumping over there
    }
  },

  // stops tracking the mousemoves
  stopTrack: function() {
    Colorpicker.tracking = false;
    this.field.tracking  = false;
    this.colors.tracking = false;
  },

  // tracks the cursor moves over the fields
  trackMove: function(event) {
    var field, pos = event.position(), top, left;

    if (this.field.tracking) {
      field   = this.field.dimensions();
    } else if (this.colors.tracking) {
      field   = this.colors.dimensions();
    }

    if (field) {
      top   = this.bound(pos.y - field.top,  0, field.height);
      left  = this.bound(pos.x - field.left, 0, field.width);

      if (this.field.tracking) {
        this.satur  = left / field.width;
        this.bright = 1 - top / field.height;

      } else if (this.colors.tracking) {
        // preventing it from jumping to the top
        if (top == field.height) { top = field.height - 0.1; }

        var step = field.height / 6,
            tint = this.tint = [0, 0, 0],
            stright = top % step / step,
            reverse = 1 - stright;

        if (top < step) {
          tint[0] = 1;
          tint[2] = stright;
        } else if (top < step * 2) {
          tint[0] = reverse;
          tint[2] = 1;
        } else if (top < step * 3) {
          tint[2] = 1;
          tint[1] = stright;
        } else if (top < step * 4) {
          tint[2] = reverse;
          tint[1] = 1;
        } else if (top < step * 5) {
          tint[1] = 1;
          tint[0] = stright;
        } else {
          tint[1] = reverse;
          tint[0] = 1;
        }
      }

      this.recalc().update();
    }
  },

  cancelTimer: function(event) {
    R(function() { // IE has a lack of sync in here
      if (this._hide_delay) {
        this._hide_delay.cancel();
        this._hide_delay = null;
      }
    }).bind(this).delay(10);
  }
});


/**
 * The colors field element
 *
 * Copyright (C) 2010-2011
 */
var Field = new Class(Element, {
  initialize: function(options) {
    this.$super('div', {'class': 'field'});
    this.insert(this.pointer = $E('div', {'class': 'pointer'}));
  }
});


/**
 * The tint picker block
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Colors = new Class(Element, {
  initialize: function() {
    this.$super('div', {'class': 'colors'});
    this.insert(this.pointer = $E('div', {'class': 'pointer'}));
  }
});


/**
 * The controls block unit
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Controls = new Class(Element, {
  initialize: function() {
    this.$super('div', {'class': 'controls'});
    this.insert([
        this.preview = $E('div', {'class': 'preview', 'html': '&nbsp;'}),
        this.display = $E('input', {'type': 'text', 'class': 'display', maxlength: 7}),
        $E('div', {'class': 'rgb-display'}).insert([
          $E('div').insert([$E('label', {html: 'R:'}), this.rDisplay = $E('input', {maxlength: 3, cIndex: 0})]),
          $E('div').insert([$E('label', {html: 'G:'}), this.gDisplay = $E('input', {maxlength: 3, cIndex: 1})]),
          $E('div').insert([$E('label', {html: 'B:'}), this.bDisplay = $E('input', {maxlength: 3, cIndex: 2})])
        ]),
        this.button  = new Button(Colorpicker.i18n.Done).onClick('fire', 'done')
      ]);
  }
});


/**
 * This module contains various caluculations logic for
 * the Colorpicker widget
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Colorpicker.include({
  /**
   * Converts the color to a RGB string value
   *
   * @param Array optional color
   * @return String RGB value
   */
  toRgb: function(color) {
    return 'rgb('+ this.color.join(',') +')';
  },

  /**
   * Converts the color to a HEX string value
   *
   * @param Array optional color
   * @return String HEX value
   */
  toHex: function(color) {
    return '#'+ this.color.map(function(c) { return (c < 16 ? '0' : '') + c.toString(16); }).join('');
  },

  /**
   * Converts a string value into an Array of color
   *
   * @param String value
   * @return Array of color or null
   */
  toColor: function(in_value) {
    var value = in_value.toLowerCase(), match;

    if ((match = /rgb\((\d+),(\d+),(\d+)\)/.exec(value))) {
      return [match[1], match[2], match[3]].map(parseInt);

    } else if (/#[\da-f]+/.test(value)) {
      // converting the shortified hex in to the full-length version
      if ((match = /^#([\da-f])([\da-f])([\da-f])$/.exec(value))) {
        value = '#'+match[1]+match[1]+match[2]+match[2]+match[3]+match[3];
      }

      if ((match = /#([\da-f]{2})([\da-f]{2})([\da-f]{2})/.exec(value))) {
        return [match[1], match[2], match[3]].map(function(n) { return parseInt(n, 16); });
      }
    }
  },

  /**
   * converts color into the tint, saturation and brightness values
   *
   * @return Colorpicker this
   */
  color2tint: function() {
    var color = $A(this.color).sort(function(a,b) { return a-b; }),
        min = color[0], max = color[2];

    this.bright = max / 255;
    this.satur  = 1 - min / (max || 1);

    this.tint.each(function(value, i) {
      this.tint[i] = ((!min && !max) || min == max) ? i == 0 ? 1 : 0 :
        (this.color[i] - min) / (max - min);
      return this.tint[i];
    }, this);

    return this;
  },

  /**
   * Converts tint, saturation and brightness into the actual RGB color
   *
   * @return Colorpicker this
   */
  tint2color: function() {
    var tint = this.tint, color = this.color;

    for (var i=0; i < 3; i++) {
      color[i] = 1 + this.satur * (tint[i] - 1);
      color[i] = Math.round(255 * color[i] * this.bright);
    }

    return this;
  },

  /**
   * bounds the value to the given limits
   *
   * @param {Number} value
   * @param {Number} min value
   * @param {Number} max value
   * @return {Number} the value in bounds
   */
  bound: function(in_value, min, max) {
    var value = in_value;

    if (min < max) {
      value = value < min ? min : value > max ? max : value;
    } else {
      if (value > max) { value = max; }
      if (value < min) { value = min; }
    }

    return value;
  }
});


/**
 * The document level hooks for colorpicker
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */

function document_mouseup() {
  if (Colorpicker.tracking) {
    Colorpicker.tracking.stopTrack();
  }
}

function document_mousemove(event) {
  if (Colorpicker.tracking) {
    Colorpicker.tracking.trackMove(event);
  }
}

$(document).on({
  mouseup:  document_mouseup,
  touchend: document_mouseup,

  mousemove: document_mousemove,
  touchmove: document_mousemove,

  focus: function(event) {
    var target = event.target instanceof Input ? event.target : null;

    Colorpicker.hideAll();

    if (target && (target.colorpicker || target.match(Colorpicker.Options.cssRule))) {

      (target.colorpicker || new Colorpicker({update: target}))
        .setValue(target.value()).showAt(target);
    }
  },

  blur: function(event) {
    var target = event.target, colorpicker = target.colorpicker;

    if (colorpicker) {
      // we use the delay so it didn't get hidden when the user clicks the calendar itself
      colorpicker._hide_delay = R(function() {
        colorpicker.hide();
      }).delay(200);
    }
  },

  click: function(event) {
    var target = (event.target instanceof Element) ? event.target : null;

    if (target && (target.colorpicker || target.match(Colorpicker.Options.cssRule))) {
      if (!(target instanceof Input)) {
        event.stop();
        (target.colorpicker || new Colorpicker({trigger: target}))
          .hide(null).toggleAt(target.assignedInput);
      }
    } else if (!event.find('div.rui-colorpicker')){
      Colorpicker.hideAll();
    }
  },

  keydown: function(event) {
    var colorpicker = Colorpicker.current, name = ({
      27: 'hide',        // Escape
      13: 'done'         // Enter
    })[event.keyCode];

    if (name && colorpicker && colorpicker.visible()) {
      event.stop();
      colorpicker[name]();
    }
  }
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("*.rui-button{display:inline-block; *display:inline; *zoom:1;height:1em;line-height:1em;margin:0;padding:.2em .5em;text-align:center;border:1px solid #CCC;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;cursor:pointer;color:#333;background-color:#FFF;user-select:none;-moz-user-select:none;-webkit-user-select:none} *.rui-button:hover{color:#111;border-color:#999;background-color:#DDD;box-shadow:#888 0 0 .1em;-moz-box-shadow:#888 0 0 .1em;-webkit-box-shadow:#888 0 0 .1em} *.rui-button:active{color:#000;border-color:#777;text-indent:1px;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none} *.rui-button-disabled, *.rui-button-disabled:hover, *.rui-button-disabled:active{color:#888;background:#DDD;border-color:#CCC;cursor:default;text-indent:0;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none}div.rui-re-anchor{margin:0;padding:0;background:none;border:none;float:none;display:inline;position:absolute;z-index:9999}.rui-panel{margin:0;padding:.5em;position:relative;background-color:#EEE;border:1px solid #BBB;border-radius:.3em;-moz-border-radius:.3em;-webkit-border-radius:.3em;box-shadow:.15em .3em .5em #BBB;-moz-box-shadow:.15em .3em .5em #BBB;-webkit-box-shadow:.15em .3em .5em #BBB;cursor:default}div.rui-colorpicker .field,div.rui-colorpicker .field *,div.rui-colorpicker .colors,div.rui-colorpicker .colors *{border:none;background:none;width:auto;height:auto;position:static;float:none;top:none;left:none;right:none;bottom:none;margin:0;padding:0;display:block;font-weight:normal;vertical-align:center}div.rui-colorpicker div.field,div.rui-colorpicker div.field div.pointer,div.rui-colorpicker div.colors,div.rui-colorpicker div.colors div.pointer{background:url(/images/rightjs-ui/colorpicker.png) no-repeat 0 0}div.rui-colorpicker div.field,div.rui-colorpicker div.colors,div.rui-colorpicker div.controls{display:inline-block; *display:inline; *zoom:1;position:relative;vertical-align:top;height:150px}div.rui-colorpicker div.field div.pointer,div.rui-colorpicker div.colors div.pointer{position:absolute;top:0px;left:0;width:9px;height:9px}div.rui-colorpicker input.display,div.rui-colorpicker div.preview,div.rui-colorpicker div.rgb-display,div.rui-colorpicker input.rui-ui-button{font-size:100%;display:block;width:auto;padding:0 .2em}div.rui-colorpicker input.display,div.rui-colorpicker div.preview,div.rui-colorpicker div.rgb-display input,div.rui-colorpicker input.rui-ui-button{border:1px solid #AAA;-moz-border-radius:.2em;-webkit-border-radius:.2em}div.rui-colorpicker div.field{width:150px;background-color:red;cursor:crosshair;margin-right:1.2em}div.rui-colorpicker div.field div.pointer{background-position:-170px 0;margin-left:-2px;margin-top:-2px}div.rui-colorpicker div.colors{width:16px;background-position:-150px 0;border-color:#EEE;cursor:pointer;margin-right:.6em}div.rui-colorpicker div.colors div.pointer{cursor:default;background-position:-170px -20px;margin-left:-8px;margin-top:-3px}div.rui-colorpicker div.controls{width:5em}div.rui-colorpicker div.preview{height:2em;background:white;border-color:#BBB}div.rui-colorpicker input.display{margin-top:.5em;background:#FFF;width:4.5em}div.rui-colorpicker div.rgb-display{padding:0;text-align:right;margin-top:.5em}div.rui-colorpicker div.rgb-display label{display:inline}div.rui-colorpicker div.rgb-display label:after{content:none}div.rui-colorpicker div.rgb-display input{vertical-align:top;font-size:100%;width:2em;text-align:right;margin-left:.2em;padding:0 .2em;background:#FFF;margin-bottom:1px;display:inline}div.rui-colorpicker div.rui-button{cursor:pointer;position:absolute;bottom:0;right:0;width:4em}div.rui-colorpicker-inline{display:inline-block; *display:inline; *zoom:1;position:relative;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none;z-index:auto}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Colorpicker;
})(document, Math, parseInt, RightJS);
