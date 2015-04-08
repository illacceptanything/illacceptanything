/**
 * RightJS-UI Rater v2.2.0
 * http://rightjs.org/ui/rater
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Rater = RightJS.Rater = (function(document, RightJS) {
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
 * Same as the assignable, only it doesn't work with popups
 * instead it simply updates the assigned unit value/content
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var Updater = {

  /**
   * Assigns the unit to work with an input element
   *
   * @param mixed element reference
   * @return Rater this
   */
  assignTo: function(element) {
    var assign  = R(function(element, event) {
      if ((element = $(element))) {
        element[element.setValue ? 'setValue' : 'update'](event.target.getValue());
      }
    }).curry(element);

    var connect = R(function(element, object) {
      element = $(element);
      if (element && element.onChange) {
        element.onChange(R(function() {
          this.setValue(element.value());
        }).bind(object));
      }
    }).curry(element);

    if ($(element)) {
      assign({target: this});
      connect(this);
    } else {
      $(document).onReady(R(function() {
        assign({target: this});
        connect(this);
      }.bind(this)));
    }

    return this.onChange(assign);
  }
};


/**
 * The init script for Rater
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var R        = RightJS,
    $        = RightJS.$,
    $w       = RightJS.$w,
    Xhr      = RightJS.Xhr,
    isString = RightJS.isString,
    isNumber = RightJS.isNumber;






/**
 * The Rating widget
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Rater = new Widget({
  include: Updater,

  extend: {
    version: '2.2.0',

    EVENTS: $w('change hover send'),

    Options: {
      html:          '&#9733;', // the dot html code

      size:          5,      // number of stars in the line
      value:         null,   // default value
      update:        null,   // an element to update

      disabled:      false,  // if it should be disabled
      disableOnVote: false,  // if it should be disabled when user clicks a value

      url:           null,   // an url to send results with AJAX
      param:         'rate', // the value param name
      Xhr:           null    // additional Xhr options
    }
  },

  /**
   * basic constructor
   *
   * @param mixed element reference or an options hash
   * @param Object options hash
   */
  initialize: function(options) {
    this
      .$super('rater', options)
      .on({
        click:      this._clicked,
        mouseover:  this._hovered,
        mouseout:   this._left
      });

    if (this.empty()) {
      for (var i=0; i < this.options.size; i++) {
        this.insert('<div>'+ this.options.html + '</div>');
      }
    }

    options = this.options;

    if (options.value === null) {
      options.value = this.find('.active').length;
    }

    this.setValue(options.value);

    if (options.disabled) {
      this.disable();
    }

    if (options.update) {
      this.assignTo(options.update);
    }
  },

  /**
   * Sets the element value
   *
   * @param Number or String value
   * @return Rater this
   */
  setValue: function(value) {
    if (!this.disabled()) {
      // converting the type and rounding the value
      value = isString(value) ? R(value).toInt() : value;
      value = isNumber(value) ? R(value).round() : 0;

      // checking constraints
      value = R(value).max(this.options.size);
      value = R(value).min(0);

      // highlighting the value
      this.highlight(value);

      if (this.value != value) {
        this.fire('change', {value: this.value = value});
      }
    }

    return this;
  },

  /**
   * Returns the current value of the rater
   *
   * @return Number value
   */
  getValue: function() {
    return this.value;
  },

  /**
   * Sends an Xhr request with the current value to the options.url address
   *
   * @return Rater this
   */
  send: function() {
    if (this.options.url) {
      this.request = new Xhr(this.options.url, this.options.Xhr)
        .send(this.options.param+"="+this.value);
      this.fire('send', {value: this.value});
    }
    return this;
  },

  /**
   * Disables the instance
   *
   * @return Rater this
   */
  disable: function() {
    return this.addClass('rui-rater-disabled');
  },

  /**
   * Enables this instance
   *
   * @return Rater this
   */
  enable: function() {
    return this.removeClass('rui-rater-disabled');
  },

  /**
   * Checks if the instance is disabled
   *
   * @return boolean
   */
  disabled: function() {
    return this.hasClass('rui-rater-disabled');
  },

// protected

  // callback for 'hover' event
  _hovered: function(event) {
    var index = this.children().indexOf(event.target);
    if (!this.disabled() && index > -1) {
      this.highlight(index + 1);
      this.fire('hover', {value: index + 1});
    }
  },

  // callback for user-click
  _clicked: function(event) {
    var index = this.children().indexOf(event.target);
    if (!this.disabled() && index > -1) {
      this.setValue(index + 1);
      if (this.options.disableOnVote) {
        this.disable();
      }
      this.send();
    }
  },

  // callback when user moves the mouse out
  _left: function() {
    this.setValue(this.value);
  },

  // visually highlights the value
  highlight: function(value) {
    this.children().each(function(element, i) {
      element[value - 1 < i ? 'removeClass' : 'addClass']('active');
    });
  }
});


/**
 * Document level on-demand auto-initialization
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
$(document).onMouseover(function(event) {
  var target = event.target, element = event.find('.rui-rater');

  if (element) {
    if (!(element instanceof Rater)) {
      element = new Rater(element);

      if (target.parent() === element) {
        target.fire('mouseover');
      }
    }
  }
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("div.rui-rater,div.rui-rater div{margin:0;padding:0;background:none;border:none;display:inline-block; *display:inline; *zoom:1;font-family:Arial;font-size:110%}div.rui-rater{width:6em;height:1em;vertical-align:middle}div.rui-rater div{float:left;width:1em;height:1em;line-height:1em;text-align:center;cursor:pointer;color:#888}div.rui-rater div.active{color:brown;text-shadow:#666 .05em .05em .15em}div.rui-rater-disabled div{cursor:default}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Rater;
})(document, RightJS);
