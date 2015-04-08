/**
 * Drag'n'Drop module v2.2.4
 * http://rightjs.org/plugins/drag-n-drop
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
(function(window, document, RightJS) {
/**
 * The DND module initialization script
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var R        = RightJS,
    $        = RightJS.$,
    $w       = RightJS.$w,
    Class    = RightJS.Class,
    isHash   = RightJS.isHash,
    isArray  = RightJS.isArray,
    Element  = RightJS.Element,
    Observer = RightJS.Observer;



/**
 * Draggable unit
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
var Draggable = new Class(Observer, {
  extend: {
    version: '2.2.4',

    EVENTS: $w('before start drag stop drop'),

    Options: {
      handle:            null,        // a handle element that will start the drag

      snap:              0,           // a number in pixels or [x,y]
      axis:              null,        // null or 'x' or 'y' or 'vertical' or 'horizontal'
      range:             null,        // {x: [min, max], y:[min, max]} or reference to another element

      dragClass:         'dragging',  // the in-process class name

      clone:             false,       // if should keep a clone in place
      revert:            false,       // marker if the object should be moved back on finish
      revertDuration:    'normal',    // the moving back fx duration

      scroll:            true,        // if it should automatically scroll
      scrollSensitivity: 32,          // the scrolling area size in pixels

      zIndex:            10000000,    // the element's z-index
      moveOut:           false,       // marker if the draggable should be moved out of it's context (for overflown elements)

      relName:           'draggable'  // the audodiscovery feature key
    },

    // referenece to the currently active draggable
    current: null,

    // scans the document for auto-processed draggables with the rel="draggable" attribute
    rescan: function(scope) {
      var key = this.Options.relName, ref = this === Draggable ? 'draggable' : 'droppable';

      ($(scope)||$(document)).find('*[rel^="'+key+'"]').each(function(element) {
        if (!element[ref]) {
          new this(element, new Function('return '+element.get('data-'+key))() || {});
        }
      }, this);
    }
  },

  /**
   * Basic controller
   *
   * @param mixed element reference
   * @param Object options
   */
  initialize: function(element, options) {
    this.element = $(element);
    this.$super(options);

    this._dragStart = R(function(event) {
      if (event.which === 1) {
        this.dragStart(event);
      }
    }).bind(this);
    this.handle.on({
      mousedown:  this._dragStart,
      touchstart: this._dragStart
    });

    this.element.draggable = this;
  },

  /**
   * detaches the mouse observers out of the draggable element
   *
   * @return this
   */
  destroy: function() {
    this.handle
      .stopObserving('mousedown',  this._dragStart)
      .stopObserving('touchstart', this._dragStart);
    delete(this.element.draggable);

    return this;
  },

  // additional options processing
  setOptions: function(options) {
    this.$super(options);

    // checking the handle
    this.handle = this.options.handle ? $(this.options.handle) : this.element;

    // checking the spappings
    if (isArray(this.options.snap)) {
      this.snapX = this.options.snap[0];
      this.snapY = this.options.snap[1];
    } else {
      this.snapX = this.snapY = this.options.snap;
    }

    return this;
  },

  /**
   * Moves the element back to the original position
   *
   * @return this
   */
  revert: function() {
    var position  = this.clone.position();
    var end_style = {
      top:  (position.y + this.ryDiff) + 'px',
      left: (position.x + this.rxDiff) + 'px'
    };

    if (this.options.revertDuration && this.element.morph) {
      this.element.morph(end_style, {
        duration: this.options.revertDuration,
        onFinish: R(this.swapBack).bind(this)
      });
    } else {
      this.element.setStyle(end_style);
      this.swapBack();
    }

    return this;
  },

// protected

  // handles the event start
  dragStart: function(event) {
    if (this._drag) { return false; } else { this._drag = true; }

    this.fire('before', this, event.stop());

    // calculating the positions diff
    var position = this.element.position();

    this.xDiff = event.pageX - position.x;
    this.yDiff = event.pageY - position.y;

    // grabbing the relative position diffs for nested spaces
    this.rxDiff = this.ryDiff = 0;
    this.element.parents().reverse().each(function(parent) {
      if (parent.getStyle('position') !== 'static') {
        parent = parent.position();

        this.rxDiff = - parent.x;
        this.ryDiff = - parent.y;
      }
    }, this);

    // preserving the element sizes
    var size = {
      x: this.element.getStyle('width'),
      y: this.element.getStyle('height')
    };

    if (size.x == 'auto') { size.x = this.element._.offsetWidth  + 'px'; }
    if (size.y == 'auto') { size.y = this.element._.offsetHeight + 'px'; }

    // building a clone element if necessary
    if (this.options.clone || this.options.revert) {
      this.clone = new Element(this.element._.cloneNode(true)).setStyle({
        visibility: this.options.clone ? 'visible' : 'hidden'
      }).insertTo(this.element, 'before');
    }

    // reinserting the element to the body so it was over all the other elements
    this.element.setStyle({
      position: 'absolute',
      zIndex:   Draggable.Options.zIndex++,
      top:      (position.y + this.ryDiff) + 'px',
      left:     (position.x + this.rxDiff) + 'px',
      width:    size.x,
      height:   size.y
    }).addClass(this.options.dragClass);

    if (this.options.moveOut) {
      this.element.insertTo(document.body);
    }

    // caching the window scrolls
    this.winScrolls = $(window).scrolls();
    this.winSizes   = $(window).size();

    Draggable.current = this.calcConstraints().fire('start', this, event);

    this.style = this.element._.style;
  },

  // catches the mouse move event
  dragProcess: function(event) {
    var page_x = event.pageX, page_y = event.pageY, x = page_x - this.xDiff, y = page_y - this.yDiff;

    // checking the range
    if (this.ranged) {
      if (this.minX > x) { x = this.minX; }
      if (this.maxX < x) { x = this.maxX; }
      if (this.minY > y) { y = this.minY; }
      if (this.maxY < y) { y = this.maxY; }
    }

    // checking the scrolls
    if (this.options.scroll) {
      var scrolls = {x: this.winScrolls.x, y: this.winScrolls.y},
        sensitivity = this.options.scrollSensitivity;

      if ((page_y - scrolls.y) < sensitivity) {
        scrolls.y = page_y - sensitivity;
      } else if ((scrolls.y + this.winSizes.y - page_y) < sensitivity){
        scrolls.y = page_y - this.winSizes.y + sensitivity;
      }

      if ((page_x - scrolls.x) < sensitivity) {
        scrolls.x = page_x - sensitivity;
      } else if ((scrolls.x + this.winSizes.x - page_x) < sensitivity){
        scrolls.x = page_x - this.winSizes.x + sensitivity;
      }

      if (scrolls.y < 0) { scrolls.y = 0; }
      if (scrolls.x < 0) { scrolls.x = 0; }

      if (scrolls.y < this.winScrolls.y || scrolls.y > this.winScrolls.y ||
        scrolls.x < this.winScrolls.x || scrolls.x > this.winScrolls.x) {

          $(window).scrollTo(this.winScrolls = scrolls);
      }
    }

    // checking the snaps
    if (this.snapX) { x = x - x % this.snapX; }
    if (this.snapY) { y = y - y % this.snapY; }

    // checking the constraints
    if (!this.axisY) { this.style.left = (x + this.rxDiff) + 'px'; }
    if (!this.axisX) { this.style.top  = (y + this.ryDiff) + 'px'; }

    this.fire('drag', this, event);
  },

  // handles the event stop
  dragStop: function(event) {
    this.element.removeClass(this.options.dragClass);

    // notifying the droppables for the drop
    Droppable.checkDrop(event, this);

    if (this.options.revert) {
      this.revert();
    } else {
      this._drag = false;
    }

    Draggable.current = null;

    this.fire('stop', this, event);
  },

  // swaps the clone element to the actual element back
  swapBack: function() {
    if (this.clone) {
      this.clone.replace(
        this.element.setStyle({
          width:    this.clone.getStyle('width'),
          height:   this.clone.getStyle('height'),
          position: this.clone.getStyle('position'),
          zIndex:   this.clone.getStyle('zIndex') || ''
        })
      );
    }
    this._drag = false;
  },

  // calculates the constraints
  calcConstraints: function() {
    var axis = this.options.axis;
    this.axisX = R(['x', 'horizontal']).include(axis);
    this.axisY = R(['y', 'vertical']).include(axis);

    this.ranged = false;
    var range = this.options.range;
    if (range) {
      this.ranged = true;

      // if the range is defined by another element
      var element = $(range);
      if (element instanceof Element) {
        var dims = element.dimensions();

        range = {
          x: [dims.left, dims.left + dims.width],
          y: [dims.top,  dims.top + dims.height]
        };
      }

      if (isHash(range)) {
        var size = this.element.size();

        if (range.x) {
          this.minX = range.x[0];
          this.maxX = range.x[1] - size.x;
        }
        if (range.y) {
          this.minY = range.y[0];
          this.maxY = range.y[1] - size.y;
        }
      }
    }

    return this;
  }
});

/**
 * Droppable unit
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
var Droppable = new Class(Observer, {
  extend: {
    EVENTS: $w('drop hover leave'),

    Options: {
      accept:      '*',
      containment: null,    // the list of elements (or ids) that should to be accepted

      overlap:     null,    // 'x', 'y', 'horizontal', 'vertical', 'both'  makes it respond only if the draggable overlaps the droppable
      overlapSize: 0.5,     // the overlapping level 0 for nothing 1 for the whole thing

      allowClass:  'droppable-allow',
      denyClass:   'droppable-deny',

      relName:     'droppable'   // automatically discovered feature key
    },

    // See the Draggable rescan method, case we're kinda hijacking it in here
    rescan: Draggable.rescan,

    /**
     * Checks for hoverting draggable
     *
     * @param Event mouse event
     * @param Draggable draggable
     */
    checkHover: function(event, draggable) {
      for (var i=0, length = this.active.length; i < length; i++) {
        this.active[i].checkHover(event, draggable);
      }
    },

    /**
     * Checks for a drop
     *
     * @param Event mouse event
     * @param Draggable draggable
     */
    checkDrop: function(event, draggable) {
      for (var i=0, length = this.active.length; i < length; i++) {
        this.active[i].checkDrop(event, draggable);
      }
    },

    active: []
  },

  /**
   * Basic cosntructor
   *
   * @param mixed the draggable element reference
   * @param Object options
   */
  initialize: function(element, options) {
    this.element = $(element);
    this.$super(options);

    Droppable.active.push(this.element._droppable = this);
  },

  /**
   * Detaches the attached events
   *
   * @return self
   */
  destroy: function() {
    Droppable.active = Droppable.active.without(this);
    delete(this.element.droppable);
    return this;
  },

  /**
   * checks the event for hovering
   *
   * @param Event mouse event
   * @param Draggable the draggable object
   */
  checkHover: function(event, draggable) {
    if (this.hoveredBy(event, draggable)) {
      if (!this._hovered) {
        this._hovered = true;
        this.element.addClass(this.options[this.allows(draggable) ? 'allowClass' : 'denyClass']);
        this.fire('hover', draggable, this, event);
      }
    } else if (this._hovered) {
      this._hovered = false;
      this.reset().fire('leave', draggable, this, event);
    }
  },

  /**
   * Checks if it should process the drop from draggable
   *
   * @param Event mouse event
   * @param Draggable draggable
   */
  checkDrop: function(event, draggable) {
    this.reset();
    if (this.hoveredBy(event, draggable) && this.allows(draggable)) {
      draggable.fire('drop', this, draggable, event);
      this.fire('drop', draggable, this, event);
    }
  },

  /**
   * resets the element state
   *
   * @return self
   */
  reset: function() {
    this.element.removeClass(this.options.allowClass).removeClass(this.options.denyClass);
    return this;
  },

// protected

  // checks if the element is hovered by the event
  hoveredBy: function(event, draggable) {
    var dims     = this.element.dimensions(),
        t_top    = dims.top,
        t_left   = dims.left,
        t_right  = dims.left + dims.width,
        t_bottom = dims.top  + dims.height,
        event_x  = event.pageX,
        event_y  = event.pageY;

    // checking the overlapping
    if (this.options.overlap) {
      var drag_dims = draggable.element.dimensions(),
          level     = this.options.overlapSize,
          top       = drag_dims.top,
          left      = drag_dims.left,
          right     = drag_dims.left + drag_dims.width,
          bottom    = drag_dims.top  + drag_dims.height;


      switch (this.options.overlap) {
        // horizontal overlapping only check
        case 'x':
        case 'horizontal':
          return (
            (top    > t_top    && top      < t_bottom) ||
            (bottom > t_top    && bottom   < t_bottom)
          ) && (
            (left   > t_left   && left    < (t_right - dims.width * level)) ||
            (right  < t_right  && right   > (t_left  + dims.width * level))
          );

        // vertical overlapping only check
        case 'y':
        case 'vertical':
          return (
            (left   > t_left   && left   < t_right) ||
            (right  > t_left   && right  < t_right)
          ) && (
            (top    > t_top    && top    < (t_bottom - dims.height * level)) ||
            (bottom < t_bottom && bottom > (t_top + dims.height * level))
          );

        // both overlaps check
        default:
          return (
            (left   > t_left   && left    < (t_right - dims.width * level)) ||
            (right  < t_right  && right   > (t_left  + dims.width * level))
          ) && (
            (top    > t_top    && top    < (t_bottom - dims.height * level)) ||
            (bottom < t_bottom && bottom > (t_top + dims.height * level))
          );
      }

    } else {
      // simple check agains the event position
      return event_x > t_left && event_x < t_right && event_y > t_top && event_y < t_bottom;
    }
  },

  // checks if the object accepts the draggable
  allows: function(draggable) {
    if (this.options.containment && !this._scanned) {
      this.options.containment = R(this.options.containment).map($);
      this._scanned = true;
    }

    // checking the invitations list
    var welcomed = this.options.containment ? this.options.containment.includes(draggable.element) : true;

    return welcomed && (this.options.accept == '*' ? true : draggable.element.match(this.options.accept));
  }

});

/**
 * The document events hooker
 *
 * Copyright (C) 2009-2012 Nikolay Nemshilov
 */
$(document).on({
  // parocesses the automatically discovered elements
  ready: function() {
    Draggable.rescan();
    Droppable.rescan();
  },

  mousemove: document_mousemove,
  touchmove: document_mousemove,

  mouseup:   document_mouseup,
  touchend:  document_mouseup
});


// watch the draggables moving arond
function document_mousemove(event) {
  if (Draggable.current !== null) {
    Draggable.current.dragProcess(event);
    Droppable.checkHover(event, Draggable.current);
  }
}

// releases the current draggable on mouse up
function document_mouseup(event) {
  if (Draggable.current !== null) {
    Draggable.current.dragStop(event);
  }
}

/**
 * Element level hooks for drag'n'drops
 *
 * Copyright (C) 2009-2010 Nikolay Nemshilov
 */
Element.include({
  
  makeDraggable: function(options) {
    new Draggable(this, options);
    return this;
  },
  
  undoDraggable: function() {
    if ('draggable' in this) {
      this.draggable.destroy();
    }
    
    return this;
  },
  
  makeDroppable: function(options) {
    new Droppable(this, options);
    return this;
  },
  
  undoDroppable: function() {
    if ('droppable' in this) {
      this.droppable.destroy();
    }
    
    return this;
  }
  
});

window.Draggable = RightJS.Draggable = Draggable;
window.Droppable = RightJS.Droppable = Droppable;
})(window, document, RightJS);