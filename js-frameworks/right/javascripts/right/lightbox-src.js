/**
 * RightJS-UI Lightbox v2.4.0
 * http://rightjs.org/ui/lightbox
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Lightbox = RightJS.Lightbox = (function(document, RightJS) {
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
 * The filenames to include
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var R       = RightJS,
    $       = RightJS.$,
    $$      = RightJS.$$,
    $w      = RightJS.$w,
    $E      = RightJS.$E,
    $ext    = RightJS.$ext,
    Xhr     = RightJS.Xhr,
    Class   = RightJS.Class,
    Object  = RightJS.Object,
    Element = RightJS.Element,
    Browser = RightJS.Browser;

// IE6 doesn't support position:fixed so it needs a crunch
Browser.IE6 = Browser.OLD && navigator.userAgent.indexOf("MSIE 6") > 0;








/**
 * The lightbox widget
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
var Lightbox = new Widget({

  extend: {
    version: '2.4.0',

    EVENTS: $w('show hide load'),

    Options: {
      fxName:          'fade',
      fxDuration:      300,

      group:           null, // a group marker

      hideOnEsc:       true,
      hideOnOutClick:  true,
      showCloseButton: true,

      cssRule:         "a[data-lightbox]", // all lightbox links css-rule

      // video links default size
      mediaWidth:      425,
      mediaHeight:     350,

      fullscreen:      true // allow fullscreen video
    },

    i18n: {
      Close: 'Close',
      Prev:  'Previous Image',
      Next:  'Next Image'
    },

    // the supported image-urls regexp
    Images: /\.(jpg|jpeg|gif|png|bmp)/i,

    // media content sources
    Medias: [
      [/(http:\/\/.*?youtube\.[a-z]+)\/watch\?v=([^&]+)/i,       '$1/v/$2',                      'swf'],
      [/(http:\/\/video.google.com)\/videoplay\?docid=([^&]+)/i, '$1/googleplayer.swf?docId=$2', 'swf'],
      [/(http:\/\/vimeo\.[a-z]+)\/([0-9]+).*?/i,                 '$1/moogaloop.swf?clip_id=$2',  'swf']
    ]
  },

  /**
   * basic constructor
   *
   * @param Object options override
   * @param Element optional options holder
   * @return void
   */
  initialize: function(options, context) {
    this
      .$super('lightbox', {})
      .setOptions(options, context)
      .insert([
        this.locker = new Locker(this.options),
        this.dialog = new Dialog(this.options)
      ])
      .on({
        close: this._close,
        next:  this._next,
        prev:  this._prev
      });
  },

  /**
   * Extracting the rel="lightbox[groupname]" attributes
   *
   * @param Object options
   * @param Element link with options
   * @return Dialog this
   */
  setOptions: function(options, context) {
    this.$super(options, context);

    if (context) {
      var rel = context.get('rel');
      if (rel && (rel = rel.match(/lightbox\[(.+?)\]/))) {
        this.options.group = rel[1];
      }
    }

    return this;
  },

  /**
   * Sets the popup's title
   *
   * @param mixed string or element or somethin'
   * @return Lighbox self
   */
  setTitle: function(text) {
    this.dialog.setTitle(text);

    return this;
  },

  /**
   * Shows the lightbox
   *
   * @param String/Array... content
   * @return Lightbox this
   */
  show: function(content) {
    return this._showAnd(function() {
      this.dialog.show(content, !content);
    });
  },

  /**
   * Hides the lightbox
   *
   * @return Lightbox this
   */
  hide: function() {
    Lightbox.current = null;

    return this.$super(this.options.fxName, {
      duration: this.options.fxDuration/3,
      onFinish: R(function() {
        this.fire('hide');
        this.remove();
      }).bind(this)
    });
  },

  /**
   * Loads up the data from url or a link
   *
   * @param String address or a link element
   * @param Object Xhr options
   * @return Lightbox this
   */
  load: function(link, options) {
    return this._showAnd(function() {
      this.dialog.load(link, options);
    });
  },

  /**
   * Resizes the content block to the given size
   *
   * @param Hash size
   * @return Lightbox this
   */
  resize: function(size) {
    this.dialog.resize(size);
    return this;
  },

// protected

  // handles the 'close' event
  _close: function(event) {
    event.stop();
    this.hide();
  },

  // handles the 'prev' event
  _prev: function(event) {
    event.stop();
    Pager.prev();
  },

  // handles the 'next' event
  _next: function(event) {
    event.stop();
    Pager.next();
  },

  // shows the lightbox element and then calls back
  _showAnd: function(callback) {
    if (Lightbox.current !== this) {
      Lightbox.current = this;

      // hidding all the hanging around lightboxes
      $$('div.rui-lightbox').each('remove');

      this.insertTo(document.body);
      this.dialog.show('', true);

      if (Browser.OLD) { // IE's get screwed by the transparency tricks
        this.reposition();
        Element.prototype.show.call(this);
        callback.call(this);
      } else {
        this.setStyle('display:none');
        Element.prototype.show.call(this, this.options.fxName, {
          duration: this.options.fxDuration/2,
          onFinish: R(function() {
            callback.call(this);
            this.fire('show');
          }).bind(this)
        });
      }
    } else {
      callback.call(this);
    }

    return this;
  },

  // manually repositioning under IE6 browser
  reposition: function() {
    if (Browser.IE6) {
      var win = $(window);

      this.setStyle({
        top:      win.scrolls().y + 'px',
        width:    win.size().x    + 'px',
        height:   win.size().y    + 'px',
        position: "absolute"
      });
    }
  }
});


/**
 * The class level interface
 *
 * @copyright (C) 2009 Nikolay Nemshilov
 */
Lightbox.extend({
  hide: function() {
    if (Lightbox.current) {
      Lightbox.current.hide();
    }
  },

  show: function() {
    return this.inst('show', arguments);
  },

  load: function() {
    return this.inst('load', arguments);
  },

// private

  inst: function(name, args) {
    var inst = new Lightbox();
    return inst[name].apply(inst, args);
  }
});


/**
 * Lightbox background locker element
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Locker = new Class(Element, {
  initialize: function(options) {
    this.$super('div', {'class': 'rui-lightbox-locker'});

    if (options.hideOnOutClick) {
      this.onClick('fire', 'close');
    }
  }
});


/**
 * The dialog element wrapper
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Dialog = new Class(Element, {
  /**
   * Constructor
   *
   * @param Object options
   * @return void
   */
  initialize: function(options) {
    var i18n = Lightbox.i18n;

    this.options = options;
    this.$super('div', {'class': 'rui-lightbox-dialog'});

    // building up the
    this.insert([
      this.title = $E('div', {'class': 'rui-lightbox-title'}),

      $E('div', {'class': 'rui-lightbox-body'}).insert(
        $E('div', {'class': 'rui-lightbox-body-inner'}).insert([
          this.locker    = $E('div', {'class': 'rui-lightbox-body-locker'}).insert(new Spinner(4)),
          this.scroller  = $E('div', {'class': 'rui-lightbox-scroller'}).insert(
            this.content = $E('div', {'class': 'rui-lightbox-content'})
          )
        ])
      ),

      $E('div', {'class': 'rui-lightbox-navigation'}).insert([
        this.closeButton = $E('div', {'class': 'close', html: '&times;', title: i18n.Close}).onClick('fire', 'close'),
        this.prevLink    = $E('div', {'class': 'prev',  html: '&larr;',  title: i18n.Prev}).onClick('fire',  'prev'),
        this.nextLink    = $E('div', {'class': 'next',  html: '&rarr;',  title: i18n.Next}).onClick('fire',  'next')
      ])
    ]);

    // presetting the navigation state
    this.prevLink.hide();
    this.nextLink.hide();

    if (!options.showCloseButton) {
      this.closeButton.hide();
    }
  },

  /**
   * Sets the dialogue caption
   *
   * @param String title
   * @return Dialog this
   */
  setTitle: function(title) {
    this.title.update(title||'');
  },

  /**
   * Nicely resize the dialog box
   *
   * @param Object the end size
   * @param Boolean use fx (false by default)
   * @return Dialog this
   */
  resize: function(end_size, with_fx) {
    var win_size = this.parent().size(),
        cur_size = this.scroller.size(),
        cur_top  = (win_size.y - this.size().y)/2,
        dlg_diff = this.size().x - cur_size.x; // <- use for IE6 fixes

    if (end_size) {
      // getting the actual end-size
      end_size = this.scroller.setStyle(end_size).size();

      this.scroller.setStyle({
        width:  cur_size.x + 'px',
        height: cur_size.y + 'px'
      });
    } else {
      // using the content block size
      end_size = this.content.size();
    }

    // checking the constraints
    var threshold = 100; // px

    if ((/^<img [^>]+>/img).test(this.content.html())) {
      // adjusting the sizes propoprtinally for images
      R([['x', 'y'], ['y', 'x']]).each(function(set) {
        var dim1 = set[0], dim2 = set[1], old_size = end_size[dim1];

        if ((end_size[dim1] + threshold) > win_size[dim1]) {
          end_size[dim1] = win_size[dim1] - threshold;
          end_size[dim2] = Math.floor(end_size[dim2] * end_size[dim1] / old_size);
        }
      });

      this.content.first('img').setStyle({
        width:  end_size.x + 'px',
        height: end_size.y + 'px'
      });
    } else {
      // adjusting the sizes in case of any other content
      if ((end_size.x + threshold) > win_size.x) { end_size.x = win_size.x - threshold; }
      if ((end_size.y + threshold) > win_size.y) { end_size.y = win_size.y - threshold; }
    }

    // the actual resize and reposition
    var end_top = (cur_top * 2 + cur_size.y - end_size.y) / 2;
    var dialog  = this._.style, content = this.scroller._.style;

    if (RightJS.Fx && with_fx && (end_size.x != cur_size.x || end_size.y != cur_size.y)) {

      $ext(new RightJS.Fx(this, {duration: this.options.fxDuration}), {
        render: function(delta) {
          content.width  = (cur_size.x + (end_size.x - cur_size.x) * delta) + 'px';
          content.height = (cur_size.y + (end_size.y - cur_size.y) * delta) + 'px';
          dialog.top     = (cur_top    + (end_top    - cur_top)    * delta) + 'px';

          if (Browser.IE6) {
            dialog.width  = (dlg_diff + cur_size.y + (end_size.y - cur_size.y) * delta) + 'px';
          }
        }
      }).onFinish(R(this.unlock).bind(this)).start();

    } else {
      // no-fx direct assignment
      content.width  = end_size.x + 'px';
      content.height = end_size.y + 'px';
      dialog.top     = end_top    + 'px';

      if (Browser.IE6) {
        dialog.width = (dlg_diff + end_size.x) + 'px';
      }

      if (!this.request) { this.unlock(); }
    }

    return this;
  },

  /**
   * Shows the content
   *
   * @param mixed content String/Element/Array and so one
   * @return Dialog this
   */
  show: function(content, no_fx) {
    this.content.update(content || '');
    this.resize(null, !no_fx);
  },

  /**
   * Loads up the data from the link
   *
   * @param mixed String url address or a link element
   * @param Object xhr-options
   * @return void
   */
  load: function(url, options) {
    if (url instanceof Element) {
      this.setTitle(url.get('title'));
      url = url.get('href');
    }

    Pager.show(this, url);
    this.lock().cancel();

    // defined in the loader.js file
    this.request = new Loader(url, options, R(function(content, no_fx) {
      this.request = null;
      this.show(content, no_fx);
    }).bind(this));

    return this.resize(); // the look might be changed for a media-type
  },

  /**
   * Cancels a currently loading request
   *
   * @return Dialog this
   */
  cancel: function() {
    if (this.request) {
      this.request.cancel();
    }

    return this;
  },

  /**
   * Shows the loading lock
   *
   * @return Dialog this
   */
  lock: function() {
    this.locker.setStyle('opacity:1;display:block').insertTo(this.scroller, 'before');
    return this;
  },

  /**
   * Hides the loading lock
   *
   * @return Dialog this
   */
  unlock: function() {
    this.locker.remove(R(this.content.html()).blank() ? null : 'fade', {
      duration: this.options.fxDuration * 2/3
    });

    return this;
  }
});


/**
 * Xhr/images/medias loading module
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
var Loader = new Class({
  /**
   * Constructor
   *
   * @param String url address
   * @param Object Xhr options
   * @param Function on-finish callback
   */
  initialize: function(url, options, on_finish) {
    // adjusting the dialog look for different media-types
    if (this.isImage(url, on_finish)) {
      Lightbox.current.addClass('rui-lightbox-image');
    } else if (this.isMedia(url, on_finish)) {
      Lightbox.current.addClass('rui-lightbox-media');
    } else {
      this.xhr = new Xhr(url,
        Object.merge({method: 'get'}, options)
      ).onComplete(function() {
        on_finish(this.text);
      }).send();
    }
  },

  /**
   * Cancels the request
   *
   * @return Loader this
   */
  cancel: function() {
    if (this.xhr) {
      this.xhr.cancel();
    } else if (this.img) {
      this.img.onload = function() {};
    }
  },

// protected

  // tries to initialize it as an image loading
  isImage: function(url, on_finish) {
    if (url.match(Lightbox.Images)) {
      var img = this.img = $E('img')._;
      img.onload = function() {
        on_finish(img);
      };
      img.src = url;
      return true;
    }
  },

  // tries to initialize it as a flash-element
  isMedia: function(url, on_finish) {
    var media = R(Lightbox.Medias).map(function(desc) {
      return url.match(desc[0]) ? this.buildEmbed(
        url.replace(desc[0], desc[1]), desc[2]) : null;
    }, this).compact()[0];

    if (media) {
      on_finish(media, true);
      return true;
    }
  },

  // builds an embedded media block
  buildEmbed: function(url, type) {
    var media_types = {
      swf: [
        'D27CDB6E-AE6D-11cf-96B8-444553540000',
        'http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0',
        'application/x-shockwave-flash'
      ]
    },
    options = Lightbox.current ? Lightbox.current.options : Lightbox.Options,
    sizes = ' width="'+ options.mediaWidth + '" height="'+ options.mediaHeight + '"',
    fullscreen_param = options.fullscreen ? '<param name="allowFullScreen" value="true"></param>' : '',
    fullscreen_attr  = options.fullscreen ? ' allowfullscreen="true"' : '';

    if (url.indexOf('youtube.com') > 0 && options.fullscreen) {
      url += '?version=3&amp;hl=en_US&amp;rel=0';
    }

    return '<object classid="clsid:' + media_types[type][0] +
      '" codebase="' + media_types[type][1] + '"'+ sizes + '>' +
      '<param name="src" value="'+ url +'" />'+ fullscreen_param +
      '<embed src="'+ url +'" type="'+ media_types[type][2]+'"'+ sizes + fullscreen_attr+ ' />' +
    '</object>';
  }

});


/**
 * Processes the link-groups showing things in a single Lightbox
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var Pager = {
  /**
   * Checks and shows the pager links on the dialog
   *
   * @param Dialog dialog
   * @param String url-address
   * @return void
   */
  show: function(dialog, url) {
    if (dialog.options.group) {
      this.dialog = dialog;
      this.links  = this.find(dialog.options.group);
      this.link   = this.links.first(function(link) {
        return link.get('href') === url;
      });

      var index = this.links.indexOf(this.link), size = this.links.length;

      dialog.prevLink[size && index > 0 ? 'show' : 'hide']();
      dialog.nextLink[size && index < size - 1 ? 'show' : 'hide']();
    } else {
      this.dialog = null;
    }
  },

  /**
   * Shows the prev link
   *
   * @return void
   */
  prev: function() {
    if (this.dialog && !this.timer) {
      var id   = this.links.indexOf(this.link),
          link = this.links[id - 1];

      if (link) {
        this.dialog.load(link);
        this.timeout();
      }
    }
  },

  /**
   * Shows the next link
   *
   * @return void
   */
  next: function() {
    if (this.dialog && !this.timer) {
      var id   = this.links.indexOf(this.link),
          link = this.links[id + 1];

      if (link) {
        this.dialog.load(link);
        this.timeout();
      }
    }
  },

// private

  // finding the links list
  find: function(group) {
    return $$('a').filter(function(link) {
      var data = link.get('data-lightbox');
      var rel  = link.get('rel');

      return (data && new Function("return "+ data)().group === group) ||
        (rel && rel.indexOf('lightbox['+ group + ']') > -1);
    });
  },

  // having a little nap to prevent ugly quick scrolling
  timeout: function() {
    this.timer = R(function() {
      Pager.timer = null;
    }).delay(300);
  }
};


/**
 * document level hooks
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */

$(document).on({
  /**
   * Catches clicks on the target links (respecting
   * native behavior when modifier keys are pressed,
   * e.g. opt+click must download clicked link)
   *
   * @param Event click
   * @return void
   */
  click: function(event) {
    var target = event.find(Lightbox.Options.cssRule) || event.find('a[rel^=lightbox]'),
    e = event._;

    if (target && !(e.shiftKey || e.altKey || e.ctrlKey || e.metaKey)) {
      event.stop();
      new Lightbox({}, target).load(target);
    }
  },

  /**
   * Catches the mousewheel event and tries to scroll
   * the list of objects on the lightbox
   *
   * @param Event mousewheel
   * @return void
   */
  mousewheel: function(event) {
    if (Lightbox.current) {
      var target = event.target, box = target.parent('div.rui-lightbox-content');

      if (!box || target.getStyle('overflow') === 'visible') {
        event.stop();
      }

      Lightbox.current.fire((event._.detail || -event._.wheelDelta) < 0 ? 'prev' : 'next');
    }
  },

  /**
   * Handles the navigation form a keyboard
   *
   * @param Event keydown
   * @return void
   */
  keydown: function(event) {
    var lightbox = Lightbox.current, name = ({
      27: 'close', // Esc
      33: 'prev',  // PageUp
      37: 'prev',  // Left
      38: 'prev',  // Up
      39: 'next',  // Right
      40: 'next',  // Down
      34: 'next'   // PageDown
    })[event.keyCode];

    if (lightbox && name) {
      if (name !== 'close' || lightbox.options.hideOnEsc) {
        event.stop();
        lightbox.fire(name);
      }
    }
  }
});

$(window).on({
  resize: function() {
    if (Lightbox.current) {
      Lightbox.current.reposition();
      Lightbox.current.dialog.resize();
    }
  },

  scroll: function(event) {
    if (Lightbox.current && Browser.IE6) {
      Lightbox.current.reposition();
    }
  }
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("div.rui-spinner,div.rui-spinner div{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none;display:inline-block; *display:inline; *zoom:1;border-radius:.12em;-moz-border-radius:.12em;-webkit-border-radius:.12em}div.rui-spinner{text-align:center;white-space:nowrap;background:#EEE;border:1px solid #DDD;height:1.2em;padding:0 .2em}div.rui-spinner div{width:.4em;height:70%;background:#BBB;margin-left:1px}div.rui-spinner div:first-child{margin-left:0}div.rui-spinner div.glowing{background:#777}div.rui-lightbox{position:fixed;top:0;left:0;float:none;width:100%;height:100%;margin:0;padding:0;background:none;border:none;text-align:center;z-index:9999;z-index/*\\**/:auto\\9}div.rui-lightbox-locker{position:absolute;top:0px;left:0px;width:100%;height:100%;background-color:#000;opacity:0.8;filter:alpha(opacity=80);cursor:default;z-index/*\\**/:9990\\9}div.rui-lightbox-dialog{display:inline-block; *display:inline; *zoom:1;position:relative;text-align:left;z-index/*\\**/:9999\\9}div.rui-lightbox-title{height:1.2em;margin-bottom:.1em;white-space:nowrap;color:#DDD;font-weight:bold;font-size:1.6em;font-family:Helvetica}div.rui-lightbox-body{background-color:#FFF;padding:1em;border-radius:.5em;-moz-border-radius:.5em;-webkit-border-radius:.5em}div.rui-lightbox-body-inner{position:relative}div.rui-lightbox-scroller{overflow:hidden}div.rui-lightbox-content{display:inline-block; *display:inline; *zoom:1;min-height:10em;min-width:10em;_height:10em;_width:10em}div.rui-lightbox-body-locker{background-color:white;position:absolute;left:0px;top:0px;width:100%;height:100%;opacity:0;filter:alpha(opacity=0)}div.rui-lightbox-body-locker div.rui-spinner{position:absolute;right:0;bottom:0;border:none;background:none;font-size:150%}div.rui-lightbox-navigation{color:#888;font-size:160%;font-family:Arial;height:1em;user-select:none;-moz-user-select:none;-webkit-user-select:none}div.rui-lightbox-navigation div{cursor:pointer;position:absolute}div.rui-lightbox-navigation div:hover{color:white}div.rui-lightbox-navigation div.next{left:2em}div.rui-lightbox-navigation div.close{right:0}div.rui-lightbox-image div.rui-lightbox-body,div.rui-lightbox-media div.rui-lightbox-body{padding:0;border:1px solid #777;border-radius:0px;-moz-border-radius:0px;-webkit-border-radius:0px}div.rui-lightbox-image div.rui-lightbox-content,div.rui-lightbox-media div.rui-lightbox-content{min-height:12em;min-width:12em;_height:12em;_width:12em}div.rui-lightbox-image div.rui-lightbox-content img{vertical-align:middle}div.rui-lightbox-image div.rui-lightbox-body,div.rui-lightbox-image div.rui-lightbox-body-locker,div.rui-lightbox-media div.rui-lightbox-body,div.rui-lightbox-media div.rui-lightbox-body-locker{background-color:#D8D8D8}div.rui-lightbox-image div.rui-lightbox-body-locker div.rui-spinner,div.rui-lightbox-media div.rui-lightbox-body-locker div.rui-spinner{bottom:.5em;right:.5em}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Lightbox;
})(document, RightJS);
