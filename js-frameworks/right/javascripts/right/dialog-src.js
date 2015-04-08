/**
 * RightJS-UI Dialog v2.2.2
 * http://rightjs.org/ui/dialog
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var Dialog = RightJS.Dialog = (function(RightJS) {
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
 * Dialog widget initialization script
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var R  = RightJS,
    $  = RightJS.$,
    $w = RightJS.$w,
    $E = RightJS.$E,
    Xhr     = RightJS.Xhr,
    Class   = RightJS.Class,
    Object  = RightJS.Object,
    Element = RightJS.Element;







/**
 * Basic dialog class
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
var Dialog = new Widget({
  extend: {
    version: '2.2.2',

    EVENTS: $w('ok cancel help expand collapse resize load'),

    Options: {
      lockScreen:  true,  // if you need to lock the scrreen
      fxDuration:  'short', // dialog appearance duration

      draggable:   true,  // sets if the user should be able to drag the dialog around
      closeable:   true,  // allow the user to close the dialog
      expandable:  false, // show the user to expand/collapse the dialog window width

      showHelp:    false, // show the 'Help' button
      showIcon:    null,  // null or some text to be show in the dialog body icon

      title:       null,  // default title to preset
      html:        null,  // html content to set on instance
      url:         null   // url address that should be loaded on instance
    },

    i18n: {
      Ok:       'Ok',
      Close:    'Close',
      Cancel:   'Cancel',
      Help:     'Help',
      Expand:   'Expand',
      Collapse: 'Collapse',

      Alert:    'Warning!',
      Confirm:  'Confirm',
      Prompt:   'Enter'
    },

    current: false,   // the current dialog reference
    dragged: false    // currently dragged dialog reference
  },

  /**
   * Basic constructor
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    this
      .$super('dialog', options)
      .append(
        this.head = new Dialog.Head(this),
        this.body = new Dialog.Body(this),
        this.foot = new Dialog.Foot(this)
      )
      .onCancel(this.hide);

    this.locker = $E('div', {'class': 'rui-screen-locker'});

    if (this.options.title) {
      this.title(this.options.title);
    }

    if (this.options.html) {
      this.html(this.options.html);
    }

    if (this.options.url) {
      this.load(this.options.url);
    }
  },

  /**
   * Shows the dialog
   *
   * @return Dialog this
   */
  show: function() {
    if (this.options.lockScreen) {
      this.locker.insertTo(document.body);
    }

    this
      .setStyle('visibility:hidden')
      .insertTo(document.body)
      .resize()
      .setStyle('visibility:visible;opacity:0');

    if (this.options.fxDuration) {
      this.morph({opacity: 1}, {
        duration: this.options.fxDuration
      });
    } else {
      this.setStyle('opacity:1');
    }

    return (Dialog.current = this);
  },

  /**
   * Hides the dialog
   *
   * @return Dialog this
   */
  hide: function() {
    this.locker.remove();
    this.remove();

    Dialog.current = false;

    return this;
  },

  /**
   * Repositions the dialog to the middle of the screen
   *
   * @param normal arguments
   * @return Dialog this
   */
  resize: function() {
    if (arguments.length) {
      this.$super.apply(this, arguments);
    }

    var size = this.size(), win_size = $(window).size();

    if (this.expanded) {
      size.x = win_size.x - 20;
      size.y = win_size.y - 10;
      this.$super.call(this, size);
    }

    this.setStyle({
      top:  (win_size.y - size.y)/2 + $(window).scrolls().y + 'px',
      left: (win_size.x - size.x - 16)/2 + 'px'
    });

    return this.fire('resize');
  },

  /**
   * Bidirectional method to work with titles
   *
   * @param String title to set
   * @return String title or Dialog this
   */
  title: function(text) {
    if (arguments.length) {
      this.head.title.html(text);
      return this;
    } else {
      return this.head.title.html();
    }
  },

  /**
   * Overloading the standard method, so that
   * all the content updates were going into the body element
   *
   * @param mixed content
   * @return Dialog this
   */
  update: function(content) {
    this.body.update(content);
    return this.resize();
  },

  /**
   * Redirecting the `html` method to work wiht the body html
   *
   * @param mixed content
   * @return Dialog this or html content of the body
   */
  html: function() {
    return arguments.length ?
      this.$super.apply(this, arguments) :
      this.body.html();
  },

  /**
   * Overloading the original method to bypass things into the body object
   *
   * @param String url
   * @param Object options
   * @return Dialog this
   */
  load: function(url, options) {
    this.show();
    this.body.load(url, options);
    return this;
  },

  /**
   * Expands a dialog screen-wide
   *
   * @return Dialog this
   */
  expand: function() {
    if (!this.expanded) {
      this._prevSize = this.size();
      this.resize({
        x: $(window).size().x - 20,
        y: $(window).size().y - 10
      });

      this.expanded = true;
      this.fire('expand');
    }

    return this;
  },

  /**
   * Collapses an expanded dialog to it's previous size
   *
   * @return Dialog this
   */
  collapse: function() {
    if (this.expanded) {
      this.expanded = false;
      this.resize(this._prevSize);
      this.fire('collapse');
    }

    return this;
  }
});

/**
 * Dialog header line element
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
Dialog.Head = new Class(Element, {

  initialize: function(dialog) {
    this.dialog  = dialog;
    this.options = dialog.options;

    this.$super('div', {'class': 'rui-dialog-head'});

    this.append(
      this.icon  = $E('div', {'class': 'icon'}),
      this.title = $E('div', {'class': 'title', 'html': '&nbsp;'}),
      this.tools = $E('div', {'class': 'tools'})
    );

    this.fsButton    = $E('div', {
      'class': 'expand', 'html': '&equiv;', 'title': Dialog.i18n.Expand
    }).onClick(function() {
      if (dialog.expanded) {
        dialog.collapse();
        this.html('&equiv;').set('title', Dialog.i18n.Expand);
      } else {
        dialog.expand();
        this.html('_').set('title', Dialog.i18n.Collapse);
      }
    });

    this.closeButton = $E('div', {
      'class': 'close',  'html': '&times;', 'title': Dialog.i18n.Close
    }).onClick(function() { dialog.fire('cancel'); });

    if (this.options.expandable) {
      this.tools.insert(this.fsButton);
    }

    if (this.options.closeable) {
      this.tools.insert(this.closeButton);
    }

    this.on({
      selectstart: function(e) { e.stop(); },
      mousedown:   this.dragStart,
      touchstart:  this.dragStart
    });

    if (!this.options.draggable) {
      this.dialog.addClass('rui-dialog-nodrag');
    }
  },

// protected

  dragStart: function(event) {
    if (this.options.draggable && !event.find('div.tools div')) {
      var dim = this.dialog.dimensions(),
          ev_pos = event.position();

      this.xDiff = dim.left - ev_pos.x;
      this.yDiff = dim.top  - ev_pos.y;
      this.maxX  = $(window).size().x - dim.width - 20;
      this.dlgStyle = this.dialog.get('style');

      Dialog.dragged = this.dialog;

      event.stop();
    }
  },

  dragMove: function(event) {
    var event_pos = event.position(),
        pos_x = event_pos.x + this.xDiff,
        pos_y = event_pos.y + this.yDiff;

    if (pos_x < 0) { pos_x = 0; }
    else if (pos_x > this.maxX) { pos_x = this.maxX; }
    if (pos_y < 0) { pos_y = 0; }

    this.dlgStyle.top  = pos_y + 'px';
    this.dlgStyle.left = pos_x + 'px';
  },

  dragStop: function(event) {
    Dialog.dragged = false;
  }
});

/**
 * Dialog body element
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Dialog.Body = new Class(Element, {

  initialize: function(dialog) {
    this.dialog  = dialog;
    this.options = dialog.options;

    this.$super('div', {'class': 'rui-dialog-body'});
    this.locker = $E('div', {'class': 'rui-dialog-body-locker'})
      .insert(new Spinner());
  },

  load: function(url, options) {
    this.insert(this.locker, 'top');

    this.xhr = new Xhr(url, Object.merge({method:'get'}, options))
      .onComplete(R(function(r) {
        this.update(r.text);
        this.dialog.resize().fire('load');
      }).bind(this))
      .send();

    return this;
  },

  update: function(content) {
    this.$super(content);

    if (this.options.showIcon) {
      this.insert('<div class="rui-dialog-body-icon">'+ this.options.showIcon + '</div>', 'top');
    }

    return this;
  }

});

/**
 * Dialog footer line element
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Dialog.Foot = new Class(Element, {

  initialize: function(dialog) {
    this.$super('div', {'class': 'rui-dialog-foot'});

    this.dialog = dialog;

    dialog.okButton     = new Button(Dialog.i18n.Ok,     {'class': 'ok'}).onClick(function() { dialog.fire('ok'); });
    dialog.helpButton   = new Button(Dialog.i18n.Help,   {'class': 'help'}).onClick(function() { dialog.fire('help'); });
    dialog.cancelButton = new Button(Dialog.i18n.Cancel, {'class': 'cancel'}).onClick(function() { dialog.fire('cancel'); });

    if (dialog.options.showHelp) {
      this.insert(dialog.helpButton);
    }

    if (dialog.options.closeable) {
      this.insert(dialog.cancelButton);
    }

    this.insert(dialog.okButton);
  }

});

/**
 * Alert specific dialog
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Dialog.Alert = new Class(Dialog, {

  initialize: function(options) {
    options = Object.merge({
      showIcon: '!',
      title:    Dialog.i18n.Alert
    }, options);

    this.$super(options);
    this.addClass('rui-dialog-alert');
    this.on('ok', 'hide');
  }
});

/**
 * Confirm specific dialog
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Dialog.Confirm = new Class(Dialog, {

  initialize: function(options) {
    options = Object.merge({
      showIcon: '?',
      title:    Dialog.i18n.Confirm
    }, options);

    this.$super(options);
    this.addClass('rui-dialog-confirm');
    this.on('ok', 'hide');
  }

});

/**
 * The prompt dialog class
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Dialog.Prompt = new Class(Dialog, {
  /**
   * prompts constructor, you can use additional options with this one
   *
   *   * `label` - the text for the input field label
   *   * `input` - the input field options (standard for Input unit)
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    options = Object.merge({
      showIcon: '&#x27A5;',
      title:    Dialog.i18n.Prompt,
      label:    Dialog.i18n.Prompt
    }, options);

    this.$super(options);
    this.addClass('rui-dialog-prompt');

    this.html([
      $E('label', {html: this.options.label}),
      this.input = new RightJS.Input(this.options.input || {})
    ]);

    if (this.input.get('type') !== 'textarea') {
      this.input.onKeydown(R(function(event) {
        if (event.keyCode === 13) {
          this.fire('ok');
        }
      }).bind(this));
    }
  },

  show: function() {
    this.$super.apply(this, arguments);
    this.input.select();
    return this;
  }

});

/**
 * Document level hooks for the dialogs
 *
 * Copyright (C) 2010-2012 Nikolay Nemshilov
 */
$(document).on({
  keydown: function(event) {
    if (event.keyCode === 27 && Dialog.current) {
      if (Dialog.current.options.closeable) {
        Dialog.current.fire('cancel');
      }
    } else if (event.keyCode === 13 && Dialog.current) {
      if (!(Dialog.current instanceof Dialog.Prompt)) {
        event.stop();
        Dialog.current.fire('ok');
      }
    }
  },

  mousemove: document_mousemove,
  touchmove: document_mousemove,

  mouseup:   document_mouseup,
  touchend:  document_mouseup
});

function document_mousemove(event) {
  if (Dialog.dragged) {
    Dialog.dragged.head.dragMove(event);
  }
}

function document_mouseup(event) {
  if (Dialog.dragged) {
    Dialog.dragged.head.dragStop(event);
  }
}



$(window).onResize(function() {
  if (Dialog.current) {
    Dialog.current.resize();
  }
});

var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("*.rui-button{display:inline-block; *display:inline; *zoom:1;height:1em;line-height:1em;margin:0;padding:.2em .5em;text-align:center;border:1px solid #CCC;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;cursor:pointer;color:#333;background-color:#FFF;user-select:none;-moz-user-select:none;-webkit-user-select:none} *.rui-button:hover{color:#111;border-color:#999;background-color:#DDD;box-shadow:#888 0 0 .1em;-moz-box-shadow:#888 0 0 .1em;-webkit-box-shadow:#888 0 0 .1em} *.rui-button:active{color:#000;border-color:#777;text-indent:1px;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none} *.rui-button-disabled, *.rui-button-disabled:hover, *.rui-button-disabled:active{color:#888;background:#DDD;border-color:#CCC;cursor:default;text-indent:0;box-shadow:none;-moz-box-shadow:none;-webkit-box-shadow:none}div.rui-spinner,div.rui-spinner div{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none;display:inline-block; *display:inline; *zoom:1;border-radius:.12em;-moz-border-radius:.12em;-webkit-border-radius:.12em}div.rui-spinner{text-align:center;white-space:nowrap;background:#EEE;border:1px solid #DDD;height:1.2em;padding:0 .2em}div.rui-spinner div{width:.4em;height:70%;background:#BBB;margin-left:1px}div.rui-spinner div:first-child{margin-left:0}div.rui-spinner div.glowing{background:#777}div.rui-screen-locker{position:fixed;top:0;left:0;width:100%;height:100%;margin:0;padding:0;background:#000;opacity:.5;filter:alpha(opacity=50);z-index:99999;cursor:default}div.rui-dialog{position:absolute;z-index:99999;background:white;margin:0;padding:0;padding-top:2.5em;padding-bottom:2.8em;border-radius:.35em;-moz-border-radius:.35em;-webkit-border-radius:.35em;border:1px solid #ccc}div.rui-dialog-body{min-width:20em;min-height:4.5em;margin:0;padding:0 1em;height:100%;overflow:auto;position:relative}div.rui-dialog-body-locker{position:absolute;z-index:9999;left:0;top:0;width:100%;height:100%;text-align:center;opacity:.6;filter:alpha(opacity=60)}div.rui-dialog-body-locker div.rui-spinner{border:none;background:none;font-size:150%;margin-top:8%}div.rui-dialog-body-icon{float:left;background:#eee;font-size:360%;font-family:Arial;border:2px solid gray;border-radius:.1em;-moz-border-radius:.1em;-webkit-border-radius:.1em;width:1em;line-height:1em;text-align:center;margin-right:.2em;margin-top:.05em;cursor:default;user-select:none;-moz-user-select:none;-webkit-user-select:none}div.rui-dialog-head{position:absolute;top:0;left:0;margin:0;padding:0;width:100%;line-height:2em;background:#ccc;border-radius:.35em;-moz-border-radius:.35em;-webkit-border-radius:.35em;border-bottom-left-radius:0;border-bottom-right-radius:0;-moz-border-radius-bottomleft:0;-moz-border-radius-bottomright:0;-webkit-border-bottom-left-radius:0;-webkit-border-bottom-right-radius:0;cursor:move;user-select:none;-moz-user-select:none;-webkit-user-select:none}div.rui-dialog-head div.icon{float:left;height:1.4em;width:1.4em;margin-left:1em;margin-top:.3em;margin-right:.3em;display:none}div.rui-dialog-head div.title{margin-left:1em;color:#444}div.rui-dialog-head div.tools{position:absolute;right:.3em;top:.3em}div.rui-dialog-head div.tools div{float:left;width:1.4em;line-height:1.4em;text-align:center;margin-left:.15em;cursor:pointer;background:#aaa;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;font-family:Verdana;opacity:.6;filter:alpha(opacity=60)}div.rui-dialog-head div.tools div:hover{opacity:1;filter:alpha(opacity=100);box-shadow:#444 0 0 .1em;-moz-box-shadow:#444 0 0 .1em;-webkit-box-shadow:#444 0 0 .1em}div.rui-dialog-head div.tools div.close:hover{background:#daa}div.rui-dialog-nodrag div.rui-dialog-head{cursor:default}div.rui-dialog-foot{position:absolute;bottom:0;left:0;width:100%;text-align:right}div.rui-dialog-foot div.rui-button{margin:.6em 1em;background:#eee;width:4em}div.rui-dialog-foot div.help{float:left}div.rui-dialog-foot div.cancel{margin-right:-.5em}div.rui-dialog-foot div.ok:hover{background-color:#ded}div.rui-dialog-foot div.cancel:hover{background-color:#ecc}div.rui-dialog-alert div.rui-dialog-foot{text-align:center}div.rui-dialog-alert div.rui-dialog-foot div.cancel{display:none}div.rui-dialog-alert div.rui-dialog-body-icon{color:brown;background:#FEE;border-color:brown}div.rui-dialog-confirm div.rui-dialog-body-icon{color:#44A;background:#EEF;border-color:#44a}div.rui-dialog-prompt div.rui-dialog-body-icon{color:#333}div.rui-dialog-prompt div.rui-dialog-body label{display:block;font-weight:bold;font-size:120%;color:#444;margin-bottom:.5em}div.rui-dialog-prompt div.rui-dialog-body input,div.rui-dialog-prompt div.rui-dialog-body textarea{border:1px solid #aaa;font-size:1em;display:block;width:16em;margin:0;padding:.2em;margin-left:4.7em;border-radius:.2em;-moz-border-radius:.2em;-webkit-border-radius:.2em;outline:none}div.rui-dialog-prompt div.rui-dialog-body textarea{width:24em;height:8em}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Dialog;
})(RightJS);
