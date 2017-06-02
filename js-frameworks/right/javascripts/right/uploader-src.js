/**
 * RightJS-UI Uploader v2.2.1
 * http://rightjs.org/ui/uploader
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Uploader = RightJS.Uploader = (function(RightJS) {
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
 * The uploader initialization script
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var R      = RightJS,
    $      = RightJS.$,
    $w     = RightJS.$w,
    $E     = RightJS.$E,
    Xhr    = RightJS.Xhr,
    Form   = RightJS.Form,
    RegExp = RightJS.RegExp;






/**
 * The uploading progress feature
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Uploader = new Widget({
  extend: {
    version: '2.2.1',

    EVENTS: $w('start update finish error'),

    Options: {
      url:         '/progress',
      param:       'X-Progress-ID',

      timeout:     1000,
      round:       0,
      fxDuration:  400,

      cssRule:     '[data-uploader]'
    }
  },

  /**
   * Basic constructor
   *
   * @param mixed a form reference
   * @param Object options
   */
  initialize: function(form, options) {
    this.form = form = $(form);

    // trying to find an existing progress-bar
    var element = form.first('.rui-uploader');

    this
      .$super('uploader', element)
      .setOptions(options, this.form)
      .addClass('rui-progress-bar')
      .insert([
        this.bar = this.first('.bar') || $E('div', {'class': 'bar'}),
        this.num = this.first('.num') || $E('div', {'class': 'num'})
      ]);

    if (!element) {
      this.insertTo(form);
    }
  },

  /**
   * Starts the uploading monitoring
   *
   * @return Uploader this
   */
  start: function() {
    var data = {state: 'starting'};
    return this.paint(data).prepare().request().fire('start', {data: data});
  },

// protected

  // updates uploading bar progress
  update: function(data) {
    this.paint(data).fire('update', {data: data});

    switch (data.state) {
      case 'starting':
      case 'uploading':
        R(this.request).bind(this).delay(this.options.timeout);
        break;
      case 'done':
        this.fire('finish', {data: data});
        break;
      case 'error':
        this.fire('error', {data: data});
        break;
    }

    return this;
  },

  // changes the actual element styles
  paint: function(data) {
    var percent = (this.percent || 0)/100;

    switch (data.state) {
      case 'starting':  percent = 0; break;
      case 'done':      percent = 1; break;
      case 'uploading': percent = data.received / (data.size||1); break;
    }

    this.percent = R(percent * 100).round(this.options.round);

    if (this.percent === 0 || !RightJS.Fx || !this.options.fxDuration) {
      this.bar._.style.width = this.percent + '%';
      this.num._.innerHTML   = this.percent + '%';
    } else {
      this.bar.morph({width: this.percent + '%'}, {duration: this.options.fxDuration});
      R(function() {
        this.num._.innerHTML = this.percent + '%';
      }).bind(this).delay(this.options.fxDuration / 2);
    }

    // marking the failed uploads
    this[data.state === 'error' ? 'addClass' : 'removeClass']('rui-progress-bar-failed');

    return this;
  },

  // sends a request to the server
  request: function() {
    Xhr.load(this.options.url + "?" + this.options.param + "=" + this.uid, {
      evalJS:   false,
      evalJSON: false,
      onSuccess: R(function(xhr) {
        this.update(new Function('return '+xhr.text)());
      }).bind(this)
    });

    return this;
  },

  // prepares the form to carry the x-progress-id param
  prepare: function() {
    this.uid = "";
    for (i = 0; i < 32; i++) { this.uid += Math.random(0, 15).toString(16); }

    var param = this.options.param;
    var url = this.form.get('action').replace(new RegExp('(\\?|&)'+RegExp.escape(param) + '=[^&]*', 'i'), '');
    this.form.set('action', url + (R(url).includes('?') ? '&' : '?') + param + '=' + this.uid);

    this.show();

    return this;
  }

});


/**
 * Overloading the Form#send method so we could
 * catch up the moment when a form was sent and show the bar
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var old_send = Form.prototype.send;

Form.include({
  send: function() {

    if (!this.uploader && (this.match(Uploader.Options.cssRule) || this.first('.rui-uploader'))) {
      this.uploader = new Uploader(this);
    }

    if (this.uploader) {
      this.uploader.start();
    }

    return old_send.apply(this, arguments);
  }
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("div.rui-progress-bar,div.rui-progress-bar *{margin:0;padding:0;border:none;background:none}div.rui-progress-bar{position:relative;height:1.4em;line-height:1.4em;width:20em;border:1px solid #999}div.rui-progress-bar,div.rui-progress-bar div.bar{border-radius:0.25em;-moz-border-radius:0.25em;-webkit-border-radius:0.25em}div.rui-progress-bar div.bar{position:absolute;left:0;top:0;width:0%;height:100%;background:#CCC;z-index:1}div.rui-progress-bar div.num{position:absolute;width:100%;height:100%;z-index:2;text-align:center}div.rui-progress-bar-failed{border-color:red;color:red;background:pink}.rui-uploader{display:none}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Uploader;
})(RightJS);
