/**
 * RightJS-UI RTE v2.2.1
 * http://rightjs.org/ui/rte
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Rte = RightJS.Rte = (function(RightJS, document, window) {
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
 * RTE's initialization script
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */

var R       = RightJS,
    $       = RightJS.$,
    $$      = RightJS.$$,
    $w      = RightJS.$w,
    $E      = RightJS.$E,
    $A      = RightJS.$A,
    isArray = RightJS.isArray,
    RegExp  = RightJS.RegExp,
    Class   = RightJS.Class,
    Element = RightJS.Element,
    Input   = RightJS.Input;




/**
 * The Right Text Editor
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Rte = new Widget({

  extend: {
    version: '2.2.1',

    EVENTS: $w('change focus blur'),

    // checking if the 'contentEditable' feature is supported at all
    supported: 'contentEditable' in document.createElement('div'),

    Options: {
      toolbar:     'small',  // toolbar, the name or an array of your own

      autoresize:  true,     // automatically resize the editor's height to fit the text

      showToolbar: true,     // show the toolbar
      showStatus:  true,     // show the status bar

      videoSize:   '425x344', // flash-video blocks default size

      cssRule:     'textarea[data-rte]'
    },

    // predefined toolbars set
    Toolbars: {
      small: ['Bold Italic Underline Strike Ttext|Cut Copy Paste|Header Code Quote|Link Image Video|Source'],
      basic: [
        'Save Clear|Cut Copy Paste|Bold Italic Underline Strike Ttext|Left Center Right Justify',
        'Undo Redo|Header Code Quote|Link Image Video|Dotlist Numlist|Indent Outdent|Source'
      ],
      extra: [
        'Save Clear|Cut Copy Paste|Bold Italic Underline Strike Ttext|Left Center Right Justify',
        'Undo Redo|Header Code Quote|Link Image Video|Subscript Superscript|Dotlist Numlist|Indent Outdent',
        'Format|Fontname Fontsize|Forecolor Backcolor|Source'
      ]
    },

    Tools: {}, // the index of available tools will be here

    // the keyboard bindings
    Shortcuts: {
      Bold:      'b',
      Italic:    'i',
      Underline: 'u',
//      Ttext:     't',
      Header:    'h',
      Link:      'l',
      Cut:       'x',
      Copy:      'c',
      Paste:     'v',
      Undo:      'z',
      Redo:      'shift+z',
      Source:    'e',
//      Quote:     'q',
      Code:      'p',
      Save:      's'
    },

    // tags used by default with formatting tools
    Tags: {
      Bold:      'b',
      Italic:    'i',
      Underline: 'u',
      Strike:    's',
      Ttext:     'tt',
      Code:      'pre',
      Quote:     'blockquote',
      Header:    'h2'
    },

    // the formatting options, you can use simply tag names
    // or you can also specify tag + class like 'div.blue'
    Formats: {
      'h1':         'Header 1',
      'h2':         'Header 2',
      'h3':         'Header 3',
      'h4':         'Header 4',
      'p':          'Paragraph',
      'pre':        'Preformatted',
      'blockquote': 'Blockquote',
      'tt':         'Typetext',
      'address':    'Address'
    },

    // the font-name options
    FontNames: {
      'Andale Mono':     'andale mono,times',
      'Arial':           'arial,helvetica,sans-serif',
      'Arial Black':     'arial black,avant garde',
      'Book Antiqua':    'book antiqua,palatino',
      'Comic Sans MS':   'comic sans ms,sans-serif',
      'Courier New':     'courier new,courier',
      'Georgia':         'georgia,palatino',
      'Helvetica':       'helvetica',
      'Impact':          'impact,chicago',
      'Symbol':          'symbol',
      'Tahoma':          'tahoma,arial,helvetica,sans-serif',
      'Terminal':        'terminal,monaco',
      'Times New Roman': 'times new roman,times',
      'Trebuchet MS':    'trebuchet ms,geneva',
      'Verdana':         'verdana,geneva',
      'Webdings':        'webdings',
      'Wingdings':       'wingdings,zapf dingbats'
    },

    // the font-size options
    FontSizes: '6pt 7pt 8pt 9pt 10pt 11pt 12pt 14pt 18pt 24pt 36pt',

    Videos: [
      // supported swf video resources
      [/(http:\/\/.*?youtube\.[a-z]+)\/watch\?v=([^&]+)/,       '$1/v/$2'],
      [/(http:\/\/video.google.com)\/videoplay\?docid=([^&]+)/, '$1/googleplayer.swf?docId=$2'],
      [/(http:\/\/vimeo\.[a-z]+)\/([0-9]+).*?/,                 '$1/moogaloop.swf?clip_id=$2']
    ],

    i18n: {
      Clear:       'Clear',
      Save:        'Save',
      Source:      'Source',
      Bold:        'Bold',
      Italic:      'Italic',
      Underline:   'Underline',
      Strike:      'Strike through',
      Ttext:       'Typetext',
      Header:      'Header',
      Cut:         'Cut',
      Copy:        'Copy',
      Paste:       'Paste',
      Pastetext:   'Paste as text',
      Left:        'Left',
      Center:      'Center',
      Right:       'Right',
      Justify:     'Justify',
      Undo:        'Undo',
      Redo:        'Redo',
      Code:        'Code block',
      Quote:       'Block quote',
      Link:        'Add link',
      Image:       'Insert image',
      Video:       'Insert video',
      Dotlist:     'List with dots',
      Numlist:     'List with numbers',
      Indent:      'Indent',
      Outdent:     'Outdent',
      Forecolor:   'Text color',
      Backcolor:   'Background color',
      Select:      'Select',
      Remove:      'Remove',
      Format:      'Format',
      Fontname:    'Font name',
      Fontsize:    'Size',
      Subscript:   'Subscript',
      Superscript: 'Superscript',
      UrlAddress:  'URL Address'
    },

    current: null
  },

  /**
   * Basic constructor
   *
   * @param Input textarea reference
   * @param Object additional options
   * @return void
   */
  initialize: function(textarea, options) {
    this
      .$super('rte', {})
      .setOptions(options, textarea)
      .append(
        this.toolbar = new Rte.Toolbar(this),
        this.editor  = new Rte.Editor(this),
        this.status  = new Rte.Status(this)
      );

    if (!this.options.showToolbar) {
      this.toolbar.hide();
    }

    if (!this.options.showStatus) {
      this.status.hide();
    }

    if (textarea) {
      this.assignTo(textarea);
    }

    this.undoer    = new Rte.Undoer(this);
    this.selection = new Rte.Selection(this);

    // updating the initial state
    this.selection.exec('styleWithCss', false);
    this.status.update();
  },

  /**
   * Sets the value
   *
   * @param String value
   * @return Rte this
   */
  setValue: function(value) {
    if (this.textarea) {
      this.textarea.value(value);
    }
    this.editor.update(value);
    return this;
  },

  /**
   * Returns the current value
   *
   * @return String current value
   */
  getValue: function() {
    return this.editor._.innerHTML;
  },

  /**
   * Bidirectional method to set/get the value
   *
   * @param String value
   * @return Rte this or String value
   */
  value: function(value) {
    return this[value === undefined ? 'getValue' : 'setValue'](value);
  },

  /**
   * Disables the editor
   *
   * @return Rte this
   */
  disable: function() {
    this.disabled = true;
    return this.addClass('rui-rte-disabled');
  },

  /**
   * Enables the editor
   *
   * @return Rte this
   */
  enable: function() {
    this.disabled = false;
    return this.removeClass('rui-rte-disabled');
  },

  /**
   * Puts the focus into the editor
   *
   * @return Rte this
   */
  focus: function() {
    if (Rte.current !== this) {
      Rte.current = this;
      this.editor.focus();
    }

    return this;
  },

  /**
   * Looses focus from the editor
   *
   * @return Rte this
   */
  blur: function() {
    Rte.current = null;

    this.editor.blur();

    return this;
  },

  /**
   * Assigns this Rte to work with this textarea
   *
   * @param mixed textarea reference
   * @return Rte this
   */
  assignTo: function(element) {
    var textarea = $(element),
        size = textarea.size();

    // displaying self only if the 'contentEditable' feature is supported
    // otherwise keeping original textarea where it is
    if (Rte.supported) {
      this.insertTo(textarea.setStyle(
        'position:absolute;left:-9999em;'
      ), 'before');

      this.editor.resize(size);
      this.setWidth(size.x);

      if (this.options.autoresize) {
        this.editor.setStyle({
          minHeight: size.y + 'px',
          height:  'auto'
        });
      }
    } else {
      textarea.setStyle('visibility:visible');
    }

    this.setValue(textarea.value());
    this.onChange(function() {
      textarea._.value = this.editor._.innerHTML;
    });

    this.textarea = textarea;

    return this;
  }

});

/**
 * Rte's toolbar unit
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Toolbar = new Class(Element, {

  initialize: function(rte) {
    this.$super('div', {'class': 'rui-rte-toolbar'});

    this.rte  = rte;
    rte.tools = {};

    var options = rte.options, toolbar = options.toolbar;

    R(Rte.Toolbars[toolbar] || (isArray(toolbar) ? toolbar : [toolbar])).each(function(line_s) {
      var line = $E('div', {'class': 'line'}).insertTo(this);

      R(line_s.split('|')).each(function(bar_s) {
        if (!R(bar_s).blank()) {
          var bar = $E('div', {'class': 'bar'}).insertTo(line);

          R(bar_s.split(' ')).each(function(tool) {
            tool = R(tool).capitalize();
            bar.insert(new Rte.Tools[tool](rte));
          });
        }
      });
    }, this);

    // adding hidden undo/redo tools if they are not on the toolbar
    // so that the undoer kicked in on the keybindings
    rte.tools.Undo || new Rte.Tools.Undo(rte);
    rte.tools.Redo || new Rte.Tools.Redo(rte);
  },

  /**
   * Finds a tool for the keyboard event
   *
   * @param {Event} event
   * @return {Rte.Tool} wired tool or false
   */
  shortcut: function(event) {
    var raw  = event._, key, tool;

    for (key in this.rte.tools) {
      tool = this.rte.tools[key];

      if (tool.shortcut === raw.keyCode && tool.shiftKey === raw.shiftKey) {
        return tool;
      }
    }

    return null;
  }

});

/**
 * The actual Editor unit for the Rte
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Editor = new Class(Element, {

  /**
   * Basic constructor
   *
   * @param Rte rte
   * @return void
   */
  initialize: function(rte) {
    // IE won't allow us to set 'contenteditable' progarmatically
    // so we put it as a textual content and then find it manually
    this.$super(rte
      .append('<div contenteditable="true" class="rui-rte-editor"></div>')
      .first('div.rui-rte-editor')._
    );

    this.rte = rte;

    this.on({
      focus:    this._focus,
      blur:     this._blur,
      mouseup:  this._mouseup,
      keypress: this._keypress,
      keydown:  this._keydown,
      keyup:    this._keyup
    });
  },

  /**
   * Updates the editor's content
   *
   * @param String text
   * @return Rte.Editor this
   */
  update: function(text) {
    this.$super(text);
    return this;
  },

  /**
   * puts focus on the editing area
   *
   * @return Rte.Editor this
   */
  focus: function() {
    this._.focus();
    return this;
  },

  /**
   * removes focus out of the editing area
   *
   * @return Rte.Editor this
   */
  blur: function() {
    this._.blur();
    return this;
  },

  /**
   * Removes the element from the editor, placing all its content
   * in its place
   *
   * @param raw dom element
   * @return void
   */
  removeElement: function(element) {
    if (element !== null) {
      var parent = element.parentNode;
      while (element.firstChild) {
        parent.insertBefore(element.firstChild, element);
      }
      parent.removeChild(element);
    }
  },

// protected

  _focus: function() {
    this.rte.selection.restore();
    this.rte.status.update();
    this.rte.focused = true;
  },

  _blur: function() {
    this.rte.focused = false;
    this.rte.status.update();
  },

  _mouseup: function() {
    this._focus();
  },

  _keypress: function(event) {
    if (this.__stopped) {
      event.stop();
    }
  },

  _keydown: function(event) {
    var raw = event._, stopped = false, tool;

    if (raw.metaKey || raw.ctrlKey) {
      if ((tool = this.rte.toolbar.shortcut(event))) {
        tool.call(event);
      }

      stopped = event.stopped;
    }

    // an internal marker to lock the 'keypress' event later on
    this.__stopped = stopped;
  },

  _keyup: function(event) {
    switch (event.keyCode) {
      case 37: // arrow
      case 38: // arrow
      case 39: // arrow
      case 40: // arrow
        this.rte.status.update();
        break;

      default:
        // watching the typing pauses to fire 'change' events
        var rte = this.rte, editor = this._;

        if (this._timer !== false) { window.clearTimeout(this._timer); }

        this._timer = window.setTimeout(function() {
          if (rte.__old_value !== editor.innerHTML) {
            rte.__old_value = editor.innerHTML;
            rte.fire('change');
          }
        }, this._delay);
    }
  },

  _timer: false,
  _delay: 400

});

/**
 * The Rte's status bar block
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Status = new Class(Element, {

  /**
   * Basic constructor
   *
   * @param Rte
   * @return void
   */
  initialize: function(rte) {
    this.$super('div', {'class': 'rui-rte-status'});
    this.rte   = rte;
    this.nodes = [];
    this.tags  = [];

    this.onMousedown(this._mousedown);
  },

  /**
   * Updates the current status
   *
   * @return Rte.Status this
   */
  update: function() {
    this._findNodes();
    this._checkTools();

    return this.$super(this.nodes.map(function(node, index) {
      var name = node.tagName.toLowerCase();

      if (node.id) {
        name += "#"+ node.id;
      }

      if (node.className) {
        name += "."+ node.className;
      }

      return '<a href="" data-index="'+ index +
        '" onclick="return false;" title="'+
          Rte.i18n.Select +
        '">'+ name +'</a>';

    }).join(' &rsaquo; '));
  },

  /**
   * Finds an element in the current status stack
   *
   * @param String tag name
   * @param Object optional attributes
   * @return raw element or null if nothing found
   */
  findElement: function(tag, attributes) {
    if (tag) {
      for (var i = this.nodes.length - 1, key, match; i > -1; i--) {
        if (this.nodes[i].tagName === tag) {
          match = true;

          for (key in attributes) {
            if (attributes[key] instanceof RegExp) {
              match &= attributes[key].test(this.nodes[i].getAttribute(key));
            } else {
              match &= this.nodes[i].getAttribute(key) == attributes[key];
            }
          }

          if (match) {
            return this.nodes[i];
          }
        }
      }
    }

    return null;
  },

// protected

  // runs the tools check
  _checkTools: function() {
    var tools = this.rte.tools, key;
    for (key in tools) {
      tools[key].check();
    }
  },

  // finds the nodes from the current selection to the bottom
  _findNodes: function() {
    var node   = this.rte.selection.element(),
        editor = this.rte.editor._,
        rte    = this.rte._,
        nodes  = [],
        tags   = [];

    this.nodes = [];
    this.tags  = [];

    while (node && node !== rte) {
      if (node.tagName) { // skipping the textual nodes
        nodes.unshift(node);
        tags.unshift(node.tagName);
      }

      node = node.parentNode;

      if (node === editor) {
        this.nodes = nodes;
        this.tags  = tags;
        break;
      }
    }
  },

  // catches the mousedown on the links
  _mousedown: function(event) {
    var link = event.target;

    if (link._.tagName === 'A') {
      event.stop();
      var index = link.get('data-index').toInt(),
          node  = this.nodes[index];

      this.rte.selection.wrap(node);
    }
  }

});

/**
 * Custom undo/redo manager
 *
 * The basic trouble is that the native undo manager
 * dosn't support manual changes in the editable block
 * plus it lacks an ability to save some things under IE
 *
 * So we manage our undo/redo states manually by whatching
 * the 'change' event in the RTE instance
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Undoer = new Class({

  /**
   * Basic constructor
   *
   * @param Rte rte
   * @return void
   */
  initialize: function(rte) {
    this.rte = rte;

    function save() { this.undoer.save(); }
    this.rte.on({ focus:  save, change: save });

    this.clear();
  },

  /**
   * Clears up the undo history
   *
   * @return void
   */
  clear: function() {
    this.stash = [];
    this.index = -1;
  },

  /**
   * Checks if there are undo steps
   *
   * @return boolean check result
   */
  hasUndo: function() {
    return this.stash.length > 0 && this.index > 0;
  },

  /**
   * Checks if there are redo steps
   *
   * @return boolean check result
   */
  hasRedo: function() {
    return (this.stash.length - this.index) > 1;
  },

  /**
   * Moves the history one step back
   *
   * @return void
   */
  undo: function() {
    if (this.hasUndo()) {
      this.set(-- this.index);
    }
  },

  /**
   * Moves the histor one step forward
   *
   * @return void
   */
  redo: function() {
    if (this.hasRedo()) {
      this.set(++ this.index);
    }
  },

  /**
   * Sets the history to the given step
   *
   * @param Integer step index
   */
  set: function(index) {
    if (this.stash[this.index]) {
      this.rte.editor.update(this.stash[this.index]);
      this.rte.selection.restore();
    }
  },

  /**
   * Saves the current state of the editor
   *
   * @param Event the RTE's 'change' event with the 'tool' reference
   * @return void
   */
  save: function(event) {
    var tool  = event && event.tool,
        tools = this.rte.tools,
        html, html1, html2;

    if (!tool || (tool !== tools.Undo && tool !== tools.Redo)) {
      this.rte.selection.store();

      html = this.rte.editor._.innerHTML;

      // stripping off the selection markers
      html1 = html
        .replace(SELECTION_START_RE, '')
        .replace(SELECTION_END_RE, '');

      html2 = (this.stash[this.index]||'')
        .replace(SELECTION_START_RE, '')
        .replace(SELECTION_END_RE, '');

      if (html1 !== html2) {
        // cutting loose possible redo steps
        this.stash.length = this.index + 1;
        this.stash.push(html);
        this.index = this.stash.length - 1;

        tools.Undo.check();
        tools.Redo.check();
      }

      this.rte.selection.restore();
    }
  }

});

/**
 * This class handles the selection ranges
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Selection = new Class({

  /**
   * Basic constructor
   *
   * @param Rte rte
   * @return void
   */
  initialize: function(rte) {
    this.rte = rte;
  },

  /**
   * gets/sets the current range object
   *
   * @param {Range} to set
   * @return TextRange range
   */
  range: function(range) {
    var selection = window.getSelection && window.getSelection();

    if (range) {
      if (selection) { // w3c
        selection.removeAllRanges();
        selection.addRange(range);
      } else if (range._) { // IE
        range._.select();
      }

    } else {
      try {
        range = selection.getRangeAt(0); // FF, Opera, IE9
      } catch (e) {
        try {
          range = document.createRange(); // WebKit
        } catch (ee) {
          range = new IERangeEmulator(); // Old IE
        }
      }

      return range;
    }
  },

  /**
   * Returns the dom-node that's currently in focus
   *
   * @return raw dom-element
   */
  element: function() {
    var range = this.range(), node = range.commonAncestorContainer;

    // if there is a selection, trying those
    if (!range.collapsed) {
      if (
        range.startContainer === node           &&
        range.startOffset - range.endOffset < 2 &&
        range.startContainer.hasChildNodes()
      ) {
        node = range.startContainer.childNodes[range.startOffset];
      }
    }

    node = node && node.nodeType === 3 ? node.parentNode : node;

    return node || null;
  },

  /**
   * Puts current selection over the given raw dom-node
   *
   * @param raw dom-node
   * @return void
   */
  wrap: function(node) {
    var range = this.range();
    range.selectNode(node);
    this.range(range);
  },

  /**
   * Returns the selection text
   *
   * @return String selection text
   */
  text: function() {
    return this.range().toString();
  },

  /**
   * Cheks if the selection is empty
   *
   * @return boolean check result
   */
  empty: function() {
    return this.text() === '';
  },

  /**
   * Returns the HTML content of the selection
   *
   * @return String html content
   */
  html: function() {
    var range = this.range(), tmp, fragment;

    if (range._) {
      return range._.htmlText;
    } else {
      tmp = document.createElement('div');
      fragment = range.cloneContents();

      while (fragment.firstChild) {
        tmp.appendChild(fragment.firstChild);
      }

      return tmp.innerHTML;
    }
  },

  /**
   * executes a command on this editing area
   *
   * @param String command name
   * @param mixed command value
   * @return void
   */
  exec: function(command, value) {
    try {
      // it throws errors in some cases in the non-design mode
      document.execCommand(command, false, value);
    } catch(e) {
      // emulating insert html under IE
      if (command === 'inserthtml') {
        this.range()._.pasteHTML(value);
      }
    }
  },

  /**
   * Saves the selection by inserting special SPAN elements
   * in places where the selection starts and ends so that
   * it could be restored later, even if the editor innerHTML
   * property was manipulated directly
   *
   * @return {Rte.Selection} this
   */
  store: function() {
    var range = this.range();

    // reclonning the data so it wasn't lost on changes
    range = {
      startContainer: range.startContainer,
      startOffset:    range.startOffset,
      endContainer:   range.endContainer,
      endOffset:      range.endOffset,
      collapsed:      range.collapsed
    };

    /**
     * Places the selection markers into the editor's structure
     *
     * @param String name 'end' or 'start'
     * @return void
     */
    function place_marker(name) {
      var node   = range[name + 'Container'],
          offset = range[name + 'Offset'],
          marker = document.createElement('span'),
          parent = node.parentNode,
          text   = node.nodeValue,
          ending = document.createTextNode((''+text).substr(offset));

      marker.setAttribute('rrte-'+name, '1');

      function insert_after(content, anchor) {
        if (anchor.nextSibling) {
          anchor.parentNode.insertBefore(content, anchor.nextSibling);
        } else {
          anchor.parentNode.appendChild(content);
        }
      }

      if (node.nodeType === 3) { // text-node
        if (offset === 0) {
          parent.insertBefore(marker,
            // in case both of the markers are at the beginning of
            // the same node, the 'end' marker will be already there
            // and we will need to insert the 'start' one before it.
            name === 'start' && range.collapsed ? node.previousSibling : node
          );
        } else if (offset === text.length) {
          insert_after(marker, node);
        } else {   // splitting the text node in two
          node.nodeValue = text.substr(0, offset);
          insert_after(ending, node);
          parent.insertBefore(marker, ending);
        }

      } else if (node.nodeType === 1) { // elements
        if (offset === 0) {
          if (node.firstChild) {
            node.insertBefore(marker, node.firstChild);
          } else if (node.hasChildNodes()) {
            node.appendChild(marker);
          }
        } else if (offset === node.childNodes.length) {
          node.appendChild(marker);
        } else {
          node.insertBefore(marker, node.childNodes[offset]);
        }
      }
    }

    // NOTE: the 'end' should be before the 'start' in case both of them
    //       are in the same node, so that the offest didn't shift after
    //       we insert the marker nodes
    place_marker('end');
    place_marker('start');
  },

  /**
   * Restores the selection by previously placed
   * special SPAN elements and removes them afterwards
   *
   * @return {Rte.Selection} this
   */
  restore: function() {
    var elements = $A(this.rte.editor._.getElementsByTagName('span')),
        i=0, method, parent, offset, range = this.range();

    for (; i < elements.length; i++) {
      method = elements[i].getAttribute('rrte-start') ? 'setStart' :
               elements[i].getAttribute('rrte-end') ? 'setEnd' : false;

      if (method) {
        parent = elements[i].parentNode;

        if (range._) {
          range[method](elements[i]);
        } else {
          offset = IER_getIndex(elements[i]);
          range[method](parent, offset);
        }

        parent.removeChild(elements[i]);
      }
    }

    this.range(range);
  }
});

var

SELECTION_START_MARKER = '<span rrte-start="1"></span>',
SELECTION_END_MARKER   = '<span rrte-end="1"></span>',
SELECTION_START_RE     = new RegExp(RegExp.escape(SELECTION_START_MARKER), 'i'),
SELECTION_END_RE       = new RegExp(RegExp.escape(SELECTION_END_MARKER), 'i');


/**
 * W3C ranges API emulator for the old IE browsers
 *
 * Based on the `InternetExplorerRange` implementation
 * from the http://mozile.mozdev.org project
 * by James A. Overton <james@overton.ca>
 *
 * Originally licensed under MPL/GPL2/LGPL2 licenses
 *
 * Copyrgiht (C) 2011 Nikolay Nemshilov
 */
var IERangeEmulator = new Class({

  // standard w3c attributes
  collapsed:               null,
  startContainer:          null,
  startOffset:             null,
  endContainer:            null,
  endOffset:               null,
  commonAncestorContainer: null,

  /**
   * Basic constructor
   *
   * @return void
   */
  initialize: function() {
    this._ = document.selection.createRange();

    if (document.selection.type === 'Control') {
      this.startContainer = this.endContainer = this.commonAncestorContainer = this._(0);
      this.startOffset    = this.endOffset    = 0;
    } else {
      //startPoint
      var range = this._.duplicate();
      range.collapse(true);
      range = IER_getPosition(range);

      this.startContainer = range.node;
      this.startOffset    = range.offset;

      //endPoint
      range = this._.duplicate();
      range.collapse(false);
      range = IER_getPosition(range);

      this.endContainer = range.node;
      this.endOffset    = range.offset;

      // the rest of the properties
      IER_commonAncestorContainer(this);
    }

    IER_collapsed(this);
  },

  /**
   * Sets the starting point for the range
   *
   * @param {Node} node
   * @param {Number} offset
   * @return void
   */
  setStart: function(node, offset) {
    var range = this._.duplicate();

    range.moveToElementText(node);
    range.collapse(true);

    this._.setEndPoint('StartToStart', range);

    this.startContainer = node;
    this.startOffset    = offset;

    if (this.endContainer === null && this.endOffset === null) {
      this.endContainer = node;
      this.endOffset    = offset;
    }

    IER_commonAncestorContainer(this);
    IER_collapsed(this);
  },

  /**
   * Setting the end point for the range
   *
   * @param {Node} node
   * @param {Number} offset
   * @return void
   */
  setEnd: function (node, offset) {
    var range = this._.duplicate();

    range.moveToElementText(node);
    range.collapse(true);

    this._.setEndPoint('EndToEnd', range);

    this.endContainer = node;
    this.endOffset    = offset;

    if (this.startContainer === null && this.startOffset === null) {
      this.startContainer = node;
      this.startOffset    = offset;
    }

    IER_commonAncestorContainer(this);
    IER_collapsed(this);
  },

  /**
   * Wrapps the range around the given node
   *
   * @param {Node} node
   * @return void
   */
  selectNode: function (node) {
    this._.moveToElementText(node);
  },

  /**
   * Selection text emulation
   *
   * @return {String} text
   */
  toString: function() {
    return ''+ this._.text;
  }
});


//////////////////////////////////////////////////////////////////////////////
// Some private methods for the IERangeEmaulator
//////////////////////////////////////////////////////////////////////////////


/**
 * Finds the standard node/offset pair out of the
 * IE range object
 *
 * @param {TextRange} ie text range object
 * @return {Object} 'node' and 'offset' pairs
 */
function IER_getPosition(original_range) {
  var element = original_range.parentElement(),
      range, range_size, direction, node, node_size;

  range = document.body.createTextRange();
  range.moveToElementText(element);
  range.setEndPoint("EndToStart", original_range);

  range_size = range.text.length;

  // Choose Direction
  if (range_size < element.innerText.length / 2) {
    direction = 1;
    node = element.firstChild;
  } else {
    direction = -1;
    node = element.lastChild;
    range.moveToElementText(element);
    range.setEndPoint("StartToStart", original_range);
    range_size = range.text.length;
  }

  // Loop through child nodes
  while (node) {
    switch (node.nodeType) {
      case 3: // text-node
        node_size = node.data.length;
        if(node_size < range_size) {
          range_size -= node_size;

          if (direction === 1) {
            range.moveStart("character", range_size);
          } else {
            range.moveEnd("character", -range_size);
          }

        } else {
          return direction === 1 ?
            {node: node, offset: range_size} :
            {node: node, offset: node_size - range_size};
        }
        break;

      case 1: // element-node
        node_size = node.innerText.length;

        if (direction === 1) {
          range.moveStart("character", node_size);
        } else {
          range.moveEnd("character", -node_size);
        }

        range_size = range_size - node_size;
        break;
    }

    node = direction === 1 ? node.nextSibling : node.previousSibling;
  }

  // The TextRange was not found. Return a reasonable value instead.
  return {node: element, offset: 0};
}

/**
 * Assigns a suitable `commonAncestorContainer` property for the range
 *
 * @param {IERangeEmulator} range
 * @return void
 */
function IER_commonAncestorContainer(range) {
  range.commonAncestorContainer = range._.parentElement();
}

/**
 * Sets the `collapsed` property for the range
 *
 * @param {IERangeEmulator} range
 * return void
 */
function IER_collapsed(range) {
  range.collapsed =
    range.startContainer === range.endContainer &&
    range.startOffset    === range.endOffset;
}

/**
 * Finds out the node's index among it's siblings
 *
 * @param {Node} node
 * @return {Number} index
 */
function IER_getIndex(node) {
  var index = 0;

  while ((node = node.previousSibling)) {
    index ++;
  }

  return index;
}


/**
 * The basic tools class
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tool = new Class(Element, {

  block:    true,  // should the 'keypress' event be blocked
  blip:     false, // whether it should be highlighted when used
  changes:  true,  // if this tool should fire 'change' on the rte

  shortuct: null,  // the shortuct key-code
  shiftKey: false, // if it should trigger with a shift-key only

  /**
   * Basic constructor
   *
   * @param Rte rte reference
   * @return Rte.Tool this
   */
  initialize: function(rte) {
    var name;

    // searching for the tool-name
    for (name in Rte.Tools) {
      if (Rte.Tools[name] === this.constructor) { break; }
    }

    this.name = name;
    this.shortcut = this.shortcut || Rte.Shortcuts[name];

    this.$super('div', {
      'html':  '<div class="icon"></div>', // <- icon container
      'class': 'tool '+ name.toLowerCase(),
      'title': (Rte.i18n[name] || name) + (
        this.shortcut ? " ("+ this.shortcut + ")" : ""
      )
    });

    // registering the tool
    this.rte = rte;
    rte.tools[name] = this;

    // hooking up the shortcuts
    this.shortcut = this.shortcut && this.shortcut.toLowerCase();
    this.shiftKey = this.shortcut && this.shortcut.indexOf('shift') > -1;
    this.shortcut = this.shortcut && this.shortcut.toUpperCase().charCodeAt(this.shortcut.length - 1);

    // connecting the mousedown the way that the editor din't loose the focus
    this.onMousedown(function(e) {
      e.stop(); this.mousedown();
    });

    // allowing some nice chains in the subclass
    return this;
  },

  // those three methods should be implemented in subclasses
  exec:    function() {               }, /// the actual process
  active:  function() { return false; }, // all tools not active by default
  enabled: function() { return true;  }, // all tools enabled by default

  /**
   * The entry point for all the tools, if you need to call a tool,
   * call this method. __DON'T CALL__ the #exec method directly!
   *
   * @param {Event} 'keydown' event
   * @return void
   */
  call: function(event) {
    if (!this.disabled) {
      if (event && this.block) { event.stop(); }

      this.exec();
      this.rte.status.update();
      this.rte.fire('change', {tool: this});

      if (this.blip) { this.highlight(); }
    }
  },

  /**
   * Checks the command's status
   *
   * @return void
   */
  check: function() {
    this._.className = this._.className.replace(' disabled', '');
    this.disabled = false;

    if ((this.name === 'Source' || this.rte.srcMode !== true) && this.enabled()) {
      this._.className = this._.className.replace(' active', '');
      if (this.active()) {
        this._.className += ' active';
      }

    } else {
      this._.className += ' disabled';
      this.disabled = true;
    }
  },

  /**
   * Replacing the highlight method with some css stuff instead of an Fx
   *
   * @return Rte.Tool this
   */
  highlight: function() {
    R(this.addClass('highlight').removeClass).bind(this, 'highlight').delay(100);
  },

// protected

  // mousedown event receiver (might be replaced in subclasses)
  mousedown: function() {
    this.call();
  }

});

/**
 * Native command related abstract tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tool.Command = new Class(Rte.Tool, {
  command: null, // execCommand name
  value:   null, // execCommand value

  /**
   * calling the command exec
   *
   * @return void
   */
  exec: function() {
    this.rte.selection.exec(this.command, this.value);
  },

  /**
   * Checks if the command is enabled at all
   *
   * @return boolean check result
   */
  enabled: function() {
    try {
      return document.queryCommandEnabled(this.command);
    } catch(e) {
      return false;
    }
  },

  /**
   * Queries if the command is in active state
   *
   * @return boolean check result
   */
  active: function() {
    try { // copy/paste commands cause errors under FF by default
      return this.value === null ?
        document.queryCommandState(this.command) :
        document.queryCommandValue(this.command) == this.value;
    } catch(e) {
      return false;
    }
  }
});

/**
 * Text formatting specific abstract tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tool.Format = new Class(Rte.Tool, {
  tag:  null, // element's tag name
  atts: {},   // element's attributes

  /**
   * Formatting tools basic constructor
   *
   * @param Rte rte
   * @return Rte.ToolFormat this
   */
  initialize: function(rte) {
    this.$super(rte);
    this.tag = (this.tag || Rte.Tags[this.name] || '').toUpperCase();
    return this;
  },

  /**
   * triggering the formatting
   *
   * @return void
   */
  exec: function() {
    this[this.active() ? 'unformat' : 'format']();
  },

  /**
   * Overloading the activity checks
   *
   * @return boolean check result
   */
  active: function() {
    return this.element() !== null;
  },

// protected

  /**
   * Tries to find a currently active element
   *
   * @return raw dom element or null
   */
  element: function() {
    return this.rte.status.findElement(this.tag, this.attrs);
  },

  /**
   * Removes the formatting
   *
   * @return void
   */
  unformat: function() {
    this._format(false);
  },

  /**
   * Formats the block according to the settings
   *
   * @return void
   */
  format: function() {
    this._format(true);
  },

// private

  /**
   * The actual formatting/unformatting process mumbo-jumbo
   *
   * @param boolean formatting direction true/false
   * @return void
   */
  _format: function(formatting) {
    var open_tag  = '<'+  this.tag,
        close_tag = '</'+ this.tag + '>',
        editor    = this.rte.editor,
        selection = this.rte.selection,
        range     = selection.range(),
        selected  = selection.text(),
        element   = this.element(),
        content   = element && (element.textContent || element.innerText);

    // building the open-tag attributes
    for (var attr in this.attrs) {
      open_tag += ' '+ attr +'="'+ this.attrs[attr]+ '"';
    }
    open_tag += ">";

    selection.store();

    // Old IEs screw with the starting position
    // placing it before the open tag, so here we switch it back
    if (!formatting && range._) {
      editor.html(editor.html().replace(
        new RegExp(RegExp.escape(SELECTION_START_MARKER + open_tag), 'i'),
        open_tag + SELECTION_START_MARKER
      ));
    }


    if (formatting) {
      editor.html(editor.html()
        .replace(SELECTION_START_RE, open_tag + SELECTION_START_MARKER)
        .replace(SELECTION_END_RE,  SELECTION_END_MARKER + close_tag)
      );
    } else if (element && selected === content) {
      // plainly remove the element if it was fully selected
      // in case there are several nested elements so that
      // we didn't get screwed with the regexps manipulations
      editor.removeElement(element);
    } else {
      editor.html(editor.html()
        .replace(SELECTION_START_RE, close_tag + SELECTION_START_MARKER)
        .replace(SELECTION_END_RE, SELECTION_END_MARKER + open_tag)

        // cleaning up empty tags that might left
        .replace(/<([a-z]+)[^>]*?>\s*?<\/\1>/ig, '')
      );
    }

    selection.restore();
  }

});

/**
 * A _shared module_ to provide the `options` list functionality
 * for the tools
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tool.Options = {

  /**
   * Builds the options list
   *
   * @param Object key -> value hash
   */
  build: function(options) {
    this.trigger = $E('div', {'class': 'trigger', 'html': '&middot;'});
    this.display = $E('div', {'class': 'display'});
    this.options = $E('ul',  {'class': 'options'});

    this
      .addClass('with-options')
      .append(this.display, this.options)
      .insert(this.trigger, 'top');

    this.items = {};

    for (var value in options) {
      this.items[value] = $E('li').insert(options[value]);
      this.items[value].insertTo(this.options).value = value;
    }

    // creating an the reset value
    this.items[''] = $E('li', {'class': 'remove', 'html': '--', 'title': Rte.i18n.Remove});
    this.items[''].insertTo(this.options, 'top').value = '';

    // catching the clicks
    this.options.onMousedown(R(this.pick).bind(this));

    // hidding the menu when the user interacts with the document outside of the document
    var hide = R(this.options.hide).bind(this.options, null);
    $(document).on({mousedown: hide,
      keydown: function(event) {
        if (event.keyCode === 27) { hide(); }
      }
    });

    this.value = '';
    this.updateDisplay(null);

    return this;
  },

// protected

  // handling an option pick
  pick: function(event) {
    var target = event.stop().target;

    if (target._.tagName !== 'LI') {
      target = target.parent('LI');
    }

    if (target.value !== undefined) {
      this.options.hide();
      this.value = target.value;
      this.updateDisplay(this.value || null);
      this.markActive();
      this.exec();
    }
  },

  // toggling the menu on the icon-click
  mousedown: function() {
    if (!this.disabled) {
      $$('.rui-rte-toolbar div.with-options ul.options')
        .without(this.options).each('hide');

      // marking the current value so it could be seen
      if (this.options.hidden() && this.value !== null) {
        this.markActive();
      }

      this.options.toggle('fade', {duration: 'short'});
    }
  },

  // marks the currently active item
  markActive: function() {
    for (var item in this.items) {
      this.items[item][
        item === this.value ? 'addClass' : 'removeClass'
      ]('active');
    }
  },

  // updates the display
  updateDisplay: function(value) {
    this.display.update(
      value !== null && value in this.items ?
        this.items[value].text() : this._.title
    );
  }

};

/**
 * An abstract formatting tool that works with styles
 * basically it wraps a selection with a 'span' tag and then
 * handles the 'style' attribute, adds/removes various styles, etc.
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tool.Style = new Class(Rte.Tool.Format, {
  include: Rte.Tool.Options,

  tag:   'span',  // tag name of the element to be used
  style:  null,   // the style property name (dashed)

  /**
   * Joint constructor
   *
   * @param Rte rte
   * @return Rte.Tool.StyleOptions
   */
  initialize: function(rte, options) {
    // a regular expression to process the `style` property
    this.re    = new RegExp("(^|;)\\s*"+ RegExp.escape(this.style + ":")+ "\\s*(.+?)\\s*(;|$)");
    this.attrs = { style: this.re };

    this.$super(rte).build(options);

    return this;
  },

  /**
   * Triggers the action
   *
   * @return void
   */
  exec: function() {
    // unformatting if there is a value
    if (this.active()) {
      this.attrs = {style: this.style + ":" + this._value};
      this.unformat();
    }

    // formatting if some value was asked
    if (this.value) {
      this.attrs = {style: this.style + ":" + this.value};
      this.format();
    }

    // getting back the RE checker
    this.attrs = {style: this.re};
  },

  /**
   * Overloading the original method so that we could do some additional
   * features with the actual current value
   *
   * @return void
   */
  active: function() {
    var element = this.element(), active = false, value = null;

    if (element !== null) {
      this._value = value  = this.getStyleValue();
      active = true;
    }

    this.updateDisplay(value);

    return active;
  },

// protected

  /**
   * Finds the current style value (if any)
   *
   * @return string style value or null if nothing
   */
  getStyleValue: function() {
    var element = this.element();
    var attribute = element !== null ? element.getAttribute('style') : null;

    if (attribute !== null) {
      if ((attribute = attribute.match(this.re)) !== null) {
        attribute = attribute[2];
      }
    }

    return attribute;
  }
});

/**
 * an abstract color-picking tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tool.Color = new Class(Rte.Tool.Style, {

  extend: {
    COLORS: R([
      // TODO that's ain't no cool hacker's approach!
      '000000 444444 666666 999999 cccccc eeeeee f4f4f4 ffffff',
      'f24020 f79c33 fff84c 6af244 5ef9fd 0048f7 8950f7 ee5ff8',
      'e39e9b f5cba1 fee3a1 bcd3ab a6c3c8 a2c6e5 b1abd3 d0aabc '+
      'd77169 f1b374 fdd675 9cbe83 7ca4ae 74aad8 8983bf bb839f '+
      'cc0100 e79138 f1c332 69a84f 45818e 3d85c6 674ea7 a64d79 '+
      '990000 b45f05 bf9000 38761c 134f5c 0b5394 351b75 751a47 '+
      '660000 783e03 7f6000 264e13 0b333d 063763 1f124c 4c1030'
    ])
  },

  /**
   * Basic constructor, builds the colors picking panel
   *
   * @param Rte rte
   * @return Rte.Tool.Color this
   */
  initialize: function(rte) {
    this.$super(rte, {}).addClass('color');
    this.display.clean();

    // building the color picker menu
    Rte.Tool.Color.COLORS.each(function(line) {
      var group  = $E('li', {'class': 'group'}),
          list   = $E('ul').insertTo(group),
          colors = line.split(' '), i = 0, color, forecolor;

      for (; i < colors.length; i++) {
        color     = '#' + colors[i];
        forecolor = (parseInt('ffffff', 16) - parseInt(colors[i], 16)).toString(16);

        // IE will get screwed if the length of the color is less than 6
        while (forecolor.length < 6) {
          forecolor += '0';
        }

        this.items[color] = $E('li', {
          html:  '&bull;',
          style: {
            background: color,
            color: '#'+ forecolor
          }
        });

        this.items[color].insertTo(list).value = color;
      }

      this.options.append(group);
    }, this);

    return this;
  },

// protected

  /**
   * Overloading the property so that it converted any color formats
   * into a six chars hex value
   *
   * @return String current color or null
   */
  getStyleValue: function() {
    var color = this.$super(), match;

    if (color !== null) {
      if ((match = /^#(\w)(\w)(\w)$/.exec(color))) {
        color = "#"+ match[1]+match[1]+match[2]+match[2]+match[3]+match[3];
      } else if ((match = /^\s*rgb\((\d+),\s*(\d+),\s*(\d+)\)\s*$/.exec(color))) {
        color = "#"+ R(match.slice(1)).map(function(bit) {
          bit = (bit-0).toString(16);
          return bit.length === 1 ? '0'+bit : bit;
        }).join('');
      }
    }

    return color;
  },

  /**
   * Replacing the original method so that
   * it changed the color of the display thing
   * instead of text
   *
   * @param String value
   * @return void
   */
  updateDisplay: function(value) {
    this.display._.style.background = value === null ?
      'transparent' : value;
  }
});

/**
 * An abstract URL tool, used with the links, images, videos, etc.
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tool.Url = new Class(Rte.Tool, {
  attr:   null, // the url-attribute 'src', 'href', etc.

  exec: function(url) {
    if (url === undefined) {
      this.prompt();
    } else {
      if (url) {
        this[this.element() ? 'url' : 'create'](url);
      } else {
        this.rte.editor.removeElement(this.element());
      }
    }
  },

  active: function() {
    return this.element() !== null;
  },

  prompt: function() {
    var url = prompt(Rte.i18n.UrlAddress, this.url() || 'http://some.url.com');

    if (url !== null) {
      this.exec(url);
    }
  },

// protected

  url: function(url) {
    if (this.element()) {
      if (url !== undefined) {
        this.element()[this.attr] = url;
      } else {
        return this.element()[this.attr];
      }
    }
  },

  create: function(url) {
    this.rte.selection.exec(this.command, url);
  }
});

/**
 * Making the things bold tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Bold = new Class(Rte.Tool.Format, {
});

/**
 * Making the things italic
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Italic = new Class(Rte.Tool.Format, {
});

/**
 * Making the things underline tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Underline = new Class(Rte.Tool.Format, {
});

/**
 * Making the things strike through
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Strike = new Class(Rte.Tool.Format, {
});

/**
 * The cut tool
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Cut = new Class(Rte.Tool.Command, {
  command:  'cut',
  block:    false,
  blip:     true
});

/**
 * The copy tool
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Copy = new Class(Rte.Tools.Cut, {
  command: 'copy'
});

/**
 * The paste action
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Paste = new Class(Rte.Tools.Cut, {
  command:  'paste'
});

/**
 * The paste action
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Pastetext = new Class(Rte.Tools.Command, {
  command:  'paste'
});

/**
 * the 'left' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Left = new Class(Rte.Tool.Command, {
  command: 'justifyleft'
});

/**
 * the 'center' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Center = new Class(Rte.Tool.Command, {
  command: 'justifycenter'
});

/**
 * the 'right' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Right = new Class(Rte.Tool.Command, {
  command: 'justifyright'
});

/**
 * the 'justify' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Justify = new Class(Rte.Tool.Command, {
  command: 'justifyfull'
});

/**
 * The undo tool
 *
 * The actual magic happens in the Rte.Undoer class
 * here we just show the status and blip it when used
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Undo = new Class(Rte.Tool, {
  blip: true,

  exec: function() {
    this.rte.undoer.undo();
  },

  enabled: function() {
    return this.rte.undoer.hasUndo();
  }
});

/**
 * the 'redo' tool
 *
 * The actual magic happens in the Rte.Undoer class
 * here we just show the status and blip it when used
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Redo = new Class(Rte.Tool, {
  blip: true,

  exec: function() {
    this.rte.undoer.redo();
  },

  enabled: function() {
    return this.rte.undoer.hasRedo();
  }

});

/**
 * The code block tool
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Code = new Class(Rte.Tool.Format, {
});

/**
 * The block quote tool
 *
 * Copyrigth (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Quote = new Class(Rte.Tool.Format, {
});

/**
 * the 'ttext' tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Ttext = new Class(Rte.Tool.Format, {
});

/**
 * The header block tool
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Header = new Class(Rte.Tool.Format, {
});

/**
 * The the url link tool
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Link = new Class(Rte.Tool.Url, {
  command:  'createlink',
  attr:     'href',

  enabled: function() {
    return !this.rte.selection.empty() || this.active();
  },

  element: function() {
    return this.rte.status.findElement('A', {});
  }
});

/**
 * The image tool
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Image = new Class(Rte.Tool.Url, {
  command: 'insertimage',
  attr:    'src',

  element: function() {
    var image = this.rte.selection.element();
    return image !== null && image.tagName === "IMG" ? image : null;
  }

});

/**
 * the 'video' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Video = new Class(Rte.Tool.Url, {
  command: 'inserthtml',

  enabled: function() {
    return true; // always enabled
  },

  element: function() {
    return this.rte.status.findElement('OBJECT', {});
  },

  // works with url address of the embedded object
  url: function(url) {
    var element = this.element() && this.element().getElementsByTagName('embed')[0];

    if (element) {
      if (url !== undefined) {
        element.src = this.swfUrl(url);
      } else {
        return element.src;
      }
    }
  },

  // creates the actual object block
  create: function(url) {
    var swf_url = this.swfUrl(url),
        size =  'width="'+  this.rte.options.videoSize.split('x')[0] +
              '" height="'+ this.rte.options.videoSize.split('x')[1] + '"';

    this.$super('<object '+ size +'>'+
      '<param name="src" value="'+ swf_url +'" />'+
      '<embed src="'+ swf_url +'" type="application/x-shockwave-flash" '+ size + ' />' +
    '</object>');
  },

  // making the actual 'swf' url
  swfUrl: function(url) {
    return R(Rte.Videos).map(function(desc) {
      return url.match(desc[0]) ?
        url.replace(desc[0], desc[1]) : null;
    }).compact()[0] || url;
  }
});

/**
 * the 'dotlist' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Dotlist = new Class(Rte.Tool.Command, {
  command: 'insertunorderedlist'
});

/**
 * the 'numlist' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Numlist = new Class(Rte.Tool.Command, {
  command: 'insertorderedlist'
});

/**
 * the 'indent' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Indent = new Class(Rte.Tool.Command, {
  command: 'indent'
});

/**
 * the 'Outdent' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Outdent = new Class(Rte.Tool.Command, {
  command: 'outdent'
});

/**
 * the 'forecolor' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Forecolor = new Class(Rte.Tool.Color, {
  style: 'color'
});

/**
 * the 'backcolor' tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Backcolor = new Class(Rte.Tool.Color, {
  style: 'background-color'
});

/**
 * The source tool
 *
 * Copyrigth (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Source = new Class(Rte.Tool, {
  source:   false, // the textarea element reference

  exec: function() {
    this[this.rte.srcMode ? 'showPreview' : 'showSource']();
    this.rte.srcMode = !this.rte.srcMode;
  },

  active: function() {
    return this.rte.srcMode;
  },

// protected

  showPreview: function() {
    this.rte.editor.setStyle('visibility:visible');
    if (this.source) {
      this.rte.value(this.source.value());
      this.source.remove();
    }

    this.rte.editor.focus();
  },

  showSource: function() {
    this.rte.editor.setStyle('visibility:hidden;');

    (
      this.source = this.source ||
      $E('textarea', {'class': 'rui-rte-source'})
    )
    .insertTo(this.rte.editor, 'before')
    .resize(this.rte.editor.size())
    .setValue('' + this.rte.value())
    .onKeydown(function(event) {
      var raw = event._;

      if (raw.metaKey || raw.ctrlKey) {
        if (raw.keyCode == 69) { // Ctrl+E
          event.stop();
          this.exec();
        }
      }
    }.bind(this))
    .focus();

    this.rte.focused = true;

    this.rte.status.update();

    // locking all the tools
    for (var name in this.rte.tools) {
      if (this.rte.tools[name] !== this) {
        this.rte.tools[name].addClass('disabled');
      }
    }
  }
});

/**
 * the 'clear' tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Clear = new Class(Rte.Tool, {

  exec: function() {
    this.rte.editor.clean();
  }

});

/**
 * the 'save' tool
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
Rte.Tools.Save = new Class(Rte.Tool, {

  initialize: function(rte) {
    this.$super(rte);
    if (!(rte.textarea && rte.textarea.form())) {
      this.disabled = true;
      this.addClass('disabled');
    }
  },

  exec: function() {
    if (!this.disabled) {
      this.rte.textarea.form().submit();
    }
  },

  check: function() {} // checked in the constructor
});

/**
 * Formatting style tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Format = new Class(Rte.Tool.Format, {
  include: Rte.Tool.Options,

  /**
   * Constructor, builds the options list and
   * defines the formats index for quick access
   *
   * @param Rte rte
   */
  initialize: function(rte) {
    var options = {}, rule, tag, match;

    this.formats = {};

    for (rule in Rte.Formats) {
      if ((match = rule.match(/^([a-z0-9]+)(\.([a-z0-9_\-]+))?$/))) {
        tag = match[1];

        this.formats[rule] = { tag: tag.toUpperCase(), attrs: {}, match: {} };

        if (match[3]) {
          this.formats[rule]['attrs']['class'] = match[3];
          this.formats[rule]['match']['class'] = new RegExp(
            '(^|\\s+)'+ RegExp.escape(match[3]) + '(\\s+|$)'
          );
        }

        options[rule] = '<'+ tag + ' class="'+ match[3] + '">'+
          Rte.Formats[rule] + '</'+ tag + '>';
      }
    }

    this.$super(rte).build(options);

    return this;
  },

  /**
   * Handling the formatting
   *
   * @return void
   */
  exec: function() {
    // removing the formatting first
    if (this.active() && this.rule) {
      this.tag   = this.formats[this.rule].tag;
      this.attrs = this.formats[this.rule].attrs;
      this.unformat();
    }

    // applying a new formatting if needed
    if (this.value && this.formats[this.value]) {
      this.tag   = this.formats[this.value].tag;
      this.attrs = this.formats[this.value].attrs;
      this.format();
    }
  },

  /**
   * Overloading the original method so that it updated the
   * currently used formatting style in the display area
   *
   * @return boolean check result
   */
  active: function() {
    var active = this.element() !== null;
    this.updateDisplay(this.rule);
    return active;
  },

// protected

  // overloading the original method to handle multiple options
  element: function() {
    var rule, element, status = this.rte.status;

    for (rule in this.formats) {
      element = status.findElement(
        this.formats[rule]['tag'],
        this.formats[rule]['match']
      );

      if (element !== null) {
        this.rule = rule;
        return element;
      }
    }

    return (this.rule = null);
  }



});

/**
 * Font-name options tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Fontname = new Class(Rte.Tool.Style, {
  style: 'font-family',

  /**
   * Basic constructor
   *
   * @param Rte rte
   */
  initialize: function(rte) {
    var options = {}, name, fonts = Rte.FontNames;

    for (name in fonts) {
      options[fonts[name]] = '<div style="font-family:'+
        fonts[name]+ '">' + name + '</div>';
    }

    return this.$super(rte, options);
  }
});


/**
 * The font-size tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Fontsize = new Class(Rte.Tool.Style, {
  style: 'font-size',

  /**
   * Basic constructor
   *
   * @param Rte rte
   */
  initialize: function(rte) {
    var options = {}, i=0, sizes = Rte.FontSizes.split(/\s+/);

    for (; i < sizes.length; i++) {
      options[sizes[i]] = '<div style="font-size:'+
        sizes[i] +'">'+ sizes[i] + '</div>';
    }

    return this.$super(rte, options);
  }
});

/**
 * The subscript tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Subscript = new Class(Rte.Tool.Command, {
  command: 'subscript'
});

/**
 * The superscript tool
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Rte.Tools.Superscript = new Class(Rte.Tool.Command, {
  command: 'superscript'
});

/**
 * Document level hooks for the Rte widget
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
$(document).onReady(function() {
  $$(Rte.Options.cssRule).each('getRich');
});

/**
 * Input level hooks to convert textareas into RTE
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
Input.include({
  /**
   * converts an input field into a RTE
   *
   * @param Object options
   * @return Rte editor
   */
  getRich: function(options) {
    if (this._.type === 'textarea' && !this.rte) {
      this.rte = new Rte(this, options);
    }

    return this.rte;
  }
});

var embed_style = document.createElement('style'),                 
    embed_rules = document.createTextNode("div.rui-rte,div.rui-rte-toolbar,div.rui-rte-toolbar *,div.rui-rte-editor,div.rui-rte-status,div.rui-rte-status *{margin:0;padding:0;background:none;border:none;width:auto;height:auto}textarea[data-rte]{visibility:hidden}div.rui-rte{display:inline-block; *display:inline; *zoom:1;position:relative}div.rui-rte-toolbar{padding:.15em .3em;background:#eee;border-radius:.25em .25em 0 0;-moz-border-radius:.25em .25em 0 0;-webkit-border-radius:.25em .25em 0 0;border:1px solid #ccc;border-bottom:none}div.rui-rte-toolbar div.line{display:inline-block; *display:inline; *zoom:1;margin-bottom:1px}div.rui-rte-toolbar div.bar{display:inline-block; *display:inline; *zoom:1;margin-right:2px}div.rui-rte-toolbar div.tool{display:inline-block; *display:inline; *zoom:1;margin-right:1px;vertical-align:middle;position:relative;cursor:pointer;border:1px solid #bbb;background-image:url(/images/rightjs-ui/rte.png);background-position:0px 0px;background-color:#fff;border-radius:.25em;-moz-border-radius:.25em;-webkit-border-radius:.25em}div.rui-rte-toolbar div.tool:hover{background-color:#ddd;border-color:#777}div.rui-rte-toolbar div.active{background-position:-20px 0px;background-color:#eee;border-color:#666;box-shadow:#aaa .025em .025em .5em;-moz-box-shadow:#aaa .025em .025em .5em;-webkit-box-shadow:#aaa .025em .025em .5em}div.rui-rte-toolbar div.disabled,div.rui-rte-toolbar div.disabled:hover{opacity:.4;filter:alpha(opacity=40);background-position:-40px 0px;background-color:#eee;border-color:#aaa;cursor:default}div.rui-rte-toolbar div.highlight{background-color:#BBB;border-color:#666}div.rui-rte-toolbar div.icon{height:20px;width:20px;background-image:url(/images/rightjs-ui/rte.png);background-repeat:no-repeat;background-position:20px 20px}div.rui-rte-toolbar div.save div.icon{background-position:0px -20px}div.rui-rte-toolbar div.clear div.icon{background-position:-20px -20px}div.rui-rte-toolbar div.source div.icon{background-position:-40px -20px}div.rui-rte-toolbar div.bold div.icon{background-position:0px -40px}div.rui-rte-toolbar div.italic div.icon{background-position:-20px -40px}div.rui-rte-toolbar div.underline div.icon{background-position:-40px -40px}div.rui-rte-toolbar div.strike div.icon{background-position:-60px -40px}div.rui-rte-toolbar div.cut div.icon{background-position:0px -60px}div.rui-rte-toolbar div.copy div.icon{background-position:-20px -60px}div.rui-rte-toolbar div.paste div.icon{background-position:-40px -60px}div.rui-rte-toolbar div.pastetext div.icon{background-position:-60px -60px}div.rui-rte-toolbar div.left div.icon{background-position:0px -80px}div.rui-rte-toolbar div.center div.icon{background-position:-20px -80px}div.rui-rte-toolbar div.right div.icon{background-position:-40px -80px}div.rui-rte-toolbar div.justify div.icon{background-position:-60px -80px}div.rui-rte-toolbar div.undo div.icon{background-position:0px -100px}div.rui-rte-toolbar div.redo div.icon{background-position:-20px -100px}div.rui-rte-toolbar div.quote div.icon{background-position:0px -120px}div.rui-rte-toolbar div.code div.icon{background-position:-20px -120px}div.rui-rte-toolbar div.ttext div.icon{background-position:-40px -120px}div.rui-rte-toolbar div.header div.icon{background-position:-60px -120px}div.rui-rte-toolbar div.image div.icon{background-position:0px -140px}div.rui-rte-toolbar div.link div.icon{background-position:-20px -140px}div.rui-rte-toolbar div.video div.icon{background-position:-40px -140px}div.rui-rte-toolbar div.dotlist div.icon{background-position:0px -160px}div.rui-rte-toolbar div.numlist div.icon{background-position:-20px -160px}div.rui-rte-toolbar div.indent div.icon{background-position:-40px -160px}div.rui-rte-toolbar div.outdent div.icon{background-position:-60px -160px}div.rui-rte-toolbar div.forecolor div.icon{background-position:0px -180px}div.rui-rte-toolbar div.backcolor div.icon{background-position:-20px -180px}div.rui-rte-toolbar div.symbol div.icon{background-position:0px -200px}div.rui-rte-toolbar div.subscript div.icon{background-position:-20px -200px}div.rui-rte-toolbar div.superscript div.icon{background-position:-40px -200px}div.rui-rte-toolbar div.with-options{padding-right:8px}div.rui-rte-toolbar div.with-options div.trigger{position:absolute;right:0;height:100%;width:7px;text-align:center;background:#ccc;border-left:1px solid #bbb}div.rui-rte-toolbar div.bar div:hover div.trigger,div.rui-rte-toolbar div.bar div.active div.trigger{background:#aaa;border-color:#888}div.rui-rte-toolbar div.with-options div.icon{display:none}div.rui-rte-toolbar div.with-options div.display{display:block;line-height:20px;padding:0 6px;margin:0;color:#222;font-size:12px;background:#f8f8f8}div.rui-rte-toolbar div.with-options ul.options,div.rui-rte-toolbar div.with-options ul.options li{list-style:none;margin:0;padding:0}div.rui-rte-toolbar div.with-options ul.options{display:none;cursor:default;position:absolute;margin-bottom:1px;margin-left:-1px;background:#fff;border:1px solid #aaa;border-radius:.25em;-moz-border-radius:.25em;-webkit-border-radius:.25em;box-shadow:#bbb .1em .1em .25em;-moz-box-shadow:#bbb .1em .1em .25em;-webkit-box-shadow:#bbb .1em .1em .25em}div.rui-rte-toolbar div.with-options ul.options li{padding:.2em .5em;white-space:nowrap;cursor:pointer}div.rui-rte-toolbar div.with-options ul.options li:hover{background-color:#eee}div.rui-rte-toolbar div.with-options ul.options li> *{margin:0;padding:0;border:none;position:static}div.rui-rte-toolbar div.color div.icon{display:block}div.rui-rte-toolbar div.color ul.options{padding-bottom:.5em}div.rui-rte-toolbar div.color ul.options li.group,div.rui-rte-toolbar div.color ul.options li.group:hover{background:none}div.rui-rte-toolbar div.color ul.options li.group ul{width:144px;clear:both;padding-top:.5em}div.rui-rte-toolbar div.color ul.options li.group ul li{float:left;width:16px;height:16px;line-height:16px;font-size:80%;text-align:center;text-indent:-9999em;padding:0;cursor:pointer;border:1px solid transparent}div.rui-rte-toolbar div.color ul.options li.group ul li:hover,div.rui-rte-toolbar div.color ul.options li.group ul li.active{background:none;border-color:#444;border-radius:.1em;-moz-border-radius:.1em;-webkit-border-radius:.1em}div.rui-rte-toolbar div.color ul.options li.group ul li.active{text-indent:0}div.rui-rte-toolbar div.color div.display{position:absolute;text-indent:-9999em;bottom:2px;left:3px;margin:0;padding:0;width:14px;height:4px;border-radius:.1em;-moz-border-radius:.1em;-webkit-border-radius:.1em}div.rui-rte-toolbar div.color ul.options li.group ul li.none{border-color:#444}div.rui-rte-toolbar div.color ul.options li.group ul li.label,div.rui-rte-toolbar div.color ul.options li.group ul li.label:hover{text-indent:0;border:none;margin-left:.5em;font-size:1em;cursor:default}div.rui-rte-editor{outline:none;outline:hidden;padding:.1em .3em;overflow:auto;background:white;border:1px solid #ccc}div.rui-rte-editor:focus{border-color:#aaa}div.rui-rte-editor> *:first-child{margin-top:0}div.rui-rte-editor> *:last-child{margin-bottom:0}div.rui-rte textarea.rui-rte-source{position:absolute;outline:none;resize:none}div.rui-rte-status{font-size:90%;height:1.4em;padding:0 .5em;color:#888;background:#eee;border:1px solid #ccc;border-top:none;border-radius:0 0 .25em .25em;-moz-border-radius:0 0 .25em .25em;-webkit-border-radius:0 0 .25em .25em}");      
                                                                   
embed_style.type = 'text/css';                                     
document.getElementsByTagName('head')[0].appendChild(embed_style); 
                                                                   
if(embed_style.styleSheet) {                                       
  embed_style.styleSheet.cssText = embed_rules.nodeValue;          
} else {                                                           
  embed_style.appendChild(embed_rules);                            
}                                                                  


return Rte;
})(RightJS, document, window);
