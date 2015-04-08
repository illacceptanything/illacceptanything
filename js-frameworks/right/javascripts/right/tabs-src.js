/**
 * RightJS-UI Tabs v2.2.3
 * http://rightjs.org/ui/tabs
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Tabs = RightJS.Tabs = (function(document, parseInt, RightJS) {
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
 * The tabs init-script
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var R  = RightJS,
    $  = RightJS.$,
    $$ = RightJS.$$,
    $w = RightJS.$w,
    $E = RightJS.$E,
    Fx       = RightJS.Fx,
    Object   = RightJS.Object,
    Browser  = RightJS.Browser,
    isArray  = RightJS.isArray,
    isNumber = RightJS.isNumber,
    Class    = RightJS.Class,
    Element  = RightJS.Element,
    Cookie   = RightJS.Cookie;







/**
 * The basic tabs handling engine
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Tabs = new Widget('UL', {
  extend: {
    version: '2.2.3',

    EVENTS: $w('select hide load disable enable add remove move'),

    Options: {
      idPrefix:       '',      // the tab-body elements id prefix
      tabsElement:    null,    // the tabs list element reference, in case it's situated somewhere else

      resizeFx:       'both',  // 'slide', 'fade', 'both' or null for no fx
      resizeDuration: 400,     // the tab panels resize fx duration

      scrollTabs:     false,   // use the tabs list scrolling
      scrollDuration: 400,     // the tabs scrolling fx duration

      selected:       null,    // the index of the currently opened tab, by default will check url, cookies or set 0
      disabled:       null,    // list of disabled tab indexes

      closable:       false,   // set true if you want a close icon on your tabs

      loop:           false,   // put a delay in ms to make it autostart the slideshow loop
      loopPause:      true,    // make the loop get paused when user hovers the tabs with mouse

      url:            false,   // a common remote tabs url template, should have the %{id} placeholder
      cache:          false,   // marker if the remote tabs should be cached

      Xhr:            null,    // the xhr addtional options
      Cookie:         null     // set the cookie options if you'd like to keep the last selected tab index in cookies
    },

    // scans and automatically intializes the tabs
    rescan: function(scope) {
      $(scope || document).find('.rui-tabs,*[data-tabs]').each(function(element) {
        element = element instanceof Tabs ? element : new Tabs(element);
      });
    }
  },

  /**
   * The basic constructor
   *
   * @param element or id
   * @param Object options
   */
  initialize: function(element, options) {
    this
      .$super('tabs', element)
      .setOptions(options)
      .addClass('rui-tabs');

    this.isHarmonica = this._.tagName === 'DL';
    this.isCarousel  = this.hasClass('rui-tabs-carousel');
    this.isSimple    = !this.isHarmonica && !this.isCarousel;

    this
      .findTabs()
      .initScrolls()
      .findCurrent()
      .setStyle('visibility:visible');

    if (this.options.disabled) {
      this.disable(this.options.disabled);
    }

    if (this.options.loop) {
      this.startLoop();
    }
  },

  /**
   * Shows the given tab
   *
   * @param integer tab index or a Tabs.Tab instance
   * @return Tabs this
   */
  select: function(tab) {
    return this.callTab(tab, 'select');
  },

  /**
   * Disables the given tab
   *
   * @param integer tab index or a Tabs.Tab instance or a list of them
   * @return Tabs this
   */
  disable: function(tab) {
    return this.callTab(tab, 'disable');
  },

  /**
   * Enables the given tab
   *
   * @param integer tab index or a Tabs.Tab instance or a list of them
   * @return Tabs this
   */
  enable: function(tab) {
    return this.callTab(tab, 'enable');
  },

  /**
   * Returns the reference to the currently opened tab
   *
   * @return Tab tab or undefined
   */
  current: function() {
    return this.tabs.first('current');
  },

  /**
   * Returns the list of enabled tabs
   *
   * @return Array of enabled tabs
   */
  enabled: function() {
    return this.tabs.filter('enabled');
  },

// protected

  // calls the tab (or tabs) method
  callTab: function(tabs, method) {
    R(isArray(tabs) ? tabs : [tabs]).each(function(tab) {
      if (isNumber(tab)) { tab = this.tabs[tab]; }
      if (tab && tab instanceof Tab) {
        tab[method]();
      }
    }, this);

    return this;
  },

  // finds and interconnects the tabs
  findTabs: function() {
    this.tabsList = this.isHarmonica ? this :
      $(this.options.tabsElement) || this.first('.rui-tabs-list') ||
        (this.first('UL') || $E('UL').insertTo(this)).addClass('rui-tabs-list');

    this.tabs = R([]);

    this.tabsList.children(this.isHarmonica ? 'dt' : null).map(function(node) {
      this.tabs.push(new Tab(node, this));
    }, this);

    // removing the whitespaces so the didn't screw with the margins
    for (var i=0, list = this.tabsList.get('childNodes'); i < list.length; i++) {
      if (list[i].nodeType == 3) { this.tabsList._.removeChild(list[i]); }
    }

    return this;
  }
});


/**
 * A single tab handling object
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Tab = Tabs.Tab = new Class(Element, {
  extend: {
    autoId: 0
  },

  /**
   * Constructor
   *
   * @param Element the tab's element
   * @param Tabs the main element
   * @return void
   */
  initialize: function(element, main) {
    this.$super(element._);
    this.addClass('rui-tabs-tab');

    this.main  = main;
    this.link  = this.first('a');
    this.id    = this.link.get('href').split('#')[1] || Tab.autoId++;
    this.panel = new Panel(this.findPanel(), this);

    if (this.current()) {
      this.select();
    }

    // adding the 'close' icon onto the tab if needed
    if (main.options.closable) {
      this.link.insert($E('div', {
        'class': 'rui-tabs-tab-close-icon', 'html': '&times;'
      }).onClick(R(this.remove).bind(this)));
    }

    this.onClick(this._clicked);
  },

  select: function() {
    if (this.enabled()) {
      var prev_tab = this.main.current();
      if (prev_tab) {
        prev_tab.removeClass('rui-tabs-current').fire('hide');
      }

      this.addClass('rui-tabs-current');
      this.main.scrollToTab(this);
      this.panel.show();
    }

    return this.fire('select');
  },

  disable: function() {
    return this.addClass('rui-tabs-disabled').fire('disable');
  },

  enable: function() {
    return this.removeClass('rui-tabs-disabled').fire('enable');
  },

  disabled: function() {
    return !this.enabled();
  },

  enabled: function() {
    return !this.hasClass('rui-tabs-disabled');
  },

  current: function() {
    return this.hasClass('rui-tabs-current');
  },

  remove: function(event) {
    if (event) { event.stop(); }

    // switching to the next available sibling
    if (this.current()) {
      var enabled = this.main.enabled();
      var sibling = enabled[enabled.indexOf(this) + 1] || enabled[enabled.indexOf(this)-1];

      if (sibling) {
        sibling.select();
      }
    }

    // removing the tab out of the list
    this.main.tabs.splice(this.main.tabs.indexOf(this), 1);
    this.panel.remove();

    var parent = this.parent();
    this.fire('beforeremove');
    this.$super();
    parent.fire('remove', {target: this});

    return this;
  },

// protected

  // handles the clicks on the tabs
  _clicked: function(event) {
    event.stop();
    return this.select();
  },

  // searches for a panel for the tab
  findPanel: function() {
    var main = this.main, panel_id = main.options.idPrefix + this.id, panel;

    if (main.isHarmonica) {
      var next = this.next();
      panel = (next && next._.tagName === 'DD') ? next : $E('DD').insertTo(this, 'after');
    } else {
      panel = $(panel_id) || $E(main._.tagName === 'UL' ? 'LI' : 'DIV').insertTo(main);
    }

    return panel.set('id', panel_id);
  },

  // returns the tab width, used for the scrolling calculations
  width: function() {
    var next = this.next();

    if (next) {
      return next.position().x - this.position().x;
    } else {
      return this.size().x + 1;
    }
  }

});


/**
 * The tab panels behavior logic
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Panel = Tabs.Panel = new Class(Element, {

  /**
   * Basic constructor
   *
   * @param Element panel-element
   * @param Tab the tab object
   * @return void
   */
  initialize: function(element, tab) {
    this.$super(element._);
    this.addClass('rui-tabs-panel');

    this.tab = tab;
    this.id  = this.get('id');
  },

  // shows the panel
  show: function() {
    return this.resizing(function() {
      this.tab.main.tabs.each(function(tab) {
        tab.panel[
          tab.panel === this ? 'addClass' : 'removeClass'
        ]('rui-tabs-current');
      }, this);
    });
  },

  // updates the panel content
  update: function(content) {
    // don't use resize if it's some other hidden tab was loaded asynch
    if (this.tab.current()) {
      this.resizing(function() {
        Element.prototype.update.call(this, content||'');
      });
    } else {
      this.$super(content||'');
    }

    return this;
  },

  // locks the panel with a spinner locker
  lock: function() {
    this.insert(this.locker(), 'top');
  },

// protected

  resizing: function(callback) {
    var controller = this.tab.main;

    if (controller.__working) { return this.resizing.bind(this, callback).delay(100); }

    var options    = controller.options;
    var prev_panel = controller.tabs.map('panel').first('hasClass', 'rui-tabs-current');
    var this_panel = this;
    var swapping   = prev_panel !== this_panel;
    var loading    = this.first('div.rui-tabs-panel-locker');

    // sometimes it looses the parent on remote tabs
    if (this_panel.parent().hasClass('rui-tabs-resizer')) {
      this_panel.insertTo(prev_panel.parent());
    }

    if (options.resizeFx && RightJS.Fx && prev_panel && (swapping || loading)) {
      controller.__working = true;
      var unlock = function() { controller.__working = false; };

      // calculating the visual effects durations
      var fx_name  = (options.resizeFx === 'both' && loading) ? 'slide' : options.resizeFx;
      var duration = options.resizeDuration; duration = Fx.Durations[duration] || duration;
      var resize_duration = fx_name === 'fade' ? 0 : fx_name === 'slide' ? duration : duration / 2;
      var fade_duration   = duration - resize_duration;

      if (fx_name !== 'slide') {
        this_panel.setStyle({opacity: 0});
      }

      // saving the previous sizes
      var prev_panel_height = (controller.isHarmonica && swapping) ? 0 : prev_panel.size().y;

      // applying the changes
      callback.call(this);

      // getting the new size
      var new_panel_height  = this_panel.size().y;
      var fx_wrapper = null;
      var hide_wrapper = null;
      var prev_back = null;

      if (fx_name !== 'fade' && prev_panel_height !== new_panel_height) {
        // preserving the whole element size so it didn't jump when we are tossing the tabs around
        controller._.style.height = controller.size().y + 'px';

        // wrapping the element with an overflowed element to visualize the resize
        fx_wrapper = $E('div', {
          'class': 'rui-tabs-resizer',
          'style': 'height: '+ prev_panel_height + 'px'
        });

        // in case of harmonica nicely hidding the previous panel
        if (controller.isHarmonica && swapping) {
          prev_panel.addClass('rui-tabs-current');
          hide_wrapper = $E('div', {'class': 'rui-tabs-resizer'});
          hide_wrapper._.style.height = prev_panel.size().y + 'px';
          prev_back = function() {
            hide_wrapper.replace(prev_panel.removeClass('rui-tabs-current'));
          };
          prev_panel.wrap(hide_wrapper);

          fx_wrapper._.style.height = '0px';
        }

        this_panel.wrap(fx_wrapper);

        // getting back the auto-size so we could resize it
        controller._.style.height = 'auto';

      } else {
        // removing the resize duration out of the equasion
        rezise_duration = 0;
        duration = fade_duration;
      }

      var counter = 0;
      var set_back = function() {
        if (fx_wrapper) {
          if (fx_name == 'both' && !counter) {
            return counter ++;
          }

          fx_wrapper.replace(this_panel);
        }

        unlock();
      };

      if (hide_wrapper) {
        hide_wrapper.morph({height: '0px'},
          {duration: resize_duration, onFinish: prev_back});
      }

      if (fx_wrapper) {
        fx_wrapper.morph({height: new_panel_height + 'px'},
          {duration: resize_duration, onFinish: set_back});
      }

      if (fx_name !== 'slide') {
        this_panel.morph.bind(this_panel, {opacity: 1},
          {duration: fade_duration, onFinish: set_back}
            ).delay(resize_duration);
      }

      if (!fx_wrapper && fx_name === 'slide') {
        set_back();
      }

    } else {
      callback.call(this);
    }

    return this;
  },

  // builds the locker element
  locker: function() {
    return this._locker || (this._locker =
      $E('div', {'class': 'rui-tabs-panel-locker'}).insert(new Spinner(5))
    );
  }
});


/**
 * Contains the tabs scrolling functionality
 *
 * NOTE: different types of tabs have different scrolling behavior
 *       simple tabs just scroll the tabs line without actually picking
 *       any tab. But the carousel tabs scrolls to the next/previous
 *       tabs on the list.
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
Tabs.include({
  /**
   * Shows the next tab
   *
   * @return Tabs this
   */
  next: function() {
    return this.pickTab(+1);
  },

  /**
   * Shows the preveious tab
   *
   * @return Tabs this
   */
  prev: function() {
    return this.pickTab(-1);
  },

  /**
   * Scrolls the tabs to the left
   *
   * @return Tabs this
   */
  scrollLeft: function() {
    if (!this.prevButton.hasClass('rui-tabs-scroller-disabled')) {
      this[this.isCarousel ? 'prev' : 'justScroll'](+0.6);
    }
    return this;
  },

  /**
   * Scrolls the tabs to the right
   *
   * @return Tabs this
   */
  scrollRight: function() {
    if (!this.nextButton.hasClass('rui-tabs-scroller-disabled')) {
      this[this.isCarousel ? 'next' : 'justScroll'](-0.6);
    }
    return this;
  },

// protected

  // overloading the init script to add the scrollbar support
  initScrolls: function() {
    if ((this.scrollable = (this.options.scrollTabs || this.isCarousel))) {
      this.buildScroller();
    }

    return this;
  },

  // builds the tabs scroller block
  buildScroller: function() {
    if (!(
      (this.prevButton = this.first('.rui-tabs-scroller-prev')) &&
      (this.nextButton = this.first('.rui-tabs-scroller-next'))
    )) {
      this.prevButton = $E('div', {'class': 'rui-tabs-scroller-prev', 'html': '&laquo;'});
      this.nextButton = $E('div', {'class': 'rui-tabs-scroller-next', 'html': '&raquo;'});

      // using a dummy element to insert the scroller in place of the tabs list
      $E('div').insertTo(this.tabsList, 'before')
        .replace(
          $E('div', {'class': 'rui-tabs-scroller'}).insert([
            this.prevButton, this.nextButton, this.scroller = $E('div', {
              'class': 'rui-tabs-scroller-body'
            }).insert(this.tabsList)
          ])
        ).remove();
    }

    this.prevButton.onClick(R(this.scrollLeft).bind(this));
    this.nextButton.onClick(R(this.scrollRight).bind(this));
  },

  // picks the next/prev non-disabled available tab
  pickTab: function(pos) {
    var current = this.current();
    if (current && current.enabled()) {
      var enabled_tabs = this.enabled();
      var tab = enabled_tabs[enabled_tabs.indexOf(current) + pos];
      if (tab) { tab.select(); }
    }
  },

  // scrolls the tabs line to make the tab visible
  scrollToTab: function(tab) {
    if (this.scroller) {
      // calculating the previous tabs widths
      var tabs_width      = 0;
      for (var i=0; i < this.tabs.length; i++) {
        tabs_width += this.tabs[i].width();
        if (this.tabs[i] === tab) { break; }
      }

      // calculating the scroll (the carousel tabs should be centralized)
      var available_width = this.scroller.size().x;
      var scroll = (this.isCarousel ? (available_width/2 + tab.width()/2) : available_width) - tabs_width;

      // check if the tab doesn't need to be scrolled
      if (!this.isCarousel) {
        var current_scroll  = parseInt(this.tabsList.getStyle('left') || 0, 10);

        if (scroll >= current_scroll && scroll < (current_scroll + available_width - tab.width())) {
          scroll = current_scroll;
        } else if (current_scroll > -tabs_width && current_scroll <= (tab.width() - tabs_width)) {
          scroll = tab.width() - tabs_width;
        }
      }

      this.scrollTo(scroll);
    }
  },

  // just scrolls the scrollable area onto the given number of scrollable area widths
  justScroll: function(size) {
    if (!this.scroller) { return this; }
    var current_scroll  = parseInt(this.tabsList.getStyle('left') || 0, 10);
    var available_width = this.scroller.size().x;

    this.scrollTo(current_scroll + available_width * size);
  },

  // scrolls the tabs list to the position
  scrollTo: function(scroll) {
    // checking the constraints
    var available_width = this.scroller.size().x;
    var overall_width   = this.tabs.map('width').sum();

    if (scroll < (available_width - overall_width)) {
      scroll = available_width - overall_width;
    }
    if (scroll > 0) { scroll = 0; }

    // applying the scroll
    this.tabsList.morph({left: scroll+'px'}, {duration: this.options.scrollDuration});

    this.checkScrollButtons(overall_width, available_width, scroll);
  },

  // checks the scroll buttons
  checkScrollButtons: function(overall_width, available_width, scroll) {
    var has_prev = false, has_next = false;

    if (this.isCarousel) {
      var enabled = this.enabled();
      var current = enabled.first('current');

      if (current) {
        var index = enabled.indexOf(current);

        has_prev = index > 0;
        has_next = index < enabled.length - 1;
      }
    } else {
      has_prev = scroll !== 0;
      has_next = scroll > (available_width - overall_width);
    }

    this.prevButton[has_prev ? 'removeClass' : 'addClass']('rui-tabs-scroller-disabled');
    this.nextButton[has_next ? 'removeClass' : 'addClass']('rui-tabs-scroller-disabled');
  }

});


/**
 * This module handles the current tab state saving/restoring processes
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
function get_cookie_indexes() {
  return R(RightJS.Cookie ? (Cookie.get('right-tabs-indexes') || '').split(',') : []);
}

function save_tab_in_cookies(options, tabs, event) {
  if (RightJS.Cookie) {
    var indexes = get_cookie_indexes();
    indexes = indexes.without.apply(indexes, tabs.map('id'));
    indexes.push(event.target.id);
    Cookie.set('right-tabs-indexes', indexes.uniq().join(','), options);
  }
}

Tabs.include({

// protected

  // searches and activates the current tab
  findCurrent: function() {
    var enabled = this.enabled(), current = (
      this.tabs[this.options.selected] ||
      this.tabs[this.urlIndex()]       ||
      this.tabs[this.cookieIndex()]    ||
      enabled.first('current')         ||
      enabled[0]
    );

    if (current) {
      current.select();
    }

    // initializing the cookies storage if set
    if (this.options.Cookie) {
      this.onSelect(R(save_tab_in_cookies).curry(this.options.Cookie, this.tabs));
    }

    return this;
  },

  // tries to find the current tab index in the url hash
  urlIndex: function() {
    var index = -1, id = document.location.href.split('#')[1];

    if (id) {
      for (var i=0; i < this.tabs.length; i++) {
        if (this.tabs[i].id == id) {
          index = i;
          break;
        }
      }
    }

    return index;
  },

  // tries to find the current tab index in the cookies storage
  cookieIndex: function() {
    var index = -1;

    if (this.options.Cookie) {
      var indexes = get_cookie_indexes();
      for (var i=0; i < this.tabs.length; i++) {
        if (indexes.include(this.tabs[i].id)) {
          index = i;
          break;
        }
      }
    }

    return index;
  }

});


/**
 * This module handles the tabs cration and removing processes
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
Tabs.include({
  /**
   * Creates a new tab
   *
   * USAGE:
   *   With the #add method you have to specify the tab title
   *   optional content (possibly empty or null) and some options
   *   The options might have the following keys
   *
   *     * id - the tab/panel id (will use the idPrefix option for the panels)
   *     * url - a remote tab content address
   *     * position - an integer position of the tab in the stack
   *
   * @param String title
   * @param mixed content
   * @param Object options
   * @return Tabs this
   */
  add: function(title, content, options) {
    options = options || {};

    // creating the new tab element
    var element = $E(this.isHarmonica ? 'dt' : 'li').insert(
      $E('a', {html: title, href: options.url || '#'+(options.id||'')}
    )).insertTo(this.tabsList);

    // creating the actual tab instance
    var tab = new Tab(element, this);
    tab.panel.update(content||'');
    this.tabs.push(tab);
    tab.fire('add');

    // moving the tab in place if asked
    if ('position' in options) {
      this.move(tab, options.position);
    }

    return this;
  },

  /**
   * Moves the given tab to the given position
   *
   * NOTE if the position is not within the tabs range then it will do nothing
   *
   * @param mixed tab index or a tab instance
   * @param Integer position
   * @return Tabs this
   */
  move: function(tab, position) {
    tab = this.tabs[tab] || tab;

    if (this.tabs[position] && this.tabs[position] !== tab) {
      // moving the tab element
      this.tabs[position].insert(tab, (position === this.tabs.length-1) ? 'after' : 'before');

      // inserting the panel after the tab if it's a harmonica
      if (this.isHarmonica) {
        tab.insert(tab.panel, 'after');
      }

      // moving the tab in the registry
      this.tabs.splice(this.tabs.indexOf(tab), 1);
      this.tabs.splice(position, 0, tab);

      tab.fire('move', {index: position});
    }

    return this;
  },

  /**
   * Removes the given tab
   *
   * @param integer tab index or a Tabs.Tab instance or a list of them
   * @return Tabs this
   */
  remove: function(tab) {
    return arguments.length === 0 ? this.$super() : this.callTab(tab, 'remove');
  }

});


/**
 * This module contains the remote tabs loading logic
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
var old_select = Tab.prototype.select;

Tab.include({

  // wrapping the original mehtod, to catch the remote requests
  select: function() {
    if (this.dogPiling(arguments)) { return this; }

    var result  = old_select.apply(this, arguments);
    var url     = R(this.link.get('href'));
    var options = this.main.options;

    // building the url
    if (url.includes('#')) {
      url = options.url ? options.url.replace('%{id}', url.split('#')[1]) : null;
    }

    // if there is an actual url and no ongoing request or a cache, starting the request
    if (url && !this.request && !(options.cache || this.cache)) {
      this.panel.lock();

      try { // basically that's for the development tests, so the IE browsers didn't get screwed on the test page

        this.request = new RightJS.Xhr(url, Object.merge({method: 'get'}, options.Xhr))
          .onComplete(R(function(response) {
            if (this.main.__working) {
              return arguments.callee.bind(this, response).delay(100);
            }

            this.panel.update(response.text);

            this.request = null; // removing the request marker so it could be rerun
            if (options.cache) {
              this.cache = true;
            }

            this.fire('load');
          }).bind(this)
        ).send();

      } catch(e) { if (!Browser.OLD) { throw(e); } }
    }

    return result;
  },

// protected

  dogPiling: function(args) {
    if (this.main.__working) {
      if (this.main.__timeout) {
        this.main.__timeout.cancel();
      }

      this.main.__timeout = R(function(args) {
        this.select.apply(this, args);
      }).bind(this, args).delay(100);

      return true;
    }

    return (this.main.__timeout = null);
  }

});


/**
 * This module handles the slide-show loop feature for the Tabs
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
Tabs.include({

  /**
   * Starts the slideshow loop
   *
   * @param Number optional delay in ms
   * @return Tabs this
   */
  startLoop: function(delay) {
    if (!delay && !this.options.loop) { return this; }

    // attaching the loop pause feature
    if (this.options.loopPause) {
      this._stopLoop  = this._stopLoop  || R(this.stopLoop).bind(this, true);
      this._startLoop = this._startLoop || R(this.startLoop).bind(this, delay);

      this.forgetHovers().on({
        mouseover: this._stopLoop,
        mouseout:  this._startLoop
      });
    }

    if (this.timer) { this.timer.stop(); }

    this.timer = R(function() {
      var enabled = this.enabled();
      var current = this.current();
      var next    = enabled[enabled.indexOf(current)+1];

      this.select(next || enabled.first());

    }).bind(this).periodical(this.options.loop || delay);

    return this;
  },

  /**
   * Stops the slideshow loop
   *
   * @return Tabs this
   */
  stopLoop: function(event, pause) {
    if (this.timer) {
      this.timer.stop();
      this.timer = null;
    }
    if (!pause && this._startLoop) {
      this.forgetHovers();
    }
  },

// private
  forgetHovers: function() {
    return this
      .stopObserving('mouseover', this._stopLoop)
      .stopObserving('mouseout', this._startLoop);
  }


});


/**
 * The document level hooks for the tabs-egnine
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
$(document).onReady(function() {
  Tabs.rescan();
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("div.rui-spinner,div.rui-spinner div{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none;display:inline-block; *display:inline; *zoom:1;border-radius:.12em;-moz-border-radius:.12em;-webkit-border-radius:.12em}div.rui-spinner{text-align:center;white-space:nowrap;background:#EEE;border:1px solid #DDD;height:1.2em;padding:0 .2em}div.rui-spinner div{width:.4em;height:70%;background:#BBB;margin-left:1px}div.rui-spinner div:first-child{margin-left:0}div.rui-spinner div.glowing{background:#777}.rui-tabs,.rui-tabs-list,.rui-tabs-tab,.rui-tabs-panel,.rui-tabs-scroll-left,.rui-tabs-scroll-right,.rui-tabs-scroll-body,.rui-tabs-panel-locker,.rui-tabs-resizer{margin:0;padding:0;background:none;border:none;list-style:none;display:block;width:auto;height:auto}.rui-tabs{display:block;visibility:hidden;border-bottom:1px solid #CCC}.rui-tabs-resizer{overflow:hidden}.rui-tabs-list{display:block;position:relative;padding:0 .5em;border-bottom:1px solid #CCC;white-space:nowrap}.rui-tabs-list .rui-tabs-tab,.rui-tabs-tab *,.rui-tabs-tab *:hover{display:inline-block; *display:inline; *zoom:1;cursor:pointer;text-decoration:none;vertical-align:center}.rui-tabs-list .rui-tabs-tab{vertical-align:bottom;margin-right:.1em}.rui-tabs-tab a{outline:none;position:relative;border:1px solid #CCC;background:#DDD;color:#444;padding:.3em 1em;border-radius:.3em;-moz-border-radius:.3em;-webkit-border-radius:.3em;border-bottom:none;border-bottom-left-radius:0;border-bottom-right-radius:0;-moz-border-radius-bottomleft:0;-moz-border-radius-bottomright:0;-webkit-border-bottom-left-radius:0;-webkit-border-bottom-right-radius:0}.rui-tabs-tab a:hover{border-color:#CCC;background:#EEE}.rui-tabs-list .rui-tabs-current a,.rui-tabs-list .rui-tabs-current a:hover{font-weight:bold;color:#000;background:#FFF;border-bottom:1px solid #FFF;border-top-width:2px;padding-top:.34em;padding-bottom:.34em;top:1px}.rui-tabs-tab a img{border:none;opacity:.6;filter:alpha(opacity=60)}.rui-tabs-tab a:hover img,.rui-tabs-list .rui-tabs-current a img{opacity:1;filter:alpha(opacity=100)}.rui-tabs-disabled a,.rui-tabs-disabled a:hover{background:#EEE;border-color:#DDD;color:#AAA;cursor:default}.rui-tabs-disabled a img,.rui-tabs-disabled a:hover img{opacity:.5;filter:alpha(opacity=50)}.rui-tabs-tab-close-icon{display:inline-block; *display:inline; *zoom:1;margin-right:-0.5em;margin-left:0.5em;cursor:pointer;opacity:0.5;filter:alpha(opacity=50)}.rui-tabs-tab-close-icon:hover{opacity:1;filter:alpha(opacity=100);color:#B00;text-shadow:#888 .15em .15em .2em}.rui-tabs-panel{display:none;position:relative;min-height:4em;padding:.5em 0}.rui-tabs-current{display:block}.rui-tabs-scroller{position:relative;padding:0 1.4em}.rui-tabs-scroller-prev,.rui-tabs-scroller-next{width:1.1em;text-align:center;background:#EEE;color:#666;cursor:pointer;border:1px solid #CCC;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;position:absolute;bottom:0px;left:0px;padding:0.3em 0;user-select:none;-moz-user-select:none;-webkit-user-select:none}.rui-tabs-scroller-prev:hover,.rui-tabs-scroller-next:hover{color:#000;background:#DDD;border-color:#AAA}.rui-tabs-scroller-prev:active,.rui-tabs-scroller-next:active{background:#eee;border-color:#ccc}.rui-tabs-scroller-next{left:auto;right:0px}.rui-tabs-scroller-disabled,.rui-tabs-scroller-disabled:hover{cursor:default;background:#DDD;border-color:#DDD;color:#AAA}.rui-tabs-scroller-body{overflow:hidden;width:100%;position:relative}.rui-tabs-scroller .rui-tabs-list{padding-left:0;padding-right:0;width:9999em;z-index:10}.rui-tabs-panel-locker{position:absolute;top:0px;left:0px;opacity:0.5;filter:alpha(opacity=50);background:#CCC;width:100%;height:100%;text-align:center}.rui-tabs-panel-locker .rui-spinner{position:absolute;left:44%;top:44%;background:none;border:none;height:2em}.rui-tabs-panel-locker .rui-spinner div{background:#666;width:.65em;margin-left:.15em}.rui-tabs-panel-locker .rui-spinner div.glowing{background:#000}.rui-tabs-carousel .rui-tabs-list{border:none}.rui-tabs-carousel .rui-tabs-tab a,.rui-tabs-carousel .rui-tabs-scroller .rui-tabs-scroller-prev,.rui-tabs-carousel .rui-tabs-scroller .rui-tabs-scroller-next{height:6em;line-height:6em;padding:0;border-bottom:1px solid #ccc;border-radius:.25em;-moz-border-radius:.25em;-webkit-border-radius:.25em}.rui-tabs-carousel .rui-tabs-tab{margin-right:3px}.rui-tabs-carousel .rui-tabs-tab a img{border:1px solid #CCC;vertical-align:middle;margin:.4em;padding:0;border-radius:0;-moz-border-radius:0;-webkit-border-radius:0}.rui-tabs-carousel .rui-tabs-list .rui-tabs-current a{border-width:1px;border-color:#AAA;padding:0;top:auto}.rui-tabs-carousel .rui-tabs-list .rui-tabs-current a img{border-color:#bbb}.rui-tabs-carousel .rui-tabs-panel{text-align:center}dl.rui-tabs{border:none}dt.rui-tabs-tab,dt.rui-tabs-tab a,dt.rui-tabs-tab a:hover{display:block;float:none}dt.rui-tabs-tab a,dt.rui-tabs-tab a:hover{padding:.2em 1em;border:1px solid #ccc;border-radius:.25em;-moz-border-radius:.3em;-webkit-border-radius:.3em}dl.rui-tabs dt.rui-tabs-current a{background:#EEE;border-bottom-left-radius:0;border-bottom-right-radius:0;-moz-border-radius-bottomleft:0;-moz-border-radius-bottomright:0;-webkit-border-bottom-left-radius:0;-webkit-border-bottom-right-radius:0}dl.rui-tabs dd.rui-tabs-current+dt.rui-tabs-tab a{border-top-left-radius:0;border-top-right-radius:0;-moz-border-radius-topleft:0;-moz-border-radius-topright:0;-webkit-border-top-left-radius:0;-webkit-border-top-right-radius:0}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Tabs;
})(document, parseInt, RightJS);
