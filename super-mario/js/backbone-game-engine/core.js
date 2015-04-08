(function() {

  /**
   *
   * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
   *
   * Copyright (c) 2014 Martin Drapeau
   * https://github.com/martindrapeau/backbone-game-engine
   *
   */

  // Sprite class; a Backbone Model which implements the required update
  // and draw functions to animate a sprite, frame by frame.
  Backbone.Sprite = Backbone.Model.extend({
    defaults: {
      name: undefined,

      // Position and state
      x: 0,
      y: 0,
      width: 0,
      height: 0,
      paddingLeft: 0,
      paddingRight: 0,
      paddingTop: 0,
      paddingBottom: 0,
      state: "idle",

      // Sprite sheet id
      spriteSheet: undefined,

      // Animations
      sequenceIndex: 0,
      zIndex: 0,

      static: false,
      collision: false,

      visible: true,
      persist: true
    },
    animations: {
      idle: {
        sequences: [0], // Array of frame indices for the animation
        delay: 0 // Delay per sequence in milliseconds.
      }
    },
    // Sprite sheet instance. Set automatically.
    spriteSheet: undefined,
    // Attributes to persist.
    saveAttributes: ["name", "state", "sequenceIndex", "x", "y"],
    initialize: function(attributes, options) {
      this.lastSequenceChangeTime = 0;
      this.bbox = {x1: 0, y1: 0, y1: 0, y2: 0};
      this.on("attach", this.onAttach, this);
      this.on("detach", this.onDetach, this);
    },
    onAttach: function() {},
    onDetach: function() {},
    toSave: function() {
      return this.get("persist") ? _.pick(this.toJSON(), this.saveAttributes) : null;
    },
    getLeft: function(withPadding) {
      return this.attributes.x + (withPadding && this.attributes.paddingLeft ? this.attributes.paddingLeft : 0);
    },
    getRight: function(withPadding) {
      return this.attributes.x + this.attributes.width - (withPadding && this.attributes.paddingRight ? this.attributes.paddingRight : 0);
    },
    getTop: function(withPadding) {
      return this.attributes.y + (withPadding && this.attributes.paddingTop ? this.attributes.paddingTop : 0);
    },
    getBottom: function(withPadding) {
      return this.attributes.y + this.attributes.height - (withPadding && this.attributes.paddingBottom ? this.attributes.paddingBottom : 0);
    },
    getBbox: function(withPadding) {
      this.bbox.x1 = this.getLeft(withPadding);
      this.bbox.x2 = this.getRight(withPadding);
      this.bbox.y1 = this.getTop(withPadding);
      this.bbox.y2 = this.getBottom(withPadding);
      return this.bbox;
    },
    getCenterX: function(withPadding) {
      var x = this.getLeft(withPadding),
          width = this.getRight(withPadding) - x;
      return x + width/2;
    },
    getCenterY: function(withPadding) {
      var y = this.getTop(withPadding),
          height = this.getBottom(withPadding) - y;
      return x + height/2;
    },
    update: function(dt) {
      // Fetch animation and change sequence if need be
      var animation = this.getAnimation(),
          sequenceIndex = this.get("sequenceIndex"),
          delay = this.sequenceDelay ? this.sequenceDelay(animation) : animation.delay || 200,
          now = _.now();

      if (!animation || sequenceIndex >= animation.sequences.length) {
        this.set("sequenceIndex", 0);
        this.lastSequenceChangeTime = now;
      } else if (animation && delay && now > this.lastSequenceChangeTime + delay) {
        this.set("sequenceIndex", sequenceIndex < animation.sequences.length-1 ? sequenceIndex + 1 : 0);
        this.lastSequenceChangeTime = now;
      }

      if (typeof this.onUpdate == "function") return this.onUpdate(dt);
      return true;
    },
    draw: function(context, options) {
      options || (options = {});
      if (this.get("visible") === false) return this;
      
      var animation = this.getAnimation(),
          sequenceIndex = this.get("sequenceIndex") || 0;
      if (!animation || animation.sequences.length == 0) return;
      if (sequenceIndex >= animation.sequences.length) sequenceIndex = 0;

      var sequence = animation.sequences[sequenceIndex]
          frameIndex = _.isNumber(sequence) ? sequence : sequence.frame,
          frame = this.spriteSheet.frames[frameIndex],
          scaleX = animation.scaleX && animation.scaleX != 1 ? animation.scaleX : null,
          scaleY = animation.scaleY && animation.scaleY != 1 ? animation.scaleY : null,
          x = Math.round(this.get("x") + (options.offsetX || 0) + (sequence.x || 0)),
          y = Math.round(this.get("y") + (options.offsetY || 0) + (sequence.y || 0));
      if (sequence.scaleY && sequence.scaleX != 1) scaleX = sequence.scaleX;
      if (sequence.scaleY &&  sequence.scaleY != 1) scaleY = sequence.scaleY;

      // Handle transformations (only scaling for now)
      if (_.isNumber(scaleX) || _.isNumber(scaleY)) {
        context.save();
        var flipX = scaleX && scaleX != 1 ? x + frame.width / 2 : 0;
        var flipY = scaleY && scaleY != 1 ? y + frame.height / 2 : 0;
        context.translate(flipX, flipY);
        context.scale(scaleX || 1, scaleY || 1);
        context.translate(-flipX, -flipY);
      }

      context.drawImage(
        this.spriteSheet.img,
        frame.x, frame.y, frame.width, frame.height,
        x, y, frame.width, frame.height
      );

      if (_.isNumber(scaleX) || _.isNumber(scaleY)) context.restore();

      if (typeof this.onDraw == "function") this.onDraw(context, options);
      return this;
    },
    getAnimation: function(state) {
      return this.animations[state || this.attributes.state];
    },
    overlaps: function(x, y) {
      var sx1 = this.attributes.x + (this.attributes.paddingLeft || 0),
          sy1 = this.attributes.y + (this.attributes.paddingTop || 0),
          sx2 = this.attributes.x + this.attributes.width - (this.attributes.paddingRight || 0),
          sy2 = this.attributes.y + this.attributes.height - (this.attributes.paddingBottom || 0);
      if (y === undefined) {
        var o = x;
        return !(
          sx1 > o.x + (o.width || 0) ||
          sx2 < o.x ||
          sy1 > o.y + (o.height || 0) ||
          sy2 < o.y
        );
      }
      return (x >= sx1 && y >= sy1 && x <= sx2 && y <= sy2);
    }
  });

  // Sprite Collection. Able to instantiate a Sprite with the proper
  // class by looking at the name attribute. A Sprite name is dasherized
  // and its class name is camel-case (i.e. class MySprite and name my-sprite).
  Backbone.SpriteCollection = Backbone.Collection.extend({
    model: function(attributes, options) {
      var cls = _.classify(attributes.name);
      if (!_.isFunction(Backbone[cls])) throw "Invalid cls " + cls + " for " + attributes.name;
      return new Backbone[cls](attributes, options);
    }
  });

  // SpriteSheet class; a Backbone model which breaks an image into
  // frames used for animation.
  Backbone.SpriteSheet = Backbone.Model.extend({
    defaults: {
      x: 0,
      y: 0,
      img: undefined, // Element id to find image in DOM
      tileWidth: undefined,
      tileHeight: undefined,
      tileColumns: undefined,
      tileRows: undefined
    },
    initialize: function(attributes, options) {
      this.frames = [];
      this.buildFrames();
      this.on("change:img", this.spawnImg);
      this.spawnImg();
    },
    // Constructs the frames array. A frame is an object with x, y width and height.
    // Determines the image position in the spritesheet. Used as sx, sy, sw, and sh
    // when drawing on the canvas.
    buildFrames: function() {
      var sheet = this.toJSON();
      for (var row = 0; row < sheet.tileRows; row++) {
        for (var col = 0; col < sheet.tileColumns; col++)
          this.frames.push({
            x: sheet.x + col * sheet.tileWidth,
            y: sheet.y + row * sheet.tileHeight,
            width: sheet.tileWidth,
            height: sheet.tileHeight
          });
        }
      return this;
    },
    // Sets the img property by looking at attribute img.
    // If a string, we assume it is the id of a DOM element and fetch appropriately.
    spawnImg: function() {
      var img = this.get("img");

      if (typeof img == "string") {
        var id = img.replace("#", "");
        img = document.getElementById(id);
        if (!img)
          throw "Invalid img #" + id + " for " + this.get("name") + ". Cannot find element by id.";
      }

      if (typeof img != "object" || !img.src)
        throw "Invalid img attribute for " + this.get("name") + ". Not a valid Image object.";

      this.img = img;
      return this;
    }
  });

  // SpriteSheetCollection class; a Backbone collection of SpriteSheet models.
  // 
  // Once the sprite sheet collection is instantiated, call method
  // attachToSpriteClasses() to automatically attach sprite sheets to sprites.
  // Will set the spriteSheet property on classes, avoiding you to have to do
  // it yourself.
  Backbone.SpriteSheetCollection = Backbone.Collection.extend({
    model: Backbone.SpriteSheet,
    // Attaches sprite sheets to sprite classes. Must be called before
    // calling draw on any sprite.
    attachToSpriteClasses: function() {
      var spriteSheets = this;
      _.each(Backbone, function(cls) {
        if (_.isFunction(cls) && cls.prototype instanceof Backbone.Sprite &&
            cls.prototype.defaults && cls.prototype.defaults.spriteSheet &&
            (!cls.prototype.spriteSheet || cls.prototype.spriteSheet.attributes.name != cls.prototype.defaults.spriteSheet)) {
          var spriteSheet = spriteSheets.get(cls.prototype.defaults.spriteSheet);
          if (spriteSheet) cls.prototype.spriteSheet = spriteSheet;
        }
      });
      return this;
    }
  });


  // Engine class; a Backbone Model which wraps a Backbone Collection of
  // models that have the required update and draw methods. Will draw them
  // on an HTML5 canvas.
  Backbone.Engine = Backbone.Model.extend({
    defaults: {
      version: 0.3,
      clearOnDraw: false,
      tapDetectionDelay: 50, // in ms
      tapMoveTolerance: 5 // Move tolerance for a tap detection in pixels
    },
    initialize: function(attributes, options) {
      options || (options = {});
      this.input = options.input;
      this.canvas = options.canvas;
      this.debugPanel = options.debugPanel;

      _.bindAll(this,
        "add", "remove", "start", "stop", "toggle", "onAnimationFrame",
        "onTouchStart", "onTouchEnd", "onTouchMove", "onKey"
      );

      if (!this.canvas || typeof this.canvas.getContext !== "function")
        throw new Error("Missing or invalid canvas.");

      // Handle the pause button - stops the engine.
      var input = this.input,
          toggleFn = _.debounce(this.toggle, 50);
      if (input) this.listenTo(input, "change:pause", function(input) {
        if (input.get("pause")) toggleFn();
      });

      this.context = this.canvas.getContext("2d");
      this.context.imageSmoothingEnabled = false;
      this.lastTime = _.now();
      this.start();

      this.sprites = new Backbone.Collection();

      // Trigger attach and detach events on sprites
      // Also set the property engine
      var engine = this;
      this.sprites.on("reset", function() {
        this.each(function(sprite) {
          sprite.engine = engine;
          sprite._draw = false;
          sprite.trigger("attach", engine);
        });
      });
      this.sprites.on("add", function(sprite) {
        sprite.engine = engine;
        sprite._draw = false;
        sprite.trigger("attach", engine);
      });
      this.sprites.on("remove", function(sprite) {
        sprite.trigger("detach", engine);
        delete sprite._draw;
        sprite.engine = undefined;
      });
      this.sprites.on("reset", function(sprites, options) {
        if (options && options.previousModels)
          _.each(options.previousModels, function(sprite) {
            sprite.trigger("detach", engine);
            delete sprite._draw;
            sprite.engine = undefined;
          });
      });

      // Touch (triggers tap event)
      this._gesture = undefined;
      this._touchStartTime = undefined;
      this._currX = this._startX = 0;
      this._currY = this._startY = 0;
      $(document).on("touchstart.engine", this.onTouchStart);
      $(document).on("mousedown.engine", this.onTouchStart);
      $(document).on("touchend.engine", this.onTouchEnd);
      $(document).on("mouseup.engine", this.onTouchEnd);
      $(document).on("touchcancel.engine", this.onTouchEnd);
      $(document).on("touchmove.engine", this.onTouchMove);
      $(document).on("mousemove.engine", this.onTouchMove);

      // Keyboard (triggers key event)
      $(document).on("keyup.engine", this.onKey);

      // For the trigger of reset event
      setTimeout(function() {
        engine.trigger("reset");
      }, 1);
    },
    add: function() {
      return this.sprites.add.apply(this.sprites, arguments);
    },
    remove: function() {
      return this.sprites.remove.apply(this.sprites, arguments);
    },
    reset: function() {
      return this.sprites.reset.apply(this.sprites, arguments);
    },
    isRunning: function() {
      return !!this.timerId;
    },
    start: function() {
      var now = _.now();
      this.lastTime = now;
      this.fpsStartTime = now;
      this.fpsCount = 0;
      this.cycleTime = 0;
      this.cycleTimes = [];
      this.timerId = requestAnimationFrame(this.onAnimationFrame);
      this.trigger("start");
      return this;
    },
    stop: function() {
      cancelAnimationFrame(this.timerId);
      this.timerId = null;
      this.trigger("stop");
      return this;
    },
    toggle: function() {
      if (this.timerId)
        this.stop();
      else
        this.start();
      return this;
    },
    onAnimationFrame: function() {
      var context = this.context,
          now = _.now(),
          dt = now - this.lastTime,
          sprite;

      // Update
      for (var i = 0; i < this.sprites.models.length; i++) {
        sprite = this.sprites.models[i];
        sprite._draw = sprite.update.call(sprite, dt);
      }

      if (this.attributes.clearOnDraw)
        context.clearRect(0, 0, context.canvas.width, context.canvas.height);

      // Draw
      for (var i = 0; i < this.sprites.models.length; i++) {
        sprite = this.sprites.models[i];
        if (sprite._draw) sprite.draw.call(sprite, context);
      }

      // Call ourself again next time
      this.timerId = requestAnimationFrame(this.onAnimationFrame);

      // Save our timestamp
      this.lastTime = now;

      this.fpsCount += 1;
      if (now - this.fpsStartTime > 1000) {
        this.fps = this.fpsCount;
        this.fpsCount = 0;
        this.fpsStartTime = now;
      }

      this.cycleTimes.push( _.now() - now);
      if (this.cycleTimes.length == 5) {
        this.cycleTime = Math.round(_.average(this.cycleTimes));
        this.cycleTimes = [];
      }

      if (this.debugPanel) this.debugPanel.set({fps: this.fps, ct: this.cycleTime});
      return this;
    },

    // Handle touch and trigger tap event
    getPointerEvent: function(e) {
      return e.targetTouches ? e.targetTouches[0] : e;
    },
    onTouchStart: function(e) {
      e.preventDefault();
      var pointer = this.getPointerEvent(e);
      this._startX = this._currX = pointer.pageX;
      this._startY = this._currY = pointer.pageY;
      this._gesture = "tap";
      this._touchStartTime = _.now();
    },
    onTouchEnd: function(e) {
      if (!this._touchStartTime) return;
      e.preventDefault();

      var now = _.now();
      e.canvas = this.canvas;
      e.canvasX = this._currX - this.canvas.offsetLeft + this.canvas.scrollLeft;
      e.canvasY = this._currY - this.canvas.offsetTop + this.canvas.scrollTop;
      e.canvasHandled = false;

      if (this._gesture == "tap" && now - this._touchStartTime > this.get("tapDetectionDelay")) {
        this.trigger("tap", e);
      } else if (this._gesture == "drag") {
        this.trigger("dragend", e);
      }
      this._gesture = undefined;
      this._touchStartTime = undefined;
    },
    onTouchMove: function(e) {
      if (!this._touchStartTime) return;
      e.preventDefault();

      var pointer = this.getPointerEvent(e),
          tolerance = this.get("tapMoveTolerance");
      this._currX = pointer.pageX;
      this._currY = pointer.pageY;

      e.canvas = this.canvas;
      e.canvasX = this._currX - this.canvas.offsetLeft + this.canvas.scrollLeft;
      e.canvasY = this._currY - this.canvas.offsetTop + this.canvas.scrollTop;
      e.canvasDeltaX = this._currX - this._startX;
      e.canvasDeltaY = this._currY - this._startY;

      if (this._gesture == "drag") {
        this.trigger("dragmove", e);
      } else if (Math.abs(e.canvasDeltaX) > tolerance || Math.abs(e.canvasDeltaY) > tolerance) {
        this._gesture = "drag";
        this.trigger("dragstart", e);
      }
    },

    // Handle keyboard and trigger key event
    onKey: function(e) {
      e.canvas = this.canvas;
      this.trigger("key", e);
    }
  });

  // Clock class; Ticks every given delay in milliseconds.
  // Bind an event on change:ticks to do something.
  // Can be added to the engine collection. Does not draw anything
  // on screen.
  Backbone.Clock = Backbone.Model.extend({
    defaults: {
      ticks: 0,
      delay: 200
    },
    update: function(dt) {
      var now = _.now();
      if (!this.lastTickTime || now > this.lastTickTime + this.get("delay")) {
        this.set("ticks", this.get("ticks") + 1);
        this.lastTickTime = now;
      }
      return false;
    },
    draw: function() {}
  });

  // Element class; mimics an elementary fixed position element on canvas.
  // Supports background color, rounded corners, background image and text.
  // Also has rudimentary animations.
  var fontRe = /(\d+)px/;
  Backbone.Element = Backbone.Model.extend({
    defaults: {
      x: 0,
      y: 0,
      width: 0,
      height: 0,
      // Image
      img: undefined,
      imgX: 0,
      imgY: 0,
      imgWidth: 0,
      imgHeight:0,
      imgMargin:0,
      // Background
      borderRadius: 0,
      backgroundColor: "rgba(160, 160, 160, 1)",
      // Text
      text:  undefined,
      textPadding: 0,
      textLineHeight: 30,
      textContextAttributes: undefined,
      // Animations
      easingTime: 1000,
      easing: "linear",
      opacity: 1,
      scale: 1
    },
    initialize: function() {
      this.on("attach", this.onAttach);
      this.on("detach", this.onDetach);
      this.on("change:text change:textContextAttributes", this.clearTextMetrics);
    },
    clearTextMetrics: function() {
      if (this.textMetrics) this.textMetrics = undefined;
    },
    onAttach: function() {
      this.onDetach();
      if (!this.img && this.attributes.img) this.spawnImg();
    },
    onDetach: function() {
    },
    clearAnimation: function() {
      this._animation = undefined;
      this._animationUpdateFn = undefined;
      this._startTime = undefined;
      this._startX = undefined;
      this._startY = undefined;
      this._targetX = undefined;
      this._targetY = undefined;
      this._callback = undefined;
    },
    moveTo: function(x, y, callback) {
      this._animation = "move";
      this._startTime = _.now();
      this._startX = this.get("x");
      this._startY = this.get("y");
      this._targetX = x;
      this._targetY = y;
      this._callback = callback;
      this._animationUpdateFn = function(dt) {
        var now = _.now(),
            easingTime = this.get("easingTime"),
            easing = this.get("easing");
        if (now < this._startTime + easingTime) {
          var factor = Backbone.EasingFunctions[easing]((now - this._startTime) / easingTime);
          this.set({
            x: this._startX + factor * (this._targetX - this._startX),
            y: this._startY + factor * (this._targetY - this._startY)
          });
        } else {
          if (typeof this._callback == "function") _.defer(this._callback.bind(this));
          this.set({x: this._targetX, y: this._targetY}, {silent: true});
          this.clearAnimation();
        }
      };
      return this;
    },
    fadeIn: function(callback) {
      this._animation = "fadeIn";
      this._startTime = _.now();
      this._callback = callback;
      this.set("opacity", 0);
      this._animationUpdateFn = function(dt) {
        var now = _.now(),
            easingTime = this.get("easingTime"),
            easing = this.get("easing");
        if (now < this._startTime + easingTime) {
          this.set("opacity", Backbone.EasingFunctions[easing]((now - this._startTime) / easingTime));
        } else {
          if (typeof this._callback == "function") _.defer(this._callback.bind(this));
          this.set({opacity: 1}, {silent: true});
          this.clearAnimation();
        }
      };
      return this;
    },
    fadeOut: function(callback) {
      this._animation = "fadeOut";
      this._startTime = _.now();
      this._callback = callback;
      this.set("opacity", 1);
      this._animationUpdateFn = function(dt) {
        var now = _.now(),
            easingTime = this.get("easingTime"),
            easing = this.get("easing");
        if (now < this._startTime + easingTime) {
          this.set("opacity", 1 - Backbone.EasingFunctions[easing]((now - this._startTime) / easingTime));
        } else {
          if (typeof this._callback == "function") _.defer(this._callback.bind(this));
          this.set({opacity: 0}, {silent: true});
          this.clearAnimation();
        }
      };
      return this;
    },
    update: function(dt) {
      if (this._animation) this._animationUpdateFn(dt);
      if (typeof this.onUpdate == "function") this.onUpdate(dt);
      return true;
    },
    draw: function(context, options) {
      var b = this.toJSON();
      if (b.opacity == 0) return this;

      options || (options = {});
      b.x += options.offsetX || 0;
      b.y += options.offsetY || 0;

      context.save();

      context.globalAlpha = b.opacity;
      var offsetX = (1-b.scale) * b.width/2,
          offsetY = (1-b.scale) * b.height/2;

      if (b.backgroundColor && b.backgroundColor != "transparent") {
        if (b.borderRadius)
          drawRoundRect(context, b.x+offsetX, b.y+offsetY, b.width*b.scale, b.height*b.scale, b.borderRadius, b.backgroundColor, false);
        else
          drawRect(context, b.x+offsetX, b.y+offsetY, b.width*b.scale, b.height*b.scale, b.backgroundColor, false);
      }

      if (this.img)
        context.drawImage(this.img,
          b.imgX, b.imgY, b.imgWidth, b.imgHeight,
          b.x + b.imgMargin + offsetX, b.y + b.imgMargin + offsetY, b.imgWidth*b.scale, b.imgHeight*b.scale
        );

      if (b.text !== undefined && b.text !== null && ""+b.text !== "")
        this.drawText(b, context, options);

      if (typeof this.onDraw == "function")
        this.onDraw(context, options);

      context.restore();

      return this;
    },
    drawText: function(b, context, options) {
      var x = b.x,
          y = b.y,
          offsetX = (1-b.scale) * b.width/2,
          offsetY = (1-b.scale) * b.height/2,
          padding = b.textPadding * b.scale;

      if (typeof b.textContextAttributes == "object")
        for (var attr in b.textContextAttributes)
          if (b.textContextAttributes.hasOwnProperty(attr))
            context[attr] = b.textContextAttributes[attr];
      switch (context.textAlign) {
        case "left":
        case "start":
          x += padding + offsetX;
          break;
        case "right":
        case "end":
          x += b.width - padding - offsetX;
          break;
        case "center":
          x += b.width/2;
          break;
      }
      var lines = (""+b.text).split("\n");
      switch (context.textBaseline) {
        case "top":
          y += padding + offsetY;
        case "bottom":
          y += b.height - padding - (lines.length-1)*b.textLineHeight - offsetY;
          break;
        case "middle":
          y += b.height/2 - (lines.length-1)*b.textLineHeight/2;
          break;
      }
      if (b.scale != 1 && b.textContextAttributes.font) {
        var matches = b.textContextAttributes.font.match(fontRe);
        if (matches.length == 2)
          context.font = b.textContextAttributes.font.replace(fontRe, matches[1]*b.scale+"px");
      }
      if (lines.length == 1) {
        context.fillText(b.text, x, y);
        if (!this.textMetrics) this.textMetrics = context.measureText(b.text);
      } else {
        var textMetrics;
        for (var i = 0; i < lines.length; i++) {
          context.fillText(lines[i], x, y);
          y += b.textLineHeight;
          if (!this.textMetrics) {
            var temp = context.measureText(lines[i]);
            if (!textMetrics || temp.width > textMetrics.width) textMetrics = temp;
          }
        }
        if (textMetrics) this.textMetrics = textMetrics;
      }
      return this;
    },
    overlaps: Backbone.Sprite.prototype.overlaps,
    spawnImg: Backbone.SpriteSheet.prototype.spawnImg
  });

  // Button class; an element when pressed animates a slight grow/shrink
  // and triggers a tap event
  Backbone.Button = Backbone.Element.extend({
    onAttach: function() {
      Backbone.Element.prototype.onAttach.apply(this, arguments);
      this.listenTo(this.engine, "tap", this.onTap);
    },
    onDetach: function() {
      Backbone.Element.prototype.onDetach.apply(this, arguments);
      this.stopListening(this.engine);
    },
    pressed: function(callback) {
      this._animation = "pressed";
      this._startTime = _.now();
      this._callback = callback
      this.set("scale", 1);
      this._animationUpdateFn = function(dt) {
        var now = _.now(),
            easing = "linear",
            easingTime = 200;
        if (now < this._startTime + easingTime) {
          this.set("scale", 1.05 - Math.abs(Backbone.EasingFunctions[easing]((now - this._startTime) / easingTime)-0.5)/10 );
        } else {
          if (typeof this._callback == "function") _.defer(this._callback.bind(this));
          this.set({scale: 1}, {silent: true});
          this.clearAnimation();
        }
      };
      return this;
    },
    onTap: function(e) {
      if (this.get("opacity") == 0 && !e.canvasHandled) return;
      if (e.canvasX >= this.attributes.x && e.canvasX <= this.attributes.x + this.attributes.width &&
          e.canvasY >= this.attributes.y && e.canvasY <= this.attributes.y + this.attributes.height) {
        this.pressed(_.partial(this.trigger, "tap", e));
        e.canvasHandled = true;
      }
    }
  });

  // Displays a message top center
  Backbone.Message = Backbone.Model.extend({
    defaults: {
      x: 0,
      y: 0,
      text: null,
      delay: 5000,
      active: false,
      color: "#fff"
    },
    initialize: function() {
      _.bindAll(this, "show", "hide");
      this.on("attach", this.hide);
      this.on("detach", this.hide);
    },
    update: function(dt) {
      return !!this.get("active");
    },
    draw: function(context) {
      var m = this.toJSON();

      context.fillStyle = this.get("color");
      context.font = "16px arial";
      context.textAlign = "center";
      context.textBaseline = "middle";
      context.fillText(m.text || "", m.x, m.y);

      return this;
    },
    show: function(text) {
      var message = this;

      if (this.timerId) clearTimeout(this.timerId);

      this.set({text: text || this.get("text") || "no message", active: true}, {silent:true});
      this.timerId = setTimeout(this.hide, this.get("delay"));

      return this;
    },
    hide: function() {
      if (this.timerId) clearTimeout(this.timerId);
      this.set({active: false, text: null}, {silent: true});
      return this;
    }
  })

  // DebugPanel class; draws debug information on screen.
  Backbone.DebugPanel = Backbone.Model.extend({
    defaults: {
      fps: 0
    },
    initialize: function(attributes, options) {
      options || (options = {});
      this.color = options.color || "#ff0";
    },
    update: function(dt) {
      return true;
    },
    draw: function(context) {
      var text = JSON.stringify(this.toJSON());
      context.fillStyle = this.color;
      context.font = "12px arial";
      context.textAlign = "left";
      context.textBaseline = "middle";
      context.fillText(text, 100, 10);
      return this;
    }
  });


  // The global timer function. Aims to do 60 frames per second.

  // requestAnimationFrame polyfill
  // Source: http://www.paulirish.com/2011/requestanimationframe-for-smart-animating/
  var lastTime = 0;
  var vendors = ['webkit', 'moz'];
  for(var x = 0; x < vendors.length && !window.requestAnimationFrame; ++x) {
    window.requestAnimationFrame = window[vendors[x]+'RequestAnimationFrame'];
    window.cancelAnimationFrame =
    window[vendors[x]+'CancelAnimationFrame'] || window[vendors[x]+'CancelRequestAnimationFrame'];
  }

  if (!window.requestAnimationFrame)
    window.requestAnimationFrame = function(callback, element) {
      var currTime = _.now();
      var timeToCall = Math.max(0, 16 - (currTime - lastTime));
      var id = window.setTimeout(function() { callback(currTime + timeToCall); },
        timeToCall);
      lastTime = currTime + timeToCall;
      return id;
    };

  if (!window.cancelAnimationFrame)
    window.cancelAnimationFrame = function(id) {
      clearTimeout(id);
    };


  // Underscore extensions - global helper functions
  _.mixin({
    minNotNull: function(values) {
      var min = null;
      _.each(values, function(value) {
        if (_.isNumber(value) && (min == null || value < min))
          min = value;
      });
      return min;
    },
    maxNotNull: function(values) {
      var max = null;
      _.each(values, function(value) {
        if (_.isNumber(value) && (max == null || value > max))
          max = value;
      });
      return max;
    },
    deepClone: function(object) {
      return JSON.parse(JSON.stringify(object));
    },
    sum: function(a) {
      return _.reduce(a, function(sum, num){ return sum + num; }, 0);
    },
    average: function(a) {
      return _.sum(a) / a.length;
    },
    // Taken from Underscore String
    // Source: https://github.com/epeli/underscore.string
    titleize: function(str){
      if (str == null) return '';
      str  = String(str).toLowerCase();
      return str.replace(/(?:^|\s|-)\S/g, function(c){ return c.toUpperCase(); });
    },
    classify: function(str){
      return _.titleize(String(str).replace(/[\W_]/g, ' ')).replace(/\s/g, '');
    },
    ms2time: function(ms) {
      var sec_num = Math.round(parseInt(ms, 10) / 1000); // don't forget the second param
      var hours = Math.floor(sec_num / 3600);
      var minutes = Math.floor((sec_num - (hours * 3600)) / 60);
      var seconds = sec_num - (hours * 3600) - (minutes * 60);

      if (minutes < 10) {minutes = "0"+minutes;}
      if (seconds < 10) {seconds = "0"+seconds;}
      var time = hours + ':' + minutes + ':' + seconds;
      return time;
    },
    // source: http://stackoverflow.com/questions/950087/include-a-javascript-file-in-another-javascript-file
    loadScript: function(url, callback) {
      var head = document.getElementsByTagName("head")[0];
      var script = document.createElement("script");
      script.type = "text/javascript";
      script.src = url;
      if (callback) {
        script.onreadystatechange = callback;
        script.onload = callback;
      }
      head.appendChild(script);
    },
    loadStylesheet: function(url, callback) {
      var head = document.getElementsByTagName("head")[0];
      var link = document.createElement("link");
      link.rel = "stylesheet";
      link.type = "text/css";
      link.href = url;
      if (callback) {
        link.onreadystatechange = callback;
        link.onload = callback;
      }
      head.appendChild(link);
    }
  });

  /*
   * https://gist.github.com/gre/1650294
   * Easing Functions - inspired from http://gizma.com/easing/
   * only considering the t value for the range [0, 1] => [0, 1]
   */
  Backbone.EasingFunctions = {
    // no easing, no acceleration
    linear: function (t) { return t },
    // accelerating from zero velocity
    easeInQuad: function (t) { return t*t },
    // decelerating to zero velocity
    easeOutQuad: function (t) { return t*(2-t) },
    // acceleration until halfway, then deceleration
    easeInOutQuad: function (t) { return t<.5 ? 2*t*t : -1+(4-2*t)*t },
    // accelerating from zero velocity 
    easeInCubic: function (t) { return t*t*t },
    // decelerating to zero velocity 
    easeOutCubic: function (t) { return (--t)*t*t+1 },
    // acceleration until halfway, then deceleration 
    easeInOutCubic: function (t) { return t<.5 ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1 },
    // accelerating from zero velocity 
    easeInQuart: function (t) { return t*t*t*t },
    // decelerating to zero velocity 
    easeOutQuart: function (t) { return 1-(--t)*t*t*t },
    // acceleration until halfway, then deceleration
    easeInOutQuart: function (t) { return t<.5 ? 8*t*t*t*t : 1-8*(--t)*t*t*t },
    // accelerating from zero velocity
    easeInQuint: function (t) { return t*t*t*t*t },
    // decelerating to zero velocity
    easeOutQuint: function (t) { return 1+(--t)*t*t*t*t },
    // acceleration until halfway, then deceleration 
    easeInOutQuint: function (t) { return t<.5 ? 16*t*t*t*t*t : 1+16*(--t)*t*t*t*t }
  };

}).call(this);
