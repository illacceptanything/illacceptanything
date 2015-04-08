/**
 * RightJS-UI Selectable v2.2.3
 * http://rightjs.org/ui/selectable
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
var Selectable = RightJS.Selectable = (function(document, RightJS) {
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
 * The filenames to include
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var R        = RightJS,
    $        = RightJS.$,
    $$       = RightJS.$$,
    $w       = RightJS.$w,
    $E       = RightJS.$E,
    $A       = RightJS.$A,
    Object   = RightJS.Object,
    isHash   = RightJS.isHash,
    isArray  = RightJS.isArray,
    isString = RightJS.isString,
    isNumber = RightJS.isNumber,
    defined  = RightJS.defined,
    Input    = RightJS.Input,
    Element  = RightJS.Element;








/**
 * Selectable unit main script
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
var Selectable = new Widget('UL', {
  include: Updater,

  extend: {
    version: '2.2.3',

    EVENTS: $w('change select unselect disable enable hover leave show hide'),

    Options: {
      options:    null,    // a hash of key-value pairs
      selected:   null,    // an array of selected keys
      disabled:   null,    // an array of disabled keys

      multiple:   true,    // a flag if it shoulde a multiselect or a single select widget

      fxName:     'slide', // the drop-down options list fx-name null, 'slide', 'fade'
      fxDuration: 'short', // the drop-down options list fx-duration

      update:     null,    // a field to be assigned to
      parseIds:   false,   // if it should parse integer ids out of the keys

      limit:      null,    // put some number if you'd like to limit the number of selected items

      hCont  :   '&bull;'  // single-selectable handle content
    },

    // converting normal select boxes into selectables
    rescan: function(context) {
      $(context||document).find('.rui-selectable').each(function(element) {
        if (!(element instanceof Selectable)) {
          new Selectable(element);
        }
      });
    }
  },

  /**
   * Basic constructor
   *
   * @param mixed reference to an element or the options hash
   * @param Object options hash
   */
  initialize: function() {
    var args = $A(arguments).compact(), options = args.pop(),
      element = args.pop(), selectbox;

    // figuring out the arguments
    if (!isHash(options) || options instanceof Element) {
      element = $(element || options);
      options = null;
    }

    // converting the selectboxes
    if (element && (element = $(element)) instanceof Input) {
      this.selectbox = selectbox = element;
      options = Object.merge(this.harvestOptions(element), options);
      element = null;
    }

    // main initialization
    this
      .$super('selectable', element)
      .setOptions(options)
      .on({
        mousedown:  this._mousedown,
        touchstart: this._mousedown,
        mouseover:  this._mouseover,
        mouseout:   this._mouseout,
        mouseup:    this._mouseup,
        touchend:   this._mouseup,
        click:      this._click,

        select:    this._change,
        unselect:  this._change
      });

    if (this.empty()) { this.build(); }

    // applying the rest of the options
    options = this.options;

    // single-select options additional features
    if (!options.multiple || this.hasClass('rui-selectable-single')) {
      this.isSingle = true;
      this.addClass('rui-selectable-single');
      this.buildSingle();

      if (options.selected === null) {
        this.select(this.items()[0]);
      }
    }

    if (options.disabled) { this.disable(options.disabled); }
    if (options.selected) { this.select(options.selected);  }
    if (options.update)   { this.assignTo(options.update);  }

    // replacing the selectboxes with the selectables
    if (selectbox) {
      this.assignTo(selectbox).insertTo(selectbox, 'before');

      // hidding it in the hidden layer so it was sent with the form
      selectbox.wrap($E('div', {
        style: 'position:absolute;z-index:-1;visibility:hidden;width:0;height:0;overflow:hidden'
      }));
    }
  },

  /**
   * Sets the value
   *
   * @param Array of selectee keys
   * @return Selectable this
   */
  setValue: function(value) {
    // parsing the value
    if (isString(value)) {
      value = value.split(',').map('trim')
        .filter(function(s) { return !s.blank(); });
    }

    // resetting the selections
    this.items().each('removeClass', 'rui-selectable-selected');

    return this.select(value);
  },

  /**
   * Returns the list of selected items
   *
   * @return Array of selectees
   */
  getValue: function() {
    if (this.isSingle) {
      var item  = this.items().first('hasClass', 'rui-selectable-selected');
      return item ? this.itemValue(item) : null;
    } else {
      return this.items().filter('hasClass', 'rui-selectable-selected').map(function(item) {
        return this.itemValue(item);
      }, this);
    }
  },

  /**
   * disables the given key or keys
   * NOTE: if no keys specified, then all the items will be disabled
   *
   * @param mixed optional key or keys to disable
   * @return Selectable this
   */
  disable: function(keys) {
    this.mapOrAll(keys).each(function(item) {
      this.fire('disable', item.addClass('rui-selectable-disabled'));
    }, this);
    return this;
  },

  /**
   * disables the given key or keys
   * NOTE: if no keys specified, then all the items will be enabled
   *
   * @param mixed optional key or keys to enable
   * @return Selectable this
   */
  enable: function(keys) {
    this.mapOrAll(keys).each(function(item) {
      this.fire('enable', item.removeClass('rui-selectable-disabled'));
    }, this);
    return this;
  },

  /**
   * Checks if the given key or keys are disabled
   * NOTE: if no keys specified, then will check if all the items are disabled
   *
   * @param mixed optional key or keys to enable
   * @return Selectable this
   */
  disabled: function(keys) {
    return this.mapOrAll(keys).every('hasClass', 'rui-selectable-disabled');
  },

  /**
   * selects item(s) that refers to the given key or keys
   *
   * @param mixed key or keys
   * @return Selectable this
   */
  select: function(keys) {
    var items = this.mapEnabled(keys), selected_class = 'rui-selectable-selected';

    if (this.isSingle && items) {
      this.items().each('removeClass', selected_class);
      items = R([items[0]]);
    }

    // applying the selection limit if ncessary
    if (!this.isSingle && this.options.limit) {
      var selected = this.items().filter('hasClass', selected_class), clean = [];
      while (items.length && (selected.length + clean.length) < this.options.limit) {
        var item = items.shift();
        if (!selected.include(item)) {
          clean.push(item);
        }
      }
      items = clean;
    }

    items.compact().each(function(item) {
      this.fire('select', item.addClass(selected_class));
    }, this);

    return this;
  },

  /**
   * Unselects item(s) that refers to the given key or keys
   *
   * @param mixed key or keys
   * @return Selectable this
   */
  unselect: function(keys) {
    var prev_value = this.getValue();

    this.mapEnabled(keys).each(function(item) {
      this.fire('unselect', item.removeClass('rui-selectable-selected'));
    }, this);

    return this;
  },

  /**
   * Checks if item(s) are selected
   *
   * @param mixed key or keys
   * @return Boolean check result
   */
  selected: function(keys) {
    return this.mapEnabled(keys).every('hasClass', 'rui-selectable-selected');
  },

  /**
   * Overloading the method so it worked nicely with the single versions
   *
   * @param Element target
   * @param String optional position
   * @return Selectable this
   */
  insertTo: function(target, where) {
    this.$super.call(
      (this.isSingle ? this.container : this), target, where
    );

    return this;
  },

  /**
   * Overloading the method so that single selectables were removed
   * properly
   *
   * @return Selectable this
   */
  remove: function() {
    this.$super.call(this.isSingle ? this.container : this);
    return this;
  },

// protected

  // wrapping the events trigger to feed it with some more options
  fire: function(name, item) {
    if (item && item instanceof Element) {
      this.$super(name, {item: item, index: this.items().indexOf(item)});
    } else {
      this.$super.apply(this, arguments);
    }
    return this;
  },

  // finds out the value for the item
  itemValue: function(item) {
    var value = R([item._value, item.get('id') || item.get('val')]).compact()[0];

    return  value !== undefined ? (
      this.options.parseIds ? value.match(/\d+/) : value
    ) : this.items().indexOf(item);
  },

  // returns the list of items
  items: function() {
    return this.find('li');
  },

  // returns matching items or all of them if there's no key
  mapOrAll: function(keys) {
    var items = this.items();

    if (defined(keys)) {
      if (!isArray(keys)) { keys = [keys]; }

      items = R(keys).map(function(key) {
        var index = (isString(key) && /^\d+$/.test(key)) ? parseInt(key,10) : key, item = key;

        if (isNumber(index)) {
          item = items[index];
        } else if(isString(key)) {
          item = items.first(function(i) {
            return i.id == key || i.val == key;
          });
        }

        return item;
      }, this).compact();
    }

    return items;
  },

  // maps and filters only enabled items
  mapEnabled: function(keys) {
    return this.mapOrAll(keys).filter(function(item) {
      return !item.hasClass('rui-selectable-disabled');
    }, this);
  },

  // onmousedown callback
  _mousedown: function(event) {
    event.stop();
    var item = event.target, items = this.items();

    if (items.include(item) && !this.disabled(item)) {
      if (this.isSingle) {  // single-selects are always select
        this.select(item);
      } else if (this.selected(item)) {
        this.unselect(item);
        this._massRemove = true; // mass-removing start
      } else {
        this.select(item);
        this._massSelect = true; // mass-selection start
      }

      // mass-selection with a shift/meta key
      if ((event.shiftKey || event.metaKey) && this._prevItem) {
        var index1 = items.indexOf(this._prevItem);
        var index2 = items.indexOf(item);

        if (index1 != index2) {
          if (index1 > index2) {
            var t = index1;
            index1 = index2;
            index2 = index1;
          }

          for (var i=index1; i < index2; i++) {
            this[this._prevItem.hasClass('rui-selectable-selected') ? 'select' : 'unselect'](items[i]);
          }
        }
      }

      this._prevItem = item;
    }
  },

  // onmouseup callback
  _mouseup: function(event) {
    event.stop();
    this._massRemove = this._massSelect = false; // mass-selection stop
  },

  // mouseover callback
  _mouseover: function(event) {
    var item = event.target;
    this.fire('hover', item);

    if (!this.isSingle) {
      if (this._massSelect) {
        this.select(item);
      } else if (this._massRemove) {
        this.unselect(item);
      }
    }
  },

  // mouseout callback
  _mouseout: function(event) {
    this.fire('leave', event.target);
  },

  // mouseclick callback
  _click: function(event) {
    event.stop();
  },

  // select/unselect listener fires the onchange events
  _change: function() {
    if (''+this.value != ''+this.getValue()) {
      this.value = this.getValue();
      this.fire('change');
    }
  },

  // builds the widget programmatically
  build: function() {
    var options = this.options.options, items = R([]);

    if (isArray(options)) {
      options.each(function(option) {
        items.push(isArray(option) ? option : [option, option]);
      });
    } else {
      for (var key in options) {
        items.push([options[key], key]);
      }
    }

    items.each(function(option) {
      var item = $E('li', {val: option[1], html: option[0]});
      item._value = option[1];
      this.insert(item);
    }, this);

    return this;
  },

  // builds a container for a single-select
  buildSingle: function() {
    this.container = $E('div', {'class': 'rui-selectable-container'})
      .insert([
        this.trigger = $E('div', {'html': this.options.hCont, 'class': 'rui-selectable-handle'}),
        this.display = $E('ul', {'class': 'rui-selectable-display'})
      ])
      .onClick(R(this.toggleList).bind(this));

    if (this.parent()) {
      this.container.insertTo(this, 'instead');
    }

    this.container.insert(this);

    $(document).onClick(R(this.hideList).bind(this));

    return this
      .onSelect('showItem')
      .onSelect('hideList')
      .addClass('rui-dd-menu');
  },

  // toggles the single-selects list
  toggleList: function(event) {
    event.stop();
    return this.visible() ? this.hideList() : this.showList(event);
  },

  // shows list for the single-selects
  showList: function(event) {
    event.stop();

    $$('.rui-selectable-single').without(this).each('hide');

    var dims = this.container.dimensions(), pos = this.container.position();

    this.setStyle({
      top:  (dims.top + dims.height - pos.y - 1) + 'px',
      left: (dims.left - pos.x) + 'px',
      width: dims.width + 'px'
    }).show(this.options.fxName, {
      duration: this.options.fxDuration,
      onFinish: this.fire.bind(this, 'show', this)
    });

    if (!this.options.fxName) {
      this.fire('show', this);
    }
  },

  // hides the list for the single-selects
  hideList: function() {
    if (this.isSingle && this.visible()) {
      this.hide(this.options.fxName, {
        duration: this.options.fxDuration,
        onFinish: this.fire.bind(this, 'hide')
      });

      if (!this.options.fxName) {
        this.fire('hide');
      }
    }
  },

  // shows the item in the main view of a single-selector
  showItem: function() {
    var item = this.items().first('hasClass', 'rui-selectable-selected') || this.items().first();
    this.display.html('<li>'+(item ? item.html() : '&nbsp;')+'</li>');
  },

  // harvests options from a selectbox element
  harvestOptions: function(selectbox) {
    var options = new Function('return '+ selectbox.get('data-selectable'))() || {};

    options.multiple = selectbox._.type == 'select-multiple';
    options.options  = R([]);
    options.selected = R([]);
    options.disabled = R([]);

    $A(selectbox._.getElementsByTagName('OPTION')).each(function(option, index) {
      var html = option.innerHTML, value = option.getAttribute('value');

      options.options.push([html, value === null ? html : value]);

      if (option.selected && !selectbox._.disabled) { options.selected.push(index); }
      if (option.disabled ||  selectbox._.disabled) { options.disabled.push(index); }
    });

    if (options.selected.empty()) { options.selected = 0; }

    return options;
  }
});


/**
 * The document on-load for Selectable
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
$(document).onReady(function() {
  Selectable.rescan();
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("*.rui-dd-menu, *.rui-dd-menu li{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none} *.rui-dd-menu{display:none;position:absolute;z-index:9999;background:white;border:1px solid #BBB;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;box-shadow:#DDD .2em .2em .4em;-moz-box-shadow:#DDD .2em .2em .4em;-webkit-box-shadow:#DDD .2em .2em .4em} *.rui-dd-menu li{padding:.2em .4em;border-top:none;border-bottom:none;cursor:pointer} *.rui-dd-menu li.current{background:#DDD} *.rui-dd-menu li:hover{background:#EEE}dl.rui-dd-menu dt{padding:.3em .5em;cursor:default;font-weight:bold;font-style:italic;color:#444;background:#EEE}dl.rui-dd-menu dd li{padding-left:1.5em} *.rui-selectable, *.rui-selectable li, *.rui-selectable dt, *.rui-selectable dd, *.rui-selectable ul,div.rui-selectable-container ul.rui-selectable-display,div.rui-selectable-container ul.rui-selectable-display li{margin:0;padding:0;border:none;background:none;list-style:none} *.rui-selectable{border:1px solid #CCC;border-bottom:none;display:inline-block; *display:inline; *zoom:1;min-width:10em;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;user-select:none;-moz-user-select:none;-webkit-user-select:none} *.rui-selectable li{padding:.3em 1em;cursor:pointer;border-bottom:1px solid #CCC} *.rui-selectable li:hover{background:#EEE} *.rui-selectable li.rui-selectable-selected{font-weight:bold;background:#DDD} *.rui-selectable li.rui-selectable-disabled, *.rui-selectable li.rui-selectable-disabled:hover{background:#CCC;color:#777;cursor:default}dl.rui-selectable dt{padding:.3em .5em;cursor:default;font-weight:bold;font-style:italic;color:#444;background:#EEE;border-bottom:1px solid #CCC}dl.rui-selectable dd li{padding-left:1.5em} *.rui-selectable-single{background:#FFF;display:none} *.rui-selectable-single li{overflow:hidden}div.rui-selectable-container{border:1px solid #CCC;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;display:inline-block; *display:inline; *zoom:1; *width:10em;vertical-align:middle;min-width:10em;cursor:pointer;height:1.6em;position:relative}div.rui-selectable-container div.rui-selectable-handle{font-family:Arial;position:absolute;right:0;width:0.8em;background:#DDD;text-align:center;height:100%;line-height:0.8em;font-size:200%;color:#888;border-left:1px solid #CCC}div.rui-selectable-container:hover div.rui-selectable-handle{color:#666}div.rui-selectable-container ul.rui-selectable-display{display:block;width:auto;overflow:hidden;margin-right:2em}div.rui-selectable-container ul.rui-selectable-display li{line-height:1.6em;padding:0 .5em;overflow:hidden;width:100%;white-space:nowrap}select.rui-selectable{visibility:hidden}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Selectable;
})(document, RightJS);
