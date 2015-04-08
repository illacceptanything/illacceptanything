/**
 * Additional Visual Effects v2.2.0
 * http://rightjs.org/plugins/effects
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
(function(RightJS) {
  if (!RightJS.Fx) { throw "RightJS Fx is missing"; }
  
  /**
 * The plugin initializtion script
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */

var R        = RightJS,
    $        = RightJS.$,
    $w       = RightJS.$w,
    $A       = RightJS.$A,
    Fx       = RightJS.Fx,
    Class    = RightJS.Class,
    Object   = RightJS.Object,
    Element  = RightJS.Element,
    defined  = RightJS.defined,
    isHash   = RightJS.isHash,
    isString = RightJS.isString;

RightJS.Effects = {
  version: '2.2.0'
};



/**
 * The basic move visual effect
 *
 * @copyright (C) 2009-2010 Nikolay V. Nemshilov
 */
Fx.Move = new Class(Fx.Morph, {
  extend: {
    Options: Object.merge(Fx.Options, {
      duration: 'long',
      position: 'absolute' // <- defines the positions measurment principle, not the element positioning
    })
  },
  
  prepare: function(end_position) {
    return this.$super(this.getEndPosition(end_position));
  },
  
  // moved to a separated method to be able to call it from subclasses
  getEndPosition: function(end_position) {
    var position = this.element.getStyle('position'), end_style = {};
    
    if (position != 'absolute' || position != 'relative') {
      this.element._.style.position = position = position == 'fixed' ? 'absolute' : 'relative';
    }
    
    if (end_position.top)  { end_position.y = end_position.top.toInt();  }
    if (end_position.left) { end_position.x = end_position.left.toInt(); }
    
    // adjusting the end position
    var cur_position = this.element.position();
    var par_position = this.getParentPosition();
    var rel_left     = cur_position.x - par_position.x;
    var rel_top      = cur_position.y - par_position.y;
    
    if (this.options.position == 'relative') {
      if (position == 'absolute') {
        if (defined(end_position.x)) { end_position.x += cur_position.x; }
        if (defined(end_position.y)) { end_position.y += cur_position.x; }
      } else {
        if (defined(end_position.x)) { end_position.x += rel_left; }
        if (defined(end_position.y)) { end_position.y += rel_top;  }
      }
    } else if (position == 'relative') {
      if (defined(end_position.x)) { end_position.x += rel_left - cur_position.x; }
      if (defined(end_position.y)) { end_position.y += rel_top  - cur_position.y; }
    }
    
    // need this to bypass the other styles from the subclasses
    for (var key in end_position) {
      switch (key) {
        case 'top': case 'left': break;
        case 'y':   end_style.top  = end_position.y + 'px'; break;
        case 'x':   end_style.left = end_position.x + 'px'; break;
        default:    end_style[key] = end_position[key];
      }
    }
    
    return end_style;
  },
  
  getParentPosition: function() {
    Fx.Move.Dummy = Fx.Move.Dummy || new Element('div', {style: 'width:0;height:0;visibility:hidden'});
    this.element.insert(Fx.Move.Dummy, 'before');
    var position = Fx.Move.Dummy.position();
    Fx.Move.Dummy.remove();
    return position;
  }
});

/**
 * Zoom visual effect, graduately zoom and element in or out
 *
 * @copyright (C) 2009-2011 Nikolay V. Nemshilov
 */
Fx.Zoom = new Class(Fx.Move, {
  PROPERTIES: $w('width height lineHeight paddingTop paddingRight paddingBottom paddingLeft fontSize borderWidth'),

  extend: {
    Options: Object.merge(Fx.Move.Options, {
      position: 'relative', // overriding the Fx.Move default
      duration: 'normal',
      from:     'center'
    })
  },

  prepare: function(size, additional_styles) {
    return this.$super(this._getZoomedStyle(size, additional_styles));
  },

// private

  // calculates the end zoommed style
  _getZoomedStyle: function(size, additional_styles) {
    var proportion = this._getProportion(size);

    return Object.merge(
      this._getBasicStyle(proportion),
      this._getEndPosition(proportion),
      additional_styles
    );
  },

  // calculates the zooming proportion
  _getProportion: function(size) {
    if (isHash(size)) {
      var dummy = $E('div').insertTo(
        $E('div', {
          style: "visibility:hidden;float:left;height:0;width:0"
        }).insertTo(document.body)
      ).setStyle(size);

      size = size.height ?
        dummy.size().y / this.element.size().y :
        dummy.size().x / this.element.size().x ;

      dummy.remove();
    } else if (isString(size)) {
      size  = R(size).endsWith('%') ? R(size).toFloat() / 100 : R(size).toFloat();
    }

    return size;
  },

  // getting the basic end style
  _getBasicStyle: function(proportion) {
    var style = clone_styles(this.element, this.PROPERTIES), re = /([\d\.]+)/g;

    function adjust_value(m) {
      return ''+ (R(m).toFloat() * proportion);
    }

    for (var key in style) {
      if (key === 'width' || key === 'height') {
        style[key] = style[key] || (this.element['offset'+R(key).capitalize()]+'px');
      }

      if (style[key].match(re)) {
        style[key] = style[key].replace(re, adjust_value);
      } else {
        delete(style[key]);
      }
    }

    // preventing the border disappearance
    if (style.borderWidth && R(style.borderWidth).toFloat() < 1) {
      style.borderWidth = '1px';
    }

    return style;
  },

  // getting the position adjustments
  _getEndPosition: function(proportion) {
    var position = {};
    var sizes    = this.element.size();
    var x_diff   = sizes.x * (proportion - 1);
    var y_diff   = sizes.y * (proportion - 1);

    switch (this.options.from.replace('-', ' ').split(' ').sort().join('_')) {
      case 'top':
        position.x = - x_diff / 2;
        break;

      case 'right':
        position.x = - x_diff;
        position.y = - y_diff / 2;
        break;

      case 'bottom':
        position.x = - x_diff / 2;
      case 'bottom_left':
        position.y = - y_diff;
        break;

      case 'bottom_right':
        position.y = - y_diff;
      case 'right_top':
        position.x = - x_diff;
        break;

      case 'center':
        position.x = - x_diff / 2;
      case 'left':
        position.y = - y_diff / 2;
        break;

      default: // left_top or none, do nothing, let the thing expand as is
    }

    return position;
  }
});

function clone_styles(element, keys) {
  for (var i=0, len = keys.length, style = element.computedStyles(), clean = {}, key; i < len; i++) {
    key = keys[i];
    if (key in style) {
      clean[key] = ''+ style[key];
    }
  }

  return clean;
}

/**
 * Bounce visual effect, slightly moves an element forward and back
 *
 * @copyright (C) 2009 Nikolay V. Nemshilov
 */
Fx.Bounce = new Class(Fx, {
  extend: {
    Options: Object.merge(Fx.Options, {
      duration:  'short',
      direction: 'top',
      value:     16 // the shake distance
    })
  },
  
  prepare: function(value) {
    value = value || this.options.value;
    
    var position = this.element.position();
    var duration = Fx.Durations[this.options.duration]     || this.options.duration;
    var move_options = {duration: duration, position: 'relative'};
    
    var key = 'y'; // top bounce by default
    
    switch (this.options.direction) {
      case 'right':
        value = -value;
      case 'left':
        key = 'x';
        break;
      case 'bottom':
        value = -value;
    }
    
    var up_pos = {}, down_pos = {};
    up_pos[key]   = -value;
    down_pos[key] = value;
    
    new Fx.Move(this.element, move_options).start(up_pos);
    new Fx.Move(this.element, move_options).start(down_pos);
    
    this.finish.bind(this).delay(1);
    
    return this;
  }
});

/**
 * run out and run in efffects
 *
 * Copyright (C) 2009-2010 Nikolay V. Nemshilov
 */
Fx.Run = new Class(Fx.Move, {
  extend: {
    Options: Object.merge(Fx.Move.Options, {
      direction: 'left'
    })
  },
  
  prepare: function(in_how) {
    var how = in_how || 'toggle', position = {}, dimensions = this.element.dimensions(), threshold = 80;
    
    if (how == 'out' || (how == 'toggle' && this.element.visible())) {
      if (this.options.direction == 'left') {
        position.x = -dimensions.width - threshold;
      } else {
        position.y = -dimensions.height - threshold;
      }
      this.onFinish(function() {
        this.element.hide().setStyle(this.getEndPosition({x: dimensions.left, y: dimensions.top}));
      });
    } else {
      dimensions = this.element.setStyle('visibility: hidden').show().dimensions();
      var pre_position = {};
      
      if (this.options.direction == 'left') {
        pre_position.x = - dimensions.width - threshold;
        position.x = dimensions.left;
      } else {
        pre_position.y = - dimensions.height - threshold;
        position.y = dimensions.top;
      }
      
      this.element.setStyle(this.getEndPosition(pre_position)).setStyle('visibility: visible');
    }
    
    return this.$super(position);
  }
});

/**
 * The puff visual effect
 *
 * Copyright (C) 2009-2010 Nikolay V. Nemshilov
 */
Fx.Puff = new Class(Fx.Zoom, {
  extend: {
    Options: Object.merge(Fx.Zoom.Options, {
      size: 1.4  // the end/initial size of the element
    })
  },
  
// protected

  prepare: function(in_how) {
    var how = in_how || 'toggle', opacity = 0, size = this.options.size, initial_style;
    
    if (how == 'out' || (how == 'toggle' && this.element.visible())) {
      initial_style = this.getEndPosition(this._getZoomedStyle(1));
      this.onFinish(function() {
        initial_style.opacity = 1;
        this.element.hide().setStyle(initial_style);
      });
      
    } else {
      this.element.setStyle('visibility: visible').show();
      
      var width = this.element.offsetWidth;
      initial_style = this.getEndPosition(this._getZoomedStyle(1));
      
      this.onFinish(function() {
        this.element.setStyle(initial_style);
      });
      
      this.element.setStyle(Object.merge(
        this.getEndPosition(this._getZoomedStyle(size)), {
          opacity: 0,
          visibility: 'visible'
        }
      ));
      
      size = width / this.element.offsetWidth;
      opacity = 1;
    }
    
    
    return this.$super(size, {opacity: opacity});
  }
  
});

/**
 * Glow effect, kinda the same thing as Hightlight, but changes the text color
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
Fx.Glow = new Class(Fx.Morph, {
  extend: {
    Options: Object.merge(Fx.Options, {
      color:      '#FF8',
      transition: 'Exp'
    })
  },

  // protected

  /**
   * starts the transition
   *
   * @param high String the hightlight color
   * @param back String optional fallback color
   * @return self
   */
  prepare: function(start, end) {
    var element       = this.element,
        element_style = element._.style,
        style_name    = 'color',
        end_color     = end || element.getStyle(style_name);

    // trying to find the end color
    end_color = [element].concat(element.parents())
      .map('getStyle', style_name)
      .compact().first() || '#FFF';

    element_style[style_name] = (start || this.options.color);

    return this.$super({color: end_color});
  }
});

/**
 * Element shortcuts for the additional effects
 *
 * @copyright (C) 2009-2011 Nikolay Nemshilov
 */
RightJS.Element.include({
  /**
   * The move visual effect shortcut
   *
   * @param position Object end position x/y or top/left
   * @param options Object fx options
   * @return Element self
   */
  move: function(position, options) {
    return call_fx(this, 'move', [position, options || {}]); // <- don't replace with arguments
  },

  /**
   * The bounce effect shortcut
   *
   * @param Number optional bounce size
   * @param Object fx options
   * @return Element self
   */
  bounce: function() {
    return call_fx(this, 'bounce', arguments);
  },

  /**
   * The zoom effect shortcut
   *
   * @param mixed the zooming value, see Fx.Zoom#start options
   * @param Object fx options
   * @return Element self
   */
  zoom: function(size, options) {
    return call_fx(this, 'zoom', [size, options || {}]);
  },

  /**
   * Initiates the Fx.Run effect
   *
   * @param String running direction
   * @param Object fx options
   * @return Element self
   */
  run: function() {
    return call_fx(this, 'run', arguments);
  },

  /**
   * The puff effect shortcut
   *
   * @param String running direction in|out|toggle
   * @param Object fx options
   * @return Element self
   */
  puff: function() {
    return call_fx(this, 'puff', arguments);
  },

  /**
   * The Fx.Glow effect shortcut
   *
   * @param String optinal glow color
   * @param Object fx options
   * @return Element self
   */
  glow: function() {
    return call_fx(this, 'glow', arguments);
  }
});

/**
 * Runs Fx on the element
 *
 * @param Element element reference
 * @param String fx name
 * @param Array effect arguments
 * @return the element back
 */
function call_fx(element, name, params) {
  var args    = $A(params).compact(),
      options = isHash(args.last()) ? args.pop() : {},
      fx      = new Fx[name.capitalize()](element, options);

  fx.start.apply(fx, args);

  return element;
}

})(RightJS);