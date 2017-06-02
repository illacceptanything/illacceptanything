/**
 * RightJS-UI Billboard v2.2.0
 * http://rightjs.org/ui/billboard
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Billboard = RightJS.Billboard = (function(RightJS) {
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
 * Billboard initialization script
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var R      = RightJS,
    $      = RightJS.$,
    $$     = RightJS.$$,
    $w     = RightJS.$w,
    $E     = RightJS.$E,
    Fx     = RightJS.Fx,
    Class  = RightJS.Class,
    Object = RightJS.Object;




/**
 * Billboards basic class
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Billboard = new Widget('UL', {
  extend: {
    version: '2.2.0',

    EVENTS:  $w('change first last'),

    Options: {
      fxName:      'stripe',  // visual effect name
      fxDuration:  'long',    // visual effect duration

      autostart:   true,      // if it should automatically start rotate things
      delay:       4000,      // delay between item shows
      loop:        true,      // loop after reaching the last one

      showButtons: true,      // should it show the next/prev buttons or not
      prevButton:  'native',  // prev item button, 'native' or an ID of your own
      nextButton:  'native',  // next item button, 'native' or an ID of your own

      stripes:     10,        // the number of stripes

      cssRule:     '*.rui-billboard'
    }
  },

  /**
   * Basic constructor
   *
   * @param mixed an element reference
   * @return void
   */
  initialize: function(element) {
    this.$super('billboard', element);

    // initializing the buttons
    if (this.options.showButtons) {
      this.prevButton = this.options.prevButton !== 'native' ? $(this.options.prevButton) :
        $E('div', {'class': 'rui-billboard-button-prev', 'html': '&lsaquo;'}).insertTo(this);
      this.nextButton = this.options.nextButton !== 'native' ? $(this.options.nextButton) :
        $E('div', {'class': 'rui-billboard-button-next', 'html': '&rsaquo;'}).insertTo(this);

      this.prevButton.onClick(R(function(event) {
        event.stop(); this.showPrev();
      }).bind(this));
      this.nextButton.onClick(R(function(event) {
        event.stop(); this.showNext();
      }).bind(this));
    }

    // catching the 'first'/'last' events
    this.onChange(function(event) {
      if (event.item === this.items().first()) {
        this.fire('first');
      } else if (event.item === this.items().last()) {
        this.fire('last');
      }
    });

    // stopping/starting the slideshow with mouse over/out events
    this.on({
      mouseover: function() {
        this.stop();
      },

      mouseout: function(event) {
        if (this.options.autostart && !event.find('.rui-billboard')) {
          this.start();
        }
      }
    });

    // autostart
    if (this.options.autostart) {
      this.start();
    }
  },

  /**
   * Returns the list of items to swap
   *
   * @return Array swappable items
   */
  items: function() {
    return this.children().without(this.prevButton, this.nextButton);
  },

  /**
   * Show next item on the list
   *
   * @return Billboard this
   */
  showNext: function() {
    var items = this.items(), index = items.indexOf(this.current()) + 1;

    if (index == items.length && this.options.loop) {
      index = 0;
    }

    return this.current(index);
  },

  /**
   * Show prev item on the list
   *
   * @return Billboard this
   */
  showPrev: function() {
    var items = this.items(), index = items.indexOf(this.current()) - 1;

    if (index < 0 && this.options.loop) {
      index = items.length - 1;
    }

    return this.current(index);
  },

  /**
   * Gets/sets the current item
   *
   * @param mixed integer index or a LI element reference
   * @return Billboard this or current LI element
   */
  current: function(index) {
    var items = this.items();

    if (arguments.length) {
      if (index instanceof Element) {
        index = items.indexOf(index);
      }

      this.runFx(items[index]);
    } else {
      return items.length ? (
        items.first('hasClass', 'rui-billboard-current')  ||
        items.first().addClass('rui-billboard-current')
      ) : null;
    }

    return this;
  },

  /**
   * Starts the slide show
   *
   * @return Billboard this
   */
  start: function() {
    this.timer = R(this.showNext).bind(this).periodical(this.options.delay);
  },

  /**
   * stops the slideshow
   *
   * @return Billboard this
   */
  stop: function() {
    if (this.timer) {
      this.timer.stop();
    }
  },

  /**
   * Wrapping the event trigger so it always sent the
   * current element references
   *
   * @param String event name
   * @param Object options
   * @return Billboard this
   */
  fire: function(name, options) {
    return this.$super(name, Object.merge({
      index: this.items().indexOf(this.current()),
      item:  this.current()
    }, options));
  },

// protected

  /**
   * Runs the fx transition
   *
   * @param Element new LI element
   * @return void
   */
  runFx: function(item) {
    if (item && !this._running) {
      var Fx = Billboard.Fx[R(this.options.fxName || '').capitalize()];

      if (Fx) {
        new Fx(this).start(this.current(), item);
      } else {
        this.current().removeClass('rui-billboard-current');
        item.addClass('rui-billboard-current');
      }
    }
  }
});

/**
 * Basic billboard visual effect
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Billboard.Fx = new Class(Fx, {

  /**
   * basic constructor
   *
   * @param Billboard billboard
   * @return void
   */
  initialize: function(billboard) {
    this.container = $E('div', {'class': 'rui-billboard-fx-container'});

    this.$super(billboard, {
      duration: billboard.options.fxDuration,
      onStart: function() {
        billboard._running = true;
        billboard.insert(this.container);
      },
      onFinish: function() {
        this.container.remove();
        billboard._running = false;
        billboard.fire('change');
      }
    });
  },

  /**
   * Starts an fx on the given item
   *
   * @param {Element} old LI element
   * @param {Element} new LI element
   * @return void
   */
  prepare: function(old_item, new_item) {
    old_item.removeClass('rui-billboard-current');
    new_item.addClass('rui-billboard-current');

    this.clone = old_item.clone();

    this.container.update(this.clone);
  }

});

/**
 * Fade visual effects class
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Billboard.Fx.Fade = new Class(Billboard.Fx, {

  /**
   * Starts an fx on the given item
   *
   * @param {Element} old LI element
   * @param {Element} new LI element
   * @return void
   */
  prepare: function(old_item, new_item) {
    this.$super(old_item, new_item);
  },

  /**
   * Rendering the effect
   *
   * @param Float delta value
   * @return void
   */
  render: function(delta) {
    this.container.setStyle({opacity: 1 - delta});
  }

});

/**
 * The slide visual effects class
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Billboard.Fx.Slide = new Class(Billboard.Fx, {

  /**
   * overloading the 'prepare' method to add some stuff
   * to the container depending on which direction do we slide
   *
   * @param {Element} old LI element
   * @param {Element} new LI element
   * @return void
   */
  prepare: function(old_item, new_item) {
    this._width = this.element.current().size().x;
    this._direction = old_item.nextSiblings().include(new_item) ? -1 : 1;

    this.$super(old_item, new_item);

    this.clone.setStyle({width: this._width + 'px'});
  },

  /**
   * Rendering the Fx
   *
   * @param Float delta
   * @return void
   */
  render: function(delta) {
    this.clone._.style.left = this._direction * this._width * delta + 'px';
  }

});

/**
 * Stripe visual effects class
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Billboard.Fx.Stripe = new Class(Billboard.Fx, {

  directions: ['down', 'up', 'left', 'right'],

  /**
   * Breaking the original element onto sripes in here
   *
   * @param {Element} old LI element
   * @param {Element} new LI element
   * @return void
   */
  prepare: function(old_item, new_item) {
    this.$super(old_item, new_item);

    var length    = this.element.options.stripes,
        width     = this.element.items()[0].size().x / length,
        delay     = 100,
        direction = this.directions.shift();

    this.directions.push(direction);
    this.container.clean();

    for (var i=0; i < length; i++) {
      var stripe = $E('div', {
        'class': 'rui-billboard-stripe',
        'style': {
          width: width + 1 + 'px',
          left:  i * width + 'px'
        }
      }).insert(old_item.clone().setStyle({
        width: width * length + 'px',
        left: - i * width  + 'px'
      }));

      this.container.append(stripe);
      var options = {
        duration: this.options.duration
      };

      if (direction !== 'right' && i === (length - 1) || (direction === 'right' && i === 0)) {
        options.onFinish = R(this.finish).bind(this, true);
      }

      switch (direction) {
        case 'up':
          R(function(stripe, options) {
            stripe.setHeight(stripe.size().y);
            stripe.morph({height: '0px'}, options);
          }).bind(this, stripe, options).delay(i * delay);
          break;

        case 'down':
          stripe.setStyle('top: auto; bottom: 0px');
          R(function(stripe, options) {
            stripe.setHeight(stripe.size().y);
            stripe.morph({height: '0px'}, options);
          }).bind(this, stripe, options).delay(i * delay);
          break;

        case 'left':
          R(function(stripe, options) {
            stripe.morph({width: '0px'}, options);
          }).bind(this, stripe, options).delay(i * delay);
          break;

        case 'right':
          R(function(stripe, options) {
            stripe.morph({width: '0px'}, options);
          }).bind(this, stripe, options).delay((length - i -1) * delay);
          break;

        default:
          this.finish(true);
      }
    }
  },


  /**
   * Stubbing the finish method so it didn't finish prematurely
   *
   * @return Fx this
   */
  finish: function(for_sure) {
    if (for_sure) {
      this.$super();
    }
    return this;
  }

});

/**
 * Document level hooks for billboards
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
$(document).onReady(function() {
  $$(Billboard.Options.cssRule).each(function(element) {
    if (!(element instanceof Billboard)) {
      element = new Billboard(element);
    }
  });
});

var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("*.rui-billboard, *.rui-billboard> *{margin:0;padding:0;list-style:none} *.rui-billboard{display:inline-block; *display:inline; *zoom:1;position:relative} *.rui-billboard> *{display:none;width:100%;height:100%} *.rui-billboard> *:first-child, *.rui-billboard> *.rui-billboard-current:first-child{display:block;position:relative} *.rui-billboard> *>img{margin:0;padding:0} *.rui-billboard-current{position:absolute;left:0;top:0;display:block;z-index:999} *.rui-billboard-button-prev, *.rui-billboard-button-next{position:absolute;z-index:99999;left:.25em;top:auto;bottom:.25em;display:block;width:.5em;height:auto;text-align:center;font-size:200%;font-family:Arial;font-weight:bold;padding:0em .5em .2em .5em;background:white;opacity:0;filter:alpha(opacity:0);cursor:pointer;border:.12em solid #888;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;user-select:none;-moz-user-select:none;-webkit-user-select:none;transition:opacity .3s ease-in-out;-o-transition:opacity .3s ease-in-out;-moz-transition:opacity .3s ease-in-out;-webkit-transition:opacity .3s ease-in-out} *.rui-billboard-button-next{left:auto;right:.25em;text-align:right} *.rui-billboard-button-prev:active{text-indent:-.1em} *.rui-billboard-button-next:active{text-indent:.2em} *.rui-billboard:hover *.rui-billboard-button-prev, *.rui-billboard:hover *.rui-billboard-button-next{opacity:0.4;filter:alpha(opacity:40)} *.rui-billboard:hover *.rui-billboard-button-prev:hover, *.rui-billboard:hover *.rui-billboard-button-next:hover{opacity:0.7;filter:alpha(opacity:70)}.rui-billboard-fx-container{position:absolute;left:0;top:0;display:block;z-index:9999;overflow:hidden}.rui-billboard-fx-container> *{position:absolute;left:0;top:0}.rui-billboard-stripe{overflow:hidden}.rui-billboard-stripe> *{position:relative}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Billboard;
})(RightJS);
