/**
 * RightJS-UI Autocompleter v2.2.2
 * http://rightjs.org/ui/autocompleter
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var Autocompleter = RightJS.Autocompleter = (function(document, RightJS) {
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
 * A shared module to create textual spinners
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Spinner = new RightJS.Class(RightJS.Element, {
  /**
   * Constructor
   *
   * @param Number optional spinner size (4 by default)
   * @return void
   */
  initialize: function(size) {
    this.$super('div', {'class': 'rui-spinner'});
    this.dots = [];

    for (var i=0; i < (size || 4); i++) {
      this.dots.push(new RightJS.Element('div'));
    }

    this.dots[0].addClass('glowing');
    this.insert(this.dots);
    RightJS(this.shift).bind(this).periodical(300);
  },

  /**
   * Shifts the spinner elements
   *
   * @return void
   */
  shift: function() {
    if (this.visible()) {
      var dot = this.dots.pop();
      this.dots.unshift(dot);
      this.insert(dot, 'top');
    }
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
 * Autocompleter initializer
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var R       = RightJS,
    $       = RightJS.$,
    $w      = RightJS.$w,
    $E      = RightJS.$E,
    Xhr     = RightJS.Xhr,
    RegExp  = RightJS.RegExp,
    isArray = RightJS.isArray;








/**
 * The RightJS UI Autocompleter unit base class
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
var Autocompleter = new Widget('UL', {
  include: Toggler,

  extend: {
    version: '2.2.2',

    EVENTS: $w('show hide update load select done'),

    Options: {
      url:        document.location.href,
      param:      'search',
      method:     'get',

      minLength:  1,         // the minimal length when it starts work
      threshold:  200,       // the typing pause threshold

      cache:      true,      // use the results cache
      local:      null,      // an optional local search results list

      fxName:     'slide',   // list appearance fx name
      fxDuration: 'short',   // list appearance fx duration

      spinner:    'native',  // spinner element reference

      cssRule:    'input[data-autocompleter]' // the auto-initialization css-rule
    }
  },

  /**
   * basic constructor
   *
   * @param mixed the input element reference, a string id or the element instance
   * @param Object options
   */
  initialize: function(input, options) {
    this.input = $(input); // KEEP IT before the super call

    this
      .$super('autocompleter', options)
      .addClass('rui-dd-menu')
      .onMousedown(this.clicked);

    this.input.autocompleter = this;
  },

  /**
   * Destructor
   *
   * @return Autocompleter this
   */
  destroy: function() {
    delete(this.input.autocompleter);
    return this;
  },

  /**
   * picks the next item on the list
   *
   * @return Autocompleter this
   */
  prev: function() {
    return this.pick('prev');
  },

  /**
   * picks the next item on the list
   *
   * @return Autocompleter this
   */
  next: function() {
    return this.pick('next');
  },

  /**
   * triggers the done event, sets up the value and closes the list
   *
   * @return Autocompleter this
   */
  done: function(current) {
    current = current || this.first('li.current');

    if (current) {
      current.radioClass('current');
      this.input.setValue(current._.textContent || current._.innerText);
      this.fire('done');
    }

    return this.hide();
  },

// protected

  // preprocessing the urls a bit
  setOptions: function(options) {
    this.$super(options, this.input);

    options = this.options;

    // building the correct url template with a placeholder
    if (!R(options.url).includes('%{search}')) {
      options.url += (R(options.url).includes('?') ? '&' : '?') + options.param + '=%{search}';
    }
  },

  // works with the 'prev' and 'next' methods
  pick: function(which_one) {
    var items   = this.children(),
        current = items.first('hasClass', 'current'),
        index   = items.indexOf(current);

    if (which_one == 'prev') {
      current = index < 1 ? items.last() : items[index < 0 ? 0 : (index-1)];
    } else if (which_one == 'next') {
      current = index < 0 || index == (items.length - 1) ?
        items.first() : items[index + 1];
    }

    return this.fire('select', {item: current.radioClass('current')});
  },

  // handles mouse clicks on the list element
  clicked: function(event) {
    this.done(event.stop().find('li'));
  },

  // handles the key-press events
  keypressed: function(event) {
    if (this.input.value().length >= this.options.minLength) {
      if (this.timeout) {
        this.timeout.cancel();
      }
      this.timeout = R(this.trigger).bind(this).delay(this.options.threshold);
    } else {
      return this.hide();
    }
  },

  // triggers the actual action
  trigger: function() {
    this.timeout = null;

    this.cache = this.cache || {};
    var search = this.input.value(), options = this.options;

    if (search.length < options.minLength) { return this.hide(); }

    if (this.cache[search]) {
      this.suggest(this.cache[search], search);
    } else if (isArray(options.local)) {
      this.suggest(this.findLocal(search), search);
    } else {
      this.request = Xhr.load(options.url.replace('%{search}', encodeURIComponent(search)), {
        method:  options.method,
        spinner: this.getSpinner(),
        onComplete: R(function(response) {
          this.fire('load').suggest(response.text, search);
        }).bind(this)
      });
    }
  },

  // updates the suggestions list
  suggest: function(result_text, search) {
    // saving the result in cache
    if (this.options.cache) {
      this.cache[search] = result_text;
    }

    if (!R(result_text).blank()) {
      this.update(result_text.replace(/<ul[^>]*>|<\/ul>/im, ''));
      this.fire('update');
      if (!this._connected || this.hidden()) {
        this.showAt(this.input, 'bottom', 'resize');
        this._connected = true;
      }
    } else {
      this.hide();
    }

    return this;
  },

  // performs the locals search
  findLocal: function(search) {
    var regexp  = new RegExp("("+RegExp.escape(search)+")", 'ig');

    return R(this.options.local).map(function(option) {
      if (option.match(regexp)) {
        return '<li>'+ option.replace(regexp, '<strong>$1</strong>') +'</li>';
      }
    }).compact().join('');
  },

  // builds a native textual spinner if necessary
  getSpinner: function() {
    var options = this.options, spinner = options.spinner;

    if (spinner == 'native') {
      spinner = options.spinner = new Spinner(3).insertTo(this);
      spinner.addClass('rui-autocompleter-spinner');
    }

    // positioning the native spinner
    if (spinner instanceof Spinner) {
      Toggler_re_position.call(spinner, this.input, 'right', 'resize');
    }

    return spinner;
  }
});


/**
 * The document events hooking
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
$(document).on({
  /**
   * Initializes autocompleters on-focus
   *
   * @param Event focus
   * @return void
   */
  focus: function(event) {
    var target = event.target;

    if (target && (target instanceof RightJS.Element) && (target.autocompleter || target.match(Autocompleter.Options.cssRule))) {
      if (!target.autocompleter) {
        new Autocompleter(target);
      }
    }
  },

  /**
   * Hides autocompleters on-blur
   *
   * @param Event blur
   * @return void
   */
  blur: function(event) {
    var autocompleter = event.target ? event.target.autocompleter : null;

    if (autocompleter && autocompleter.visible()) {
      autocompleter.hide();
    }
  },

  /**
   * Catching the basic keyboard events
   * to navigate through the autocompletion list
   *
   * @param Event keydown
   * @return void
   */
  keydown: function(event) {
    var autocompleter = event.target ? event.target.autocompleter : null;

    if (autocompleter && autocompleter.visible()) {
      var method_name = ({
        27: 'hide', // Esc
        38: 'prev', // Up
        40: 'next', // Down
        13: 'done'  // Enter
      })[event.keyCode];

      if (method_name) {
        event.stop();
        autocompleter[method_name]();
      }
    }
  },

  /**
   * Catches the input fields keyup events
   * and tries to make the autocompleter to show some suggestions
   *
   * @param Event keyup
   * @return void
   */
  keyup: function(event) {
    var autocompleter = event.target ? event.target.autocompleter : null;

    if (autocompleter && !R([9, 27, 37, 38, 39, 40, 13]).include(event.keyCode)) {
      autocompleter.keypressed(event);
    }
  }
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("*.rui-dd-menu, *.rui-dd-menu li{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none} *.rui-dd-menu{display:none;position:absolute;z-index:9999;background:white;border:1px solid #BBB;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;box-shadow:#DDD .2em .2em .4em;-moz-box-shadow:#DDD .2em .2em .4em;-webkit-box-shadow:#DDD .2em .2em .4em} *.rui-dd-menu li{padding:.2em .4em;border-top:none;border-bottom:none;cursor:pointer} *.rui-dd-menu li.current{background:#DDD} *.rui-dd-menu li:hover{background:#EEE}dl.rui-dd-menu dt{padding:.3em .5em;cursor:default;font-weight:bold;font-style:italic;color:#444;background:#EEE}dl.rui-dd-menu dd li{padding-left:1.5em}div.rui-spinner,div.rui-spinner div{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none;display:inline-block; *display:inline; *zoom:1;border-radius:.12em;-moz-border-radius:.12em;-webkit-border-radius:.12em}div.rui-spinner{text-align:center;white-space:nowrap;background:#EEE;border:1px solid #DDD;height:1.2em;padding:0 .2em}div.rui-spinner div{width:.4em;height:70%;background:#BBB;margin-left:1px}div.rui-spinner div:first-child{margin-left:0}div.rui-spinner div.glowing{background:#777}div.rui-re-anchor{margin:0;padding:0;background:none;border:none;float:none;display:inline;position:absolute;z-index:9999}.rui-autocompleter{border-top-color:#DDD !important;border-top-left-radius:0 !important;border-top-right-radius:0 !important;-moz-border-radius-topleft:0 !important;-moz-border-radius-topright:0 !important;-webkit-border-top-left-radius:0 !important;-webkit-border-top-right-radius:0 !important}.rui-autocompleter-spinner{border:none !important;background:none !important;position:absolute;z-index:9999}.rui-autocompleter-spinner div{margin-top:.2em !important; *margin-top:0.1em !important}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Autocompleter;
})(document, RightJS);
