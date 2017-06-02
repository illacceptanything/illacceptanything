/**
 * RightJS-UI Calendar v2.3.2
 * http://rightjs.org/ui/calendar
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
var Calendar = RightJS.Calendar = (function(document, parseInt, RightJS) {
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
 * Converts a number into a string with leading zeros
 *
 * @param Number number
 * @return String with zeros
 */
function zerofy(number) {
  return (number < 10 ? '0' : '') + number;
}


/**
 * The filenames to include
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */

var R          = RightJS,
    $          = RightJS.$,
    $$         = RightJS.$$,
    $w         = RightJS.$w,
    $ext       = RightJS.$ext,
    $uid       = RightJS.$uid,
    isString   = RightJS.isString,
    isArray    = RightJS.isArray,
    isFunction = RightJS.isFunction,
    Class      = RightJS.Class,
    Element    = RightJS.Element,
    Input      = RightJS.Input,
    RegExp     = RightJS.RegExp,
    Browser    = RightJS.Browser;









/**
 * The calendar widget for RightJS
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
var Calendar = new Widget({
  include: [Toggler, Assignable],

  extend: {
    version: '2.3.2',

    EVENTS: $w('show hide change done'),

    Options: {
      format:         'ISO',  // a key out of the predefined formats or a format string

      showTime:       null,   // null for automatic, or true|false to enforce
      showButtons:    false,  // show the bottom buttons

      minDate:        false,  // the minimal date available
      maxDate:        false,  // the maximal date available

      fxName:         'fade',  // set to null if you don't wanna any fx
      fxDuration:     'short', // the fx-duration

      firstDay:       1,      // 1 for Monday, 0 for Sunday
      numberOfMonths: 1,      // a number or [x, y] grid size
      timePeriod:     1,      // the timepicker minimal periods (in minutes, might be bigger than 60)

      twentyFourHour: null,   // null for automatic, or true|false to enforce
      listYears:      false,  // show/hide the years listing buttons

      hideOnPick:     false,  // hides the popup when the user changes a day

      update:         null,   // a reference to an input element to assign to
      trigger:        null,   // a reference to a trigger element that would be paired too

      highlight:      null,   // a list of dates to highlight

      cssRule:        '*[data-calendar]' // css rule for calendar related elements
    },

    Formats: {
      ISO:            '%Y-%m-%d',
      POSIX:          '%Y/%m/%d',
      EUR:            '%d-%m-%Y',
      US:             '%m/%d/%Y'
    },

    i18n: {
      Done:           'Done',
      Now:            'Now',
      NextMonth:      'Next Month',
      PrevMonth:      'Previous Month',
      NextYear:       'Next Year',
      PrevYear:       'Previous Year',

      dayNames:        $w('Sunday Monday Tuesday Wednesday Thursday Friday Saturday'),
      dayNamesShort:   $w('Sun Mon Tue Wed Thu Fri Sat'),
      dayNamesMin:     $w('Su Mo Tu We Th Fr Sa'),
      monthNames:      $w('January February March April May June July August September October November December'),
      monthNamesShort: $w('Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec')
    },

    current:   null,

    // hides all the popup calendars
    hideAll: function(that_one) {
      $$('div.rui-calendar').each(function(element) {
        if (element instanceof Calendar && element !== that_one && element.visible() && !element.inlined()) {
          element.hide();
        }
      });
    }
  },

  /**
   * Basic constructor
   *
   * @param Object options
   */
  initialize: function(options) {
    this.$super('calendar', options);
    this.addClass('rui-panel');

    options = this.options;

    this.insert([
      this.swaps = new Swaps(options),
      this.grid  = new Grid(options)
    ]);

    if (options.showTime) {
      this.insert(this.timepicker = new Timepicker(options));
    }

    if (options.showButtons) {
      this.insert(this.buttons = new Buttons(options));
    }

    this.setDate(new Date()).initEvents();
  },

  /**
   * Sets the date on the calendar
   *
   * NOTE: if it's `true` then it will change the date but
   *       won't shift the months grid (used in the days picking)
   *
   * @param Date date or String date
   * @param Boolean no-shifting mode
   * @return Calendar this
   */
  setDate: function(date, no_shift) {
    if ((date = this.parse(date))) {
      var options = this.options;

      // checking the date range constrains
      if (options.minDate && options.minDate > date) {
        date = new Date(options.minDate);
      }
      if (options.maxDate && options.maxDate < date) {
        date = new Date(options.maxDate);
        date.setDate(date.getDate() - 1);
      }

      // setting the dates grid
      this._date = no_shift ? new Date(this._date || this.date) : null;
      this.grid.setDate(this._date || date, date);

      // updating the shifters state
      if (options.minDate || options.maxDate) {
        this.swaps.setDate(date);
      }

      // updating the time-picker
      if (this.timepicker && !no_shift) {
        this.timepicker.setDate(date);
      }

      if (date != this.date) {
        this.fire('change', {date: this.date = date});
      }
    }

    return this;
  },

  /**
   * Returns the current date on the calendar
   *
   * @return Date currently selected date on the calendar
   */
  getDate: function() {
    return this.date;
  },

  /**
   * Sets the value as a string
   *
   * @param String value
   * @return Calendar this
   */
  setValue: function(value) {
    return this.setDate(value);
  },

  /**
   * Returns the value as a string
   *
   * @param String optional format
   * @return String formatted date
   */
  getValue: function(format) {
    return this.format(format);
  },

  /**
   * Inserts the calendar into the element making it inlined
   *
   * @param Element element or String element id
   * @param String optional position top/bottom/before/after/instead, 'bottom' is default
   * @return Calendar this
   */
  insertTo: function(element, position) {
    this.addClass('rui-calendar-inline');
    return this.$super(element, position);
  },

  /**
   * Marks it done
   *
   * @return Calendar this
   */
  done: function() {
    if (!this.inlined()) {
      this.hide();
    }

    this.fire('done', {date: this.date});
  },

  /**
   * Checks if the calendar is inlined
   *
   * @return boolean check
   */
  inlined: function() {
    return this.hasClass('rui-calendar-inline');
  },

// protected

  /**
   * additional options processing
   *
   * @param Object options
   * @return Calendar this
   */
  setOptions: function(user_options) {
    user_options = user_options || {};
    this.$super(user_options, $(user_options.trigger || user_options.update));

    var klass   = this.constructor, options = this.options;

    // merging the i18n tables
    options.i18n = {};

    for (var key in klass.i18n) {
      options.i18n[key] = isArray(klass.i18n[key]) ? klass.i18n[key].clone() : klass.i18n[key];
    }
    $ext(options.i18n, user_options.i18n);

    // defining the current days sequence
    options.dayNames = options.i18n.dayNamesMin;
    if (options.firstDay) {
      options.dayNames.push(options.dayNames.shift());
    }

    // the monthes table cleaning up
    if (!isArray(options.numberOfMonths)) {
      options.numberOfMonths = [options.numberOfMonths, 1];
    }

    // min/max dates preprocessing
    if (options.minDate) {
      options.minDate = this.parse(options.minDate);
    }
    if (options.maxDate) {
      options.maxDate = this.parse(options.maxDate);
      options.maxDate.setDate(options.maxDate.getDate() + 1);
    }

    // format catching up
    options.format = R(klass.Formats[options.format] || options.format).trim();

    // setting up the showTime option
    if (options.showTime === null) {
      options.showTime = options.format.search(/%[HkIl]/) > -1;
    }

    // setting up the 24-hours format
    if (options.twentyFourHour === null) {
      options.twentyFourHour = options.format.search(/%[Il]/) < 0;
    }

    // enforcing the 24 hours format if the time threshold is some weird number
    if (options.timePeriod > 60 && 12 % Math.ceil(options.timePeriod/60)) {
      options.twentyFourHour = true;
    }

    if (options.update) {
      this.assignTo(options.update, options.trigger);
    }

    if (isArray(options.highlight)) {
      options.highlight = R(options.highlight).map(function(date) {
        return isString(date) ? this.parse(date) : date;
      }, this);
    }

    return this;
  },

  /**
   * hides all the other calendars on the page
   *
   * @return Calendar this
   */
  hideOthers: function() {
    Calendar.hideAll(this);
    return this;
  }
});


/**
 * The calendar month/year swapping buttons block
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Swaps = new Class(Element, {
  /**
   * Constructor
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    this.$super('div', {'class': 'swaps'});
    this.options = options;

    var i18n = options.i18n;

    this.insert([
      this.prevMonth = new Button('&lsaquo;', {title: i18n.PrevMonth, 'class': 'prev-month'}),
      this.nextMonth = new Button('&rsaquo;', {title: i18n.NextMonth, 'class': 'next-month'})
    ]);

    if (options.listYears) {
      this.insert([
        this.prevYear = new Button('&laquo;', {title: i18n.PrevYear, 'class': 'prev-year'}),
        this.nextYear = new Button('&raquo;', {title: i18n.NextYear, 'class': 'next-year'})
      ]);
    }

    this.buttons = R([this.prevMonth, this.nextMonth, this.prevYear, this.nextYear]).compact();

    this.onClick(this.clicked);
  },

  /**
   * Changes the swapping buttons state depending on the options and the current date
   *
   * @param Date date
   * @return void
   */
  setDate: function(date) {
    var options = this.options, months_num = options.numberOfMonths[0] * options.numberOfMonths[1],
        has_prev_year = true, has_next_year = true, has_prev_month = true, has_next_month = true;

    if (options.minDate) {
      var beginning = new Date(date.getFullYear(),0,1,0,0,0);
      var min_date = new Date(options.minDate.getFullYear(),0,1,0,0,0);

      has_prev_year = beginning > min_date;

      beginning.setMonth(date.getMonth() - Math.ceil(months_num - months_num/2));
      min_date.setMonth(options.minDate.getMonth());

      has_prev_month = beginning >= min_date;
    }

    if (options.maxDate) {
      var end = new Date(date);
      var max_date = new Date(options.maxDate);
      var dates = R([end, max_date]);
      dates.each(function(date) {
        date.setDate(32);
        date.setMonth(date.getMonth() - 1);
        date.setDate(32 - date.getDate());
        date.setHours(0);
        date.setMinutes(0);
        date.setSeconds(0);
        date.setMilliseconds(0);
      });

      has_next_month = end < max_date;

      // checking the next year
      dates.each('setMonth', 0);
      has_next_year = end < max_date;
    }

    this.nextMonth[has_next_month ? 'enable':'disable']();
    this.prevMonth[has_prev_month ? 'enable':'disable']();

    if (this.nextYear) {
      this.nextYear[has_next_year ? 'enable':'disable']();
      this.prevYear[has_prev_year ? 'enable':'disable']();
    }
  },

// protected

  // handles the clicks on the
  clicked: function(event) {
    var target = event.target;
    if (target && this.buttons.include(target)) {
      if (target.enabled()) {
        this.fire(target.get('className').split(/\s+/)[0]);
      }
    }
  }
});


/**
 * Represents a single month block
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Month = new Class(Element, {
  /**
   * Constructor
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    this.$super('table', {'class': 'month'});
    this.options = options;

    // the caption (for the month name)
    this.insert(this.caption = new Element('caption'));

    // the headline for the day-names
    this.insert('<thead><tr>'+
      options.dayNames.map(function(name) {return '<th>'+ name +'</th>';}).join('') +
    '</tr></thead>');

    // the body with the day-cells
    this.days = [];

    var tbody = new Element('tbody').insertTo(this), x, y, row;

    for (y=0; y < 6; y++) {
      row = new Element('tr').insertTo(tbody);
      for (x=0; x < 7; x++) {
        this.days.push(new Element('td').insertTo(row));
      }
    }

    this.onClick(this.clicked);
  },

  /**
   * Initializes the month values by the date
   *
   * @param Date date
   * @return void
   */
  setDate: function(date, current_date) {
    // getting the number of days in the month
    date.setDate(32);
    var days_number = 32 - date.getDate();
    date.setMonth(date.getMonth()-1);

    var cur_day = Math.ceil(current_date.getTime() / 86400000),
        options = this.options, i18n = options.i18n, days = this.days;

    // resetting the first and last two weeks cells
    // because there will be some empty cells over there
    for (var i=0, len = days.length-1, one, two, tre; i < 7; i++) {
      one = days[i]._;
      two = days[len - i]._;
      tre = days[len - i - 7]._;

      one.innerHTML = two.innerHTML = tre.innerHTML = '';
      one.className = two.className = tre.className = 'blank';
    }

    // putting the actual day numbers in place
    for (var i=1, row=0, week, cell; i <= days_number; i++) {
      date.setDate(i);
      var day_num = date.getDay();

      if (options.firstDay === 1) { day_num = day_num > 0 ? day_num-1 : 6; }
      if (i === 1 || day_num === 0) {
        week = days.slice(row*7, row*7 + 7); row ++;
      }

      cell = week[day_num]._;

      if (Browser.OLD) { // IE6 has a nasty glitch with that
        cell.innerHTML = '';
        cell.appendChild(document.createTextNode(i));
      } else {
        cell.innerHTML = ''+i;
      }

      cell.className = cur_day === Math.ceil(date.getTime() / 86400000) ? 'selected' : '';

      if ((options.minDate && options.minDate > date) || (options.maxDate && options.maxDate < date)) {
        cell.className = 'disabled';
      }

      if (isArray(options.highlight)) {
        if (options.highlight.first(function(d) {
          return d.getFullYear() === date.getFullYear() &&
                 d.getMonth()    === date.getMonth()    &&
                 d.getDate()     === date.getDate()
        })) { cell.className += ' highlighted'; }
      }

      week[day_num].date = new Date(date);
    }

    // setting up the caption with the month name
    var caption = (options.listYears ?
          i18n.monthNamesShort[date.getMonth()] + ',' :
          i18n.monthNames[date.getMonth()])+
        ' '+date.getFullYear(),
        element = this.caption._;

    if (Browser.OLD) {
      element.innerHTML = '';
      element.appendChild(document.createTextNode(caption));
    } else {
      element.innerHTML = caption;
    }
  },

// protected

  /**
   * Handles clicks on the day-cells
   *
   * @param Event click event
   * @return void
   */
  clicked: function(event) {
    var target = event.target, date = target.date;

    if (target && date && !target.hasClass('disabled') && !target.hasClass('blank')) {
      target.addClass('selected');

      this.fire('date-set', {
        date:  date.getDate(),
        month: date.getMonth(),
        year:  date.getFullYear()
      });
    }
  }
});


/**
 * The calendar months grid unit
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var Grid = new Class(Element, {
  /**
   * Constructor
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    this.$super('table', {'class': 'grid'});

    this.months = [];

    var tbody = new Element('tbody').insertTo(this), month;

    for (var y=0; y < options.numberOfMonths[1]; y++) {
      var row = new Element('tr').insertTo(tbody);
      for (var x=0; x < options.numberOfMonths[0]; x++) {
        this.months.push(month = new Month(options));
        new Element('td').insertTo(row).insert(month);
      }
    }
  },

  /**
   * Sets the months to the date
   *
   * @param Date date in the middle of the grid
   * @param the current date (might be different)
   * @return void
   */
  setDate: function(date, current_date) {
    var months = this.months, months_num = months.length;

    current_date = current_date || date;

    for (var i=-Math.ceil(months_num - months_num/2)+1,j=0; i < Math.floor(months_num - months_num/2)+1; i++,j++) {
      var month_date    = new Date(date);
      month_date.setDate(1);
      month_date.setMonth(date.getMonth() + i);
      months[j].setDate(month_date, current_date);
    }
  }
});


/**
 * The time-picker block unit
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Timepicker = new Class(Element, {
  /**
   * Constructor
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    this.$super('div', {'class': 'timepicker'});
    this.options = options;

    var on_change = R(this.timeChanged).bind(this);

    this.insert([
      this.hours   = new Element('select').onChange(on_change),
      this.minutes = new Element('select').onChange(on_change)
    ]);

    var minutes_threshold = options.timePeriod < 60 ? options.timePeriod : 60;
    var hours_threshold   = options.timePeriod < 60 ? 1 : Math.ceil(options.timePeriod / 60);

    for (var i=0; i < 60; i++) {
      var caption = zerofy(i);

      if (i < 24 && i % hours_threshold == 0) {
        if (options.twentyFourHour) {
          this.hours.insert(new Element('option', {value: i, html: caption}));
        } else if (i < 12) {
          this.hours.insert(new Element('option', {value: i, html: i == 0 ? 12 : i}));
        }
      }

      if (i % minutes_threshold == 0) {
        this.minutes.insert(new Element('option', {value: i, html: caption}));
      }
    }


    // adding the meridian picker if it's a 12 am|pm picker
    if (!options.twentyFourHour) {
      this.meridian = new Element('select').onChange(on_change).insertTo(this);

      R(R(options.format).includes(/%P/) ? ['am', 'pm'] : ['AM', 'PM']).each(function(value) {
        this.meridian.insert(new Element('option', {value: value.toLowerCase(), html: value}));
      }, this);
    }
  },

  /**
   * Sets the time-picker values by the data
   *
   * @param Date date
   * @return void
   */
  setDate: function(date) {
    var options = this.options;
    var hour = options.timePeriod < 60 ? date.getHours() :
      Math.round(date.getHours()/(options.timePeriod/60)) * (options.timePeriod/60);
    var minute = Math.round(date.getMinutes() / (options.timePeriod % 60)) * options.timePeriod;

    if (this.meridian) {
      this.meridian.setValue(hour < 12 ? 'am' : 'pm');
      hour = (hour == 0 || hour == 12) ? 12 : hour > 12 ? (hour - 12) : hour;
    }

    this.hours.setValue(hour);
    this.minutes.setValue(minute);
  },

// protected

  /**
   * Handles the time-picking events
   *
   * @return void
   */
  timeChanged: function(event) {
    event.stopPropagation();

    var hours   = parseInt(this.hours.value());
    var minutes = parseInt(this.minutes.value());

    if (this.meridian) {
      if (hours == 12) {
        hours = 0;
      }
      if (this.meridian.value() == 'pm') {
        hours += 12;
      }
    }

    this.fire('time-set', {hours: hours, minutes: minutes});
  }
});


/**
 * The bottom-buttons block unit
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var Buttons = new Class(Element, {
  /**
   * Constructor
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    this.$super('div', {'class': 'buttons'});

    this.insert([
      new Button(options.i18n.Now,  {'class': 'now'}).onClick('fire', 'now-clicked'),
      new Button(options.i18n.Done, {'class': 'done'}).onClick('fire', 'done-clicked')
    ]);
  }
});


/**
 * This module handles the dates parsing/formatting processes
 *
 * To format dates and times this scripts use the GNU (C/Python/Ruby) strftime
 * function formatting principles
 *
 *   %a - The abbreviated weekday name (``Sun'')
 *   %A - The  full  weekday  name (``Sunday'')
 *   %b - The abbreviated month name (``Jan'')
 *   %B - The  full  month  name (``January'')
 *   %d - Day of the month (01..31)
 *   %e - Day of the month without leading zero (1..31)
 *   %m - Month of the year (01..12)
 *   %y - Year without a century (00..99)
 *   %Y - Year with century
 *   %H - Hour of the day, 24-hour clock (00..23)
 *   %k - Hour of the day, 24-hour clock without leading zero (0..23)
 *   %I - Hour of the day, 12-hour clock (01..12)
 *   %l - Hour of the day, 12-hour clock without leading zer (0..12)
 *   %p - Meridian indicator (``AM''  or  ``PM'')
 *   %P - Meridian indicator (``pm''  or  ``pm'')
 *   %M - Minute of the hour (00..59)
 *   %S - Second of the minute (00..60)
 *   %% - Literal ``%'' character
 *
 * Copyright (C) 2009-2010 Nikolay V. Nemshilov
 */
Calendar.include({

  /**
   * Parses out the given string based on the current date formatting
   *
   * @param String string date
   * @return Date parsed date or null if it wasn't parsed
   */
  parse: function(string) {
    var date;

    if (isString(string) && string) {
      var tpl = RegExp.escape(this.options.format);
      var holders = R(tpl.match(/%[a-z]/ig)).map('match', /[a-z]$/i).map('first').without('%');
      var re  = new RegExp('^'+tpl.replace(/%p/i, '(pm|PM|am|AM)').replace(/(%[a-z])/ig, '(.+?)')+'$');

      var match = R(string).trim().match(re);

      if (match) {
        match.shift();

        var year = null, month = null, hour = null, minute = null, second = null, meridian;

        while (match.length) {
          var value = match.shift();
          var key   = holders.shift();

          if (key.toLowerCase() == 'b') {
            month = this.options.i18n[key=='b' ? 'monthNamesShort' : 'monthNames'].indexOf(value);
          } else if (key.toLowerCase() == 'p') {
            meridian = value.toLowerCase();
          } else {
            value = parseInt(value, 10);
            switch(key) {
              case 'd':
              case 'e': date   = value; break;
              case 'm': month  = value-1; break;
              case 'y':
              case 'Y': year   = value; break;
              case 'H':
              case 'k':
              case 'I':
              case 'l': hour   = value; break;
              case 'M': minute = value; break;
              case 'S': second = value; break;
            }
          }
        }

        // converting 1..12am|pm into 0..23 hours marker
        if (meridian) {
          hour = hour == 12 ? 0 : hour;
          hour = (meridian == 'pm' ? hour + 12 : hour);
        }

        date = new Date(year, month, date, hour, minute, second);
      }
    } else if (string instanceof Date || Date.parse(string)) {
      date = new Date(string);
    }

    return (!date || isNaN(date.getTime())) ? null : date;
  },

  /**
   * Formats the current date into a string depend on the current or given format
   *
   * @param String optional format
   * @return String formatted data
   */
  format: function(format) {
    var i18n   = this.options.i18n;
    var day    = this.date.getDay();
    var month  = this.date.getMonth();
    var date   = this.date.getDate();
    var year   = this.date.getFullYear();
    var hour   = this.date.getHours();
    var minute = this.date.getMinutes();
    var second = this.date.getSeconds();

    var hour_ampm = (hour == 0 ? 12 : hour < 13 ? hour : hour - 12);

    var values    = {
      a: i18n.dayNamesShort[day],
      A: i18n.dayNames[day],
      b: i18n.monthNamesShort[month],
      B: i18n.monthNames[month],
      d: zerofy(date),
      e: ''+date,
      m: (month < 9 ? '0' : '') + (month+1),
      y: (''+year).substring(2,4),
      Y: ''+year,
      H: zerofy(hour),
      k: '' + hour,
      I: (hour > 0 && (hour < 10 || (hour > 12 && hour < 22)) ? '0' : '') + hour_ampm,
      l: '' + hour_ampm,
      p: hour < 12 ? 'AM' : 'PM',
      P: hour < 12 ? 'am' : 'pm',
      M: zerofy(minute),
      S: zerofy(second),
      '%': '%'
    };

    var result = format || this.options.format;
    for (var key in values) {
      result = result.replace('%'+key, values[key]);
    }

    return result;
  }
});


/**
 * This module handles the events connection
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
Calendar.include({

// protected

  // connects the events with handlers
  initEvents: function() {
    var shift = '_shiftDate', terminate = this._terminate;

    this.on({
      // the dates/months/etc listing events
      'prev-day':     [shift, {Date:     -1}],
      'next-day':     [shift, {Date:      1}],
      'prev-week':    [shift, {Date:     -7}],
      'next-week':    [shift, {Date:      7}],
      'prev-month':   [shift, {Month:    -1}],
      'next-month':   [shift, {Month:     1}],
      'prev-year':    [shift, {FullYear: -1}],
      'next-year':    [shift, {FullYear:  1}],

      // the date/time picking events
      'date-set':     this._changeDate,
      'time-set':     this._changeTime,

      // the bottom buttons events
      'now-clicked':  this._setNow,
      'done-clicked': this.done,

      // handling the clicks
      'click':        terminate,
      'mousedown':    terminate,
      'focus':        terminate,
      'blur':         terminate
    });
  },

  // shifts the date according to the params
  _shiftDate: function(params) {
    var date = new Date(this.date), options = this.options;

    // shifting the date according to the params
    for (var key in params) {
      date['set'+key](date['get'+key]() + params[key]);
    }

    this.setDate(date);
  },

  // changes the current date (not the time)
  _changeDate: function(event) {
    var date = new Date(this.date);

    date.setDate(event.date);
    date.setMonth(event.month);
    date.setFullYear(event.year);

    this.setDate(date, true); // <- `true` means just change the date without shifting the list

    if (this.options.hideOnPick) {
      this.done();
    }
  },

  // changes the current time (not the date)
  _changeTime: function(event) {
    var date = new Date(this.date);

    date.setHours(event.hours);
    date.setMinutes(event.minutes);

    this.setDate(date);
  },

  // resets the calendar to the current time
  _setNow: function() {
    this.setDate(new Date());
  },

  /** simply stops the event so we didn't bother the things outside of the object
   *
   * @param {Event} event
   * @return void
   * @private
   */
  _terminate: function(event) {
    event.stopPropagation(); // don't let the clicks go anywere out of the clanedar

    if (this._hide_delay) {
      this._hide_delay.cancel();
      this._hide_delay = null;
    }
  }
});


/**
 * Document level event listeners for navigation and lazy initialization
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
$(document).on({
  /**
   * Watches the focus events and dispalys the calendar
   * popups when there is a related input element
   *
   * @param Event focus-event
   * @return void
   */
  focus: function(event) {
    var target = event.target instanceof Input && event.target.get('type') == 'text' ? event.target : null;

    Calendar.hideAll();

    if (target && (target.calendar || target.match(Calendar.Options.cssRule))) {
      (target.calendar || new Calendar({update: target}))
        .setValue(target.value()).showAt(target);
    }
  },

  /**
   * Watches the input elements blur events
   * and hides shown popups
   *
   * @param Event blur-event
   * @return void
   */
  blur: function(event) {
    var target = event.target, calendar = target.calendar;

    if (calendar) {
      // we use the delay so it didn't get hidden when the user clicks the calendar itself
      calendar._hide_delay = R(function() {
        calendar.hide();
      }).delay(200);
    }
  },

  /**
   * Catches clicks on trigger elements
   *
   * @param Event click
   * @return void
   */
  click: function(event) {
    var target = (event.target instanceof Element) ? event.target : null;

    if (target && (target.calendar || target.match(Calendar.Options.cssRule))) {
      if (!(target instanceof Input) || target.get('type') != 'text') {
        event.stop();
        (target.calendar || new Calendar({trigger: target}))
          .hide(null).toggleAt(target.assignedInput);
      }
    } else if (!event.find('div.rui-calendar')){
      Calendar.hideAll();
    }
  },

  /**
   * Catching the key-downs to navigate in the currently
   * opened Calendar hover
   *
   * @param Event event
   * @return void
   */
  keydown: function(event) {
    var calendar = Calendar.current, name = ({
      27: 'hide',        // Escape
      37: 'prev-day',    // Left  Arrow
      39: 'next-day',    // Right Arrow
      38: 'prev-week',   // Up Arrow
      40: 'next-week',   // Down Arrow
      33: 'prev-month',  // Page Up
      34: 'next-month',  // Page Down
      13: 'done'         // Enter
    })[event.keyCode];

    if (name && calendar && calendar.visible()) {
      event.stop();
      if (isFunction(calendar[name])) {
        calendar[name]();
      } else {
        calendar.fire(name);
      }
    }
  }
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode(".rui-panel{margin:0;padding:.5em;position:relative;background-color:#EEE;border:1px solid #BBB;border-radius:.3em;-moz-border-radius:.3em;-webkit-border-radius:.3em;box-shadow:.15em .3em .5em #BBB;-moz-box-shadow:.15em .3em .5em #BBB;-webkit-box-shadow:.15em .3em .5em #BBB;cursor:default} *.rui-button{display:inline-block; *display:inline; *zoom:1;height:1em;line-height:1em;margin:0;padding:.2em .5em;text-align:center;border:1px solid #CCC;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;cursor:pointer;color:#333;background-color:#FFF;user-select:none;-moz-user-select:none;-webkit-user-select:none} *.rui-button:hover{color:#111;border-color:#999;background-color:#DDD;box-shadow:#888 0 0 .1em;-moz-box-shadow:#888 0 0 .1em;-webkit-box-shadow:#888 0 0 .1em} *.rui-button:active{color:#000;border-color:#777;text-indent:1px;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none} *.rui-button-disabled, *.rui-button-disabled:hover, *.rui-button-disabled:active{color:#888;background:#DDD;border-color:#CCC;cursor:default;text-indent:0;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none}div.rui-re-anchor{margin:0;padding:0;background:none;border:none;float:none;display:inline;position:absolute;z-index:9999}div.rui-calendar .swaps,div.rui-calendar .grid,div.rui-calendar .timepicker,div.rui-calendar .buttons,div.rui-calendar table,div.rui-calendar table tr,div.rui-calendar table th,div.rui-calendar table td,div.rui-calendar table tbody,div.rui-calendar table thead,div.rui-calendar table caption{background:none;border:none;width:auto;height:auto;margin:0;padding:0}div.rui-calendar-inline{position:relative;display:inline-block; *display:inline; *zoom:1;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none}div.rui-calendar .swaps{position:relative}div.rui-calendar .swaps .rui-button{position:absolute;float:left;width:1em;padding:.15em .4em}div.rui-calendar .swaps .next-month{right:0em;_right:.5em}div.rui-calendar .swaps .prev-year{left:2.05em}div.rui-calendar .swaps .next-year{right:2.05em;_right:2.52em}div.rui-calendar .grid{border-spacing:0px;border-collapse:collapse;border-size:0}div.rui-calendar .grid td{vertical-align:top;padding-left:.4em}div.rui-calendar .grid>tbody>tr>td:first-child{padding:0}div.rui-calendar .month{margin-top:.2em;border-spacing:1px;border-collapse:separate}div.rui-calendar .month caption{text-align:center}div.rui-calendar .month th{color:#666;text-align:center}div.rui-calendar .month td{text-align:right;padding:.1em .3em;background-color:#FFF;border:1px solid #CCC;cursor:pointer;color:#555;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em}div.rui-calendar .month td:hover{background-color:#CCC;border-color:#AAA;color:#000}div.rui-calendar .month td.blank{background:transparent;cursor:default;border:none}div.rui-calendar .month td.selected{background-color:#BBB;border-color:#AAA;color:#222;font-weight:bold;padding:.1em .2em}div.rui-calendar .month td.disabled{color:#888;background:#EEE;border-color:#CCC;cursor:default}div.rui-calendar .month td.highlighted{background-color:#DDD;border-color:#bbb;color:#111}div.rui-calendar .timepicker{border-top:1px solid #ccc;margin-top:.3em;padding-top:.5em;text-align:center}div.rui-calendar .timepicker select{margin:0 .4em}div.rui-calendar .buttons{position:relative;margin-top:.5em}div.rui-calendar .buttons div.rui-button{width:4em;padding:.25em .5em}div.rui-calendar .buttons .done{position:absolute;right:0em;top:0}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Calendar;
})(document, parseInt, RightJS);
