/**
 * jQuery-like interfaces v2.2.1
 * http://rightjs.org/plugins/jquerysh
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
(function(RightJS) {
/**
 * jquerysh initialization script
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var _jQuery = window.jQuery,
    _$      = window.$,
    rjs_$   = RightJS.$,
    $$      = RightJS.$$,
    $E      = RightJS.$E,
    $A      = RightJS.$A,
    $ext    = RightJS.$ext,
    Xhr     = RightJS.Xhr,
    Browser = RightJS.Browser,
    Object  = RightJS.Object;




/**
 * The plugin's definition
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
RightJS.jQuerysh = {
  version: '2.2.1',

  // collection methods
  collectionMethods: {
    live: function(event, callback) {
      this.cssRule.on(event, callback);
      return this;
    },

    die: function(event, callback) {
      this.cssRule.stopObserving(event, callback);
      return this;
    }
  }
};

/**
 * jQuery-like '$' function behavior
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var $ = function(something) {
  switch(typeof something) {
    case 'string':
      var hash = something[0], id = something.substr(1);

      if (hash === '#' && (/^[\w\-]+$/).test(id)) {
        return rjs_$(id);
      } else if (hash === '<') {
        return $E('div', {html: something}).first();
      } else {
        hash = $$(something);
        hash.cssRule = RightJS(something);
        return $ext(hash, RightJS.jQuerysh.collectionMethods);
      }

    case 'function':
      return rjs_$(document).onReady(something);

    default:
      return rjs_$(something);
  }
};

/**
 * jQuery core methods emulation
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
$ext($, {

  browser: {
    webkit:  Browser.WebKit,
    opera:   Browser.Opera,
    msie:    Browser.IE,
    mozilla: Browser.Gecko
  },

// Type checks

  isFunction: function(value) {
    return RightJS.isFunction(value);
  },

  isArray: function(value) {
    return RightJS.isArray(value);
  },

  isPlainObject: function(value) {
    return RightJS.isHash(value);
  },

  isEmptyObject: function(value) {
    return Object.empty(value);
  },

  globalEval: function(script) {
    return RightJS.$eval(script);
  },

// Array stuff

  makeArray: function(value) {
    return $A(value);
  },

  each: function(list, callback) {
    return $A(list, function(item, index) {
      callback(index, item);
    });
  },

  map: function(callback) {
    return $A(value).map(callback);
  },

  unique: function(array) {
    return $A(array).uniq();
  },

  merge: function(first, second) {
    return $A(first).merge(second);
  },


// the rest of the things

  extend: function() {
    return Object.merge.apply(Object, arguments);
  },

  proxy: function(func, context) {
    return RightJS(func).bind(context);
  },

  noop: function() {
    return RightJS(function() {});
  },

  noConflict: function( deep ) {
    if ( window.$ === jQuery ) {
      window.$ = _$;
    }

    if ( deep && window.jQuery === jQuery ) {
      window.jQuery = _jQuery;
    }

    return $;
  }

});

/**
 * Element level jQuery like aliases
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
RightJS.Element.include({

  appendTo: function(target) {
    return this.insertTo(target);
  },

  prepend: function(content) {
    return this.insert(content, 'top');
  },

  before: function(content) {
    return this.insert(content, 'before');
  },

  after: function(content) {
    return this.insert(content, 'after');
  },

  insertBefore: function(target) {
    return this.insertTo(target, 'before');
  },

  attr: function(name, value) {
    return value === undefined ? this.get(name) : this.set(name, value);
  },

  css: function(name, value) {
    return (typeof(name) === 'string' && value === undefined) ?
      this.getStyle(name) : this.setStyle(name, value);
  },

  offset: function() {
    var position = this.position();
    return {
      left: position.x,
      top:  position.y
    };
  },

  width: function() {
    return this.size().x;
  },

  height: function() {
    return this.size().y;
  },

  scrollLeft: function() {
    return this.scrolls().x;
  },

  scrollTop: function() {
    return this.scrolls().y;
  },

  bind: function() {
    return this.on.apply(this, arguments);
  },

  unbind: function() {
    return this.stopObserving.apply(this, arguments);
  },

  trigger: function(name, options) {
    return this.fire(name, options);
  },

  animate: function(style, time, finish) {
    return this.morph(style, {duration: time, onFinish: finish});
  },

  fadeIn: function() {
    return this.fade('in');
  },

  fadeOut: function() {
    return this.fade('out');
  },

  slideDown: function() {
    return this.slide('in');
  },

  slideUp: function() {
    return this.slide('out');
  }

});

/**
 * jQuery like ajax interfaces
 *
 * Copyright (C) 2011 Nikolay Nemshilov
 */
$ext($, {

  param: function(object) {
    return Object.toQueryString(object);
  },

  ajax: function(url, options) {
    options = options || {};

    if (typeof(url) === 'string') {
      options.url = url;
    } else {
      options = url;
    }

    var xhr_options = {};

    function callback(original, xhr) {
      original(
        options.dataType === 'json' ? xhr.json : xhr.text,
        xhr.successful() ? 'success' : 'error',
        xhr
      );
    }

    if (options.success) {
      xhr_options.onSuccess = function() {
        callback(options.success, this);
      };
    }

    if (options.error) {
      xhr_options.onFailure = function() {
        callback(options.error, this);
      };
    }

    if (options.complete) {
      xhr_options.onComplete = function() {
        callback(options.complete, this);
      };
    }

    xhr_options.method  = options.type;

    if (options.headers) {
      xhr_options.headers = options.headers;
    }
    if (options.jsonp) {
      xhr_options.jsonp = options.jsonp;
    }
    if (options.url.indexOf('callback=?') > 0) {
      xhr_options.jsonp = true;
      options.url = options.url.replace(/(\?|\&)callback=\?/, '');
    }

    return new Xhr(options.url, xhr_options).send(options.data);
  },

  get: function() {
    return make_ajax_call({type: 'get'}, arguments);
  },

  post: function(url, data, success, data_type) {
    return make_ajax_call({type: 'post'}, arguments);
  },

  getJSON: function(url, data, success) {
    return make_ajax_call({dataType: 'json'}, arguments);
  },

  getScript: function(url, success) {
    return make_ajax_call({dataType: 'script'}, arguments);
  }

});

function make_ajax_call(opts, args) {
  return $.ajax($ext(ajax_options.apply(this, args), opts));
}

function ajax_options(url, data, success, data_type) {
  if (typeof(data) === 'function') {
    data_type = success;
    success   = data;
    data      = undefined;
  }

  return {
    url:      url,
    data:     data,
    success:  success,
    dataType: data_type
  };
}

Xhr.include({
  success: function(callback) {
    return this.on('success', callback);
  },

  error: function(callback) {
    return this.on('failure', callback);
  },

  complete: function(callback) {
    return this.on('complete', callback);
  }
});

window.$ = window.jQuery = $;
})(RightJS);