/**
 * RightJS-UI InEdit v2.2.0
 * http://rightjs.org/ui/in-edit
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var InEdit = RightJS.InEdit = (function(document, RightJS) {
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
 * In-Edit plugin initalization
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var R = RightJS,
    $ = RightJS.$,
    $w = RightJS.$w,
    Xhr     = RightJS.Xhr,
    Object  = RightJS.Object,
    Element = RightJS.Element,
    Input   = RightJS.Input;









/**
 * An inline editor feature
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var InEdit = new Widget('FORM', {
  extend: {
    version: '2.2.0',

    EVENTS: $w('show hide send update'),

    Options: {
      url:    null,    // the url address where to send the stuff
      name:   'text',  // the field name
      method: 'put',   // the method

      type:   'text',  // the input type, 'text', 'file', 'password' or 'textarea'

      toggle:  null,   // a reference to an element that should get hidden when the editor is active

      update:  true,   // a marker if the element should be updated with the response-text

      Xhr: {}          // additional Xhr options
    },

    i18n: {
      Save:   'Save',
      Cancel: 'Cancel'
    },

    current: null      // currently opened editor
  },

  /**
   * Constructor
   *
   * @param mixed an element reference
   * @param Object options
   * @return void
   */
  initialize: function(element, options) {
    this.element = $(element);

    this
      .$super('in-edit', options)
      .set('action', this.options.url)
      .insert([
        this.field   = new Input({type: this.options.type, name: this.options.name, 'class': 'field'}),
        this.spinner = new Spinner(4),
        this.submit  = new Input({type: 'submit', 'class': 'submit', value: InEdit.i18n.Save}),
        this.cancel  = new Element('a', {'class': 'cancel', href: '#', html: InEdit.i18n.Cancel})
      ])
      .onClick(this.clicked)
      .onSubmit(this.send);
  },

  /**
   * Shows the inline-editor form
   *
   * @return InEdit this
   */
  show: function() {
    if (InEdit.current !== this) {
      if (InEdit.current) { InEdit.current.hide(); }

      this.oldContent = this.element.html();

      if (!R(['file', 'password']).include(this.options.type)) {
        this.field.setValue(this.oldContent);
      }

      this.element.update(this);

      this.spinner.hide();
      this.submit.show();

      if (this.options.toggle) {
        $(this.options.toggle).hide();
      }
    }

    if (this.options.type !== 'file') {
      this.field.focus();
    }

    InEdit.current = this;
    return this.fire('show');
  },

  /**
   * Hides the form and brings the content back
   *
   * @param String optional new content
   * @return InEdit this
   */
  hide: function() {
    this.element._.innerHTML = this.oldContent;

    if (this.xhr) {
      this.xhr.cancel();
    }

    return this.finish();
  },

  /**
   * Triggers the form remote submit
   *
   * @return InEdit this
   */
  send: function(event) {
    if (event) { event.stop(); }

    this.spinner.show().resize(this.submit.size());
    this.submit.hide();

    this.xhr = new Xhr(this.options.url, Object.merge(this.options.Xhr, {
      method:     this.options.method,
      spinner:    this.spinner,
      onComplete: R(this.receive).bind(this)
    })).send(this);

    return this.fire('send');
  },

// protected

  // finishes up with the form
  finish: function() {
    if (this.options.toggle) {
      $(this.options.toggle).show();
    }

    InEdit.current = null;
    return this.fire('hide');
  },

  // the xhr callback
  receive: function() {
    if (this.options.update) {
      this.element.update(this.xhr.text);
      this.fire('update');
    }

    this.xhr = null;

    this.finish();
  },

  // catches clicks on the element
  clicked: function(event) {
    if (event.target === this.cancel) {
      event.stop();
      this.hide();
    }
  }

});


/**
 * The document hooks for in-edit form
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
$(document).onKeydown(function(event) {
  // processing the `ESC` button
  if (event.keyCode === 27 && InEdit.current) {
    InEdit.current.hide();
  }
});


/**
 * The element level inline editor extension
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
Element.include({
  /**
   * Triggers an inline-editor feature on the element
   *
   * @param Object options for the InEdit class
   * @return InEdit object
   */
  inEdit: function(options) {
    return new InEdit(this, options).show();
  }
});


var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("div.rui-spinner,div.rui-spinner div{margin:0;padding:0;border:none;background:none;list-style:none;font-weight:normal;float:none;display:inline-block; *display:inline; *zoom:1;border-radius:.12em;-moz-border-radius:.12em;-webkit-border-radius:.12em}div.rui-spinner{text-align:center;white-space:nowrap;background:#EEE;border:1px solid #DDD;height:1.2em;padding:0 .2em}div.rui-spinner div{width:.4em;height:70%;background:#BBB;margin-left:1px}div.rui-spinner div:first-child{margin-left:0}div.rui-spinner div.glowing{background:#777}form.rui-in-edit,form.rui-in-edit .cancel{margin:0;padding:0;float:none;position:static}form.rui-in-edit{display:inline-block; *display:inline; *zoom:1;border:none;background:none}form.rui-in-edit div.rui-spinner{margin-right:.2em}form.rui-in-edit div.rui-spinner div{margin-top:.2em}form.rui-in-edit textarea.field{width:100%;margin-bottom:.5em}form.rui-in-edit .field,form.rui-in-edit .submit{margin-right:.2em}form.rui-in-edit,form.rui-in-edit .field,form.rui-in-edit .submit,form.rui-in-edit div.rui-spinner,form.rui-in-edit .cancel{vertical-align:middle}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return InEdit;
})(document, RightJS);
