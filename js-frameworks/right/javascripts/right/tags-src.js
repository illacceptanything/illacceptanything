/**
 * RightJS-UI Tags v2.2.1
 * http://rightjs.org/ui/tags
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
var Tags = RightJS.Tags = (function(RightJS) {
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
 * The tags widget initialization script
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
var R       = RightJS,
    $       = RightJS.$,
    $w      = RightJS.$w,
    Class   = RightJS.Class,
    Input   = RightJS.Input,
    Element = RightJS.Element;





/**
 * The main unit for the Tags widget
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
var Tags = new Widget('INPUT', {
  extend: {
    version: '2.2.1',

    EVENTS: $w('add remove'),

    Options: {
      tags:         [],    // the tags list
      vertical:     false, // use a vertical tags list

      allowNew:     true,  // allow new tags to be created
      nocase:       true,  // caseinsensitive
      autocomplete: true,  // autocomplete the user's input

      separator:    ',',   // the tokens separator

      cssRule: 'input[data-tags]' // the autoinitialization css-rule
    },

    /**
     * Rescans and initializes the input elements in the area
     *
     * @param {Wrapper} optional scope
     * @return void
     */
    rescan: function(scope) {
      $(scope || document).find(Tags.Options.cssRule).each(function(input) {
        if (!(input instanceof Tags)) {
          input = new Tags(input);
        }
      });
    }
  },

  /**
   * Basic constructor
   *
   * @param {Input} element
   * @param {Object} options
   * @return void
   */
  initialize: function(element, options) {
    // trying to extract a plain list of tags
    var tags = R(R(''+ $(element).get('data-tags')).trim());

    if (tags.startsWith('[') && tags.endsWith(']')) {
      if (!options) { options = {}; }
      options.tags = new Function('return '+tags)();
    }

    this
      .$super('tags', element)
      .setOptions(options);

    if (RightJS.Browser.OLD) {
      this.setStyle({color: this.getStyle('backgroundColor')});
    }

    this.container = new Element('div', {'class': 'rui-tags'}).insertTo(this, 'after');

    this.list      = new Tags.List(this);
    this.input     = new Tags.Input(this);
    this.completer = new Tags.Completer(this);

    this.onFocus(function() { this.input.focus(); });

    // reinitializing with default values
    this.setValue(this._.value);
  },

  /**
   * Overloading the method so that it updated the visible list as well
   *
   * @param {String|Array} string tokens
   * @return {Tags} this
   */
  setValue: function(tags) {
    if (isString(tags)) {
      tags = R(tags.split(this.options.separator))
        .map('trim').reject('blank');
    }

    // merging the tags into the known list
    this.options.tags = R(this.options.tags).merge(tags);

    // repopulating the list
    this.list.setTags(tags);

    // setting the internal value
    return this.$super(tags.join(this.options.separator + ' '));
  }
});


/**
 * The tags list element custom wrapper
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
Tags.List = new Class(Element, {

  /**
   * Constructor, creates the list and places where it supposed to be
   *
   * @param {Tags} tags instance
   * @return void
   */
  initialize: function(main) {
    this.main  = main;

    this.$super('ul', {'class': 'list'});
    this.insertTo(main.container);

    if (this.main.options.vertical) {
      this.addClass('vertical');
    }

    function double_styles(name) {
      return main.getStyle(name).replace(
        /[\d\.]+/, function(m) { return parseFloat(m) * 2; }
      );
    }

    this.setStyle({
      fontSize:      main.getStyle('fontSize'),
      fontFamily:    main.getStyle('fontFamily'),
      fontWeight:    main.getStyle('fontWeight'),
      letterSpacing: main.getStyle('letterSpacing'),
      paddingTop:    double_styles('borderTopWidth'),
      paddingLeft:   double_styles('borderLeftWidth'),
      paddingRight:  double_styles('borderRightWidth'),
      paddingBottom: main.getStyle('borderBottomWidth')
    });

    // frakking Opera '0em' sizes bug fallback
    if (main.getStyle('fontSize') === '0em') {
      this.setStyle({fontSize: '1em'});
    }

    this.setWidth(main.size().x);
    this.reposition(true);

    this.onClick(this._click);
  },

  /**
   * Sets a list of tags
   *
   * @param {Array} tags
   * @return {Tags.List} this
   */
  setTags: function(tags) {
    tags.uniq().each(this.clean().addTag, this);

    return this;
  },

  /**
   * Returns a list of tags on the list
   *
   * @return {Array} of tokens
   */
  getTags: function() {
    return this.find('div.text').map('text');
  },

  /**
   * adds the tag to the list
   *
   * @param {String} tag
   * @return {Tags.List} this
   */
  addTag: function(tag) {
    if (this._allowed(tag)) {
      this
        .append(
          '<li>'+
            '<div class="text">'+ R(tag).trim() +'</div>'+
            '<div class="close">&times;</div>' +
          '</li>'
        ).reposition();

      this.main.fire('add', {tag: tag});
    }

    this.main._.value = this.getTags().join(
      this.main.options.separator + ' '
    );

    return this;
  },

  /**
   * Removes the last item from the list
   *
   * @return {Tags.List} this
   */
  removeLast: function() {
    var item = this.find('li').last();

    if (item) {
      this._remove(item);
    }

    return this;
  },

  /**
   * Adjusts the original input field size and
   * places the list right above it,
   * in case if the list will start folding
   *
   * @return {Tags.List} this
   */
  reposition: function(force) {
    var size = this.size().y, main = this.main.size().y, style;

    if (size !== main || force === true) {
      this.main.setHeight(size);

      style = this._.style;

      style.top  = '0px';
      style.left = '0px';

      size = this.position();
      main = this.main.position();

      style.top  = main.y - size.y + 'px';
      style.left = main.x - size.x + 'px';
    }

    return this;
  },

// private

  // catches the clicks on the list
  _click: function(event) {
    if (event.target.hasClass('close')) {
      this._remove(event.target.parent());
    } else {
      this.main.input.focus();
    }
  },

  // checks if the tag is allowed to be added to the list
  _allowed: function(tag) {
    var tags    = this.getTags(),
        options = this.main.options,
        casesensitive = !options.nocase;

    return !(casesensitive ? tags.include(tag) :
      tags.map('toLowerCase').include(tag.toLowerCase())
    ) && (
      options.allowNew || (
        casesensitive ? tags.include(tag) :
          options.tags.map('toLowerCase').include(tag.toLowerCase())
      )
    );
  },

  // removes an item out of the list
  _remove: function(item) {
    var tag = item.first('div.text').text();

    this.main.setValue(
      this.getTags().without(tag)
    );

    this.main.fire('remove', {tag: tag});
  }

});

/**
 * The 'fake' input field element
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
Tags.Input = new Class(Input, {

  /**
   * Constructor
   *
   * @param {Tabs} the main object
   * @return void
   */
  initialize: function(main) {
    this.main = main;
    this.list = main.list;

    this.$super({type: 'text', size: 1});
    this.onKeydown(this._keydown);
    this.onKeyup(this._keyup);
    this.onBlur(this._blur);
    this.insertTo(main.list);

    // used to dynamically measure the size of the field
    this.meter = new Element('div', {
      'class': 'meter',
      'style': {
        whiteSpace: 'nowrap',
        position:   'absolute',
        left:       '-99999em'
      }
    }).insertTo(this, 'after');
  },

  /**
   * Inserting itself into the tags list on the 'focus' call
   *
   * @return {Tags.Input} this
   */
  focus: function() {
    this.main.list.append(this, this.meter).reposition();
    return this.$super();
  },

  /**
   * Resets the input field state
   *
   * @return {Tags.Input} this
   */
  reset: function() {
    this.remove();
    this.meter.remove();
    this.list.reposition();
    this._.value = '';

    return this;
  },

// private

  _keydown: function(event) {
    if (event.keyCode === 8 && this._.value === '') {
      this.list.removeLast(); // deleting the last tag with backspace
      this.focus();
    } else if (event.keyCode === 13) {
      event.preventDefault(); // preventing the for to go off on Enter
    }
  },

  _keyup: function(event) {
    if (!R([9, 27, 37, 38, 39, 40, 13]).include(event.keyCode)) {
      if (this._.value.indexOf(this.main.options.separator) !== -1) {
        this._add();
        this.focus();
      } else {
        this._resize();
        this.main.completer.suggest(this._.value);
      }
    }
  },

  _blur: function(event) {
    if (this.main.completer.hidden() && this._.value !== '') {
      this._add();
      this.reset();
    }
  },

  // resizes the field to fit the text
  _resize: function() {
    this.meter.html(this._.value + 'xx');
    this._.style.width = this.meter.size().x + 'px';
    this.list.reposition();
  },

  // makes a tag out of the current value
  _add: function() {
    var value = this._.value.replace(this.main.options.separator, '');
    this._.value = '';

    if (!(/^\s*$/).test(value)) {
      this.list.addTag(value);
    }

    if (this.main.completer.visible()) {
      this.main.completer.hide();
    }
  }

});

/**
 * The tags completer popup menu
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
Tags.Completer = new Class(Element, {

  extend: {
    current: null // currently visible list reference
  },

  /**
   * Constructor
   *
   * @param {Tags} main object
   * @return void
   */
  initialize: function(main) {
    this.main  = main;
    this.list  = main.list;
    this.input = main.input;

    this.$super('ul', {'class': 'completer'});
    this.addClass('rui-dd-menu');
    this.insertTo(main.container);

    this.onClick(this._click);
  },

  /**
   * Starts the suggesting process
   *
   */
  suggest: function(value) {
    if (!(/^\s*$/).test(value) && this.main.options.autocomplete) {
      var tags = this._filter(this.main.options.tags, value);

      if (tags.length !== 0) {
        this.html(tags.map(function(tag) {
          return '<li>'+ tag.replace(value, '<b>'+ value + '</b>') +'</li>';
        }).join(''));

        this.picked = false;

        return this.show();
      }
    }

    return this.hide();
  },

  /**
   * Overloading the method so it appeared right below the input field
   *
   * @return {Tags.Completer} this
   */
  show: function() {
    var input  = this.input.dimensions(),
        style  = this._.style,
        pos;

    style.display = 'block';

    style.top  = '0px';
    style.left = '0px';

    pos = this.position();

    style.left = input.left - pos.x + 'px';
    style.top  = input.top  - pos.y + input.height + 'px';

    return (Tags.Completer.current = this);
  },

  /**
   * Hides the list of suggestions
   *
   * @return {Tags.Completer} this
   */
  hide: function() {
    this._.innerHTML       = '';
    this._.style.display   = 'none';

    Tags.Completer.current = null;

    return this;
  },


  /**
   * Highlights the next item on the list
   *
   * @return {Tags.Completer} this
   */
  next: function() {
    var item = this.first('.current');

    if (item)  { item = item.next(); }
    if (!item) { item = this.first(); }
    if (item)  { item.radioClass('current'); }

    return this;
  },

  /**
   * Highlights the previous item on the list
   *
   * @return {Tags.Completer} this
   */
  prev: function() {
    var item = this.first('.current');

    if (item)  { item = item.prev(); }
    if (!item) { item = this.children().last(); }
    if (item)  { item.radioClass('current'); }

    return this;
  },

  /**
   * Copies the picked item data into the input field
   * and hides the list
   *
   * @return {Tags.Completer} this
   */
  done: function() {
    var item = this.first('.current');

    if (item) {
      this.list.addTag(item.text());
      this.input.reset().focus();
    }

    return this.hide();
  },

// private

  // handles mouse clicks on the list
  _click: function(event) {
    var item = event.find('li');

    if (item) {
      item.radioClass('current');
    }

    this.done();
  },

  // finds an appropriate list of tags for the suggestion
  _filter: function(tags, value) {
    var used   = this.list.getTags(),
        nocase = this.main.options.nocase;

    if (nocase) {
      used  = used.map('toLowerCase');
      value = value.toLowerCase();
    }

    return tags.filter(function(tag) {
      var low_tag = nocase ? tag.toLowerCase() : tag;

      return low_tag.indexOf(value) !== -1 && !used.include(low_tag);
    });
  }
});

/**
 * Document - on-load hook
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
$(document).on({
  /**
   * Triggers autoinitialization when the document is loaded
   *
   * @return void
   */
  ready: function() {
    Tags.rescan();
  },

  /**
   * Handles the suggestions list navigation
   *
   * @param {Event} event
   * @return void
   */
  keydown: function(event) {
    var list = Tags.Completer.current,
        keys = {
          13: 'done', // Enter
          27: 'hide', // Escape
          38: 'prev', // Up
          40: 'next'  // Down
        };

    if (list !== null && event.keyCode in keys) {
      event.stop();
      list[keys[event.keyCode]]();
    }
  },

  /**
   * Hides the completer menu by an outer click
   *
   * @param {Event} click
   * @return void
   */
  click: function(event) {
    if (Tags.Completer.current) {
      Tags.Completer.current.hide();
    }
  }

});

var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("*.rui-dd-menu, *.rui-dd-menu li{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none} *.rui-dd-menu{display:none;position:absolute;z-index:9999;background:white;border:1px solid #BBB;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;box-shadow:#DDD .2em .2em .4em;-moz-box-shadow:#DDD .2em .2em .4em;-webkit-box-shadow:#DDD .2em .2em .4em} *.rui-dd-menu li{padding:.2em .4em;border-top:none;border-bottom:none;cursor:pointer} *.rui-dd-menu li.current{background:#DDD} *.rui-dd-menu li:hover{background:#EEE}dl.rui-dd-menu dt{padding:.3em .5em;cursor:default;font-weight:bold;font-style:italic;color:#444;background:#EEE}dl.rui-dd-menu dd li{padding-left:1.5em}div.rui-tags,div.rui-tags ul.list,div.rui-tags ul.list *{position:static;top:auto;left:auto;right:auto;bottom:auto;float:none;margin:0;padding:0;border:none;background:none;display:block}input[data-tags],input.rui-tags{color:transparent;color:rgba(0,0,0,0)}div.rui-tags{position:absolute;display:inline}div.rui-tags ul.list{position:absolute;overflow:hidden;min-height:1.3em}div.rui-tags ul.list li{display:inline-block; *display:inline; *zoom:1;position:relative;cursor:default;margin-right:.1em;margin-bottom:.1em;padding:0 .5em;padding-right:1.1em;background:#ddd;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;vertical-align:top}div.rui-tags ul.list li div.text{position:inline}div.rui-tags ul.list li div.close{margin-left:.25em;cursor:pointer;font-family:Arial;font-weight:normal;opacity:0.5;position:absolute;right:.25em;top:0.04em}div.rui-tags ul.list li div.close:hover{opacity:1}div.rui-tags ul.vertical li{display:block}div.rui-tags ul.list input{width:auto;height:auto;display:inline-block; *display:inline; *zoom:1;width:1em;outline:none;vertical-align:top;font-family:inherit;font-size:inherit;font-weight:inherit;letter-spacing:inherit}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Tags;
})(RightJS);
