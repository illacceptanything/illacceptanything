(function() {

  /**
   *
   * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
   *
   * Copyright (c) 2014 Martin Drapeau
   * https://github.com/martindrapeau/backbone-game-engine
   *
   */
   
  var sequenceDelay = 300,
      animations;

  // Mushroom is the base enemie class.
  Backbone.Mushroom = Backbone.Character.extend({
    defaults: _.extend(_.deepClone(Backbone.Character.prototype.defaults), {
      name: "mushroom",
      type: "character",
      width: 32,
      height: 64,
      paddingTop: 32,
      spriteSheet: "enemies",
      state: "idle-left",
      velocity: 0,
      yVelocity: 0,
      collision: true,
      aiDelay: 0
    }),
    animations: _.extend(_.deepClone(Backbone.Character.prototype.animations), {
      "squished-left": {
        sequences: [2],
        velocity: 0,
        scaleX: 1,
        scaleY: 1
      },
      "squished-right": {
        sequences: [2],
        velocity: 0,
        scaleX: -1,
        scaleY: 1
      }
    }),
    ai: function(dt) {
      var cur = this.getStateInfo();
      if (cur.mov == "squished" && !this.get("collision")) this.cancelUpdate = true;
      return this;
    },
    isAttacking: function(sprite, dir, dir2) {
      if (this.cancelUpdate) return false;
      var cur = this.getStateInfo();
      return (cur.mov == "walk" || cur.mov == "idle");
    },
    squish: function(sprite) {
      var self = this,
          cur = this.getStateInfo();
      this.set({
        state: this.buildState("squished", cur.dir),
        collision: false
      });
      this.world.setTimeout(function() {
        if (self && self.world) self.world.remove(self);
      }, 2000);
      this.cancelUpdate = true;
      return this;
    },
    hit: function(sprite, dir, dir2) {
      if (this._handlingSpriteHit) return this;
      this._handlingSpriteHit = sprite;

      var cur = this.getStateInfo(),
          opo = dir == "left" ? "right" : (dir == "right" ? "left" : (dir == "top" ? "bottom" : "top"));

      if (sprite.get("hero")) {
        if (dir == "top")
          this.squish.apply(this, arguments);
      } else if (sprite.get("state").indexOf("slide") != -1 ||
                sprite.get("type") == "tile" && dir == "bottom" && sprite.get("state") == "bounce") {
        this.knockout.apply(this, arguments);
      }
      sprite.trigger("hit", this, opo);

      this._handlingSpriteHit = undefined;
      return this;
    }
  });

  Backbone.Turtle = Backbone.Mushroom.extend({
    defaults: _.extend(_.deepClone(Backbone.Mushroom.prototype.defaults), {
      name: "turtle"
    }),
    animations: _.deepClone(Backbone.Mushroom.prototype.animations),
    isAttacking: function() {
      var cur = this.getStateInfo();
      return (cur.mov == "walk" || cur.mov == "idle");
    },
    slide: function(sprite, dir, dir2) {
      if (this.wakeTimerId) {
        this.world.clearTimeout(this.wakeTimerId);
        this.wakeTimerId = null;
      }

      var dir = sprite.getCenterX(true) > this.getCenterX(true) ? "left" : "right";
      this.set("state", this.buildState("walk", "slide", dir));
      this.cancelUpdate = true;
      return this;
    },
    squish: function(sprite, dir, dir2) {
      var cur = this.getStateInfo();

      if (cur.mov == "squished" || cur.mov == "wake")
        return this.slide.apply(this, arguments);

      if (this.wakeTimerId) {
        this.world.clearTimeout(this.wakeTimerId);
        this.wakeTimerId = null;
      }

      this.set("state", this.buildState("squished", cur.dir));
      this.wakeTimerId = this.world.setTimeout(this.wake.bind(this), 5000);

      this.cancelUpdate = true;
      return this;
    },
    hit: function(sprite, dir, dir2) {
      if (this._handlingSpriteHit) return this;
      this._handlingSpriteHit = sprite;

      var cur = this.getStateInfo(),
          opo = dir == "left" ? "right" : (dir == "right" ? "left" : (dir == "top" ? "bottom" : "top"));
      if (cur.mov2 == "slide") this.cancelUpdate = true;

      if (dir == "top") {
        this.squish.apply(this, arguments);
      } else if (sprite.get("hero") && (cur.mov == "squished" || cur.mov == "wake")) {
        this.slide.apply(this, arguments);
        opo = "bottom";
      } else if (sprite.get("state").indexOf("slide") != -1 ||
                sprite.get("type") == "tile" && dir == "bottom" && sprite.get("state") == "bounce") {
        this.knockout.apply(this, arguments);
      }

      sprite.trigger("hit", this, opo);

      this._handlingSpriteHit = undefined;
      return this;
    },
    wake: function() {
      var cur = this.getStateInfo();
      if (this.wakeTimerId) {
        this.world.clearTimeout(this.wakeTimerId);
        this.wakeTimerId = null;
      }

      if (cur.mov == "squished") {
        this.set("state", this.buildState("wake", cur.dir));
        this.wakeTimerId = this.world.setTimeout(this.wake.bind(this), 5000);
      } else if (cur.mov == "wake") {
        this.set("state", this.buildState("walk", cur.dir));
      }
      return this;
    }
  });
  animations = Backbone.Turtle.prototype.animations;
  animations["idle-left"].sequences = animations["idle-right"].sequences =
    animations["fall-left"].sequences = animations["fall-right"].sequences =
    animations["ko-left"].sequences = animations["ko-right"].sequences = [6];
  animations["walk-left"].sequences = animations["walk-right"].sequences = [6, 7];
  animations["squished-left"].sequences = animations["squished-right"].sequences = [10];
  _.extend(animations, {
    "wake-left": {
      sequences: [10, 11],
      velocity: 0,
      scaleX: 1,
      scaleY: 1,
      delay: sequenceDelay
    },
    "wake-right": {
      sequences: [10, 11],
      velocity: 0,
      scaleX: -1,
      scaleY: 1,
      delay: sequenceDelay
    },
    "walk-slide-left": {
      sequences: [10],
      velocity: -300,
      scaleX: 1,
      scaleY: 1
    },
    "walk-slide-right": {
      sequences: [10],
      velocity: 300,
      scaleX: -1,
      scaleY: 1
    },
    "fall-slide-left": {
      sequences: [10],
      velocity: -300,
      yVelocity: animations["fall-left"].yVelocity,
      yAcceleration: animations["fall-left"].yAcceleration,
      scaleX: 1,
      scaleY: 1
    },
    "fall-slide-right": {
      sequences: [10],
      velocity: 300,
      yVelocity: animations["fall-right"].yVelocity,
      yAcceleration: animations["fall-right"].yAcceleration,
      scaleX: -1,
      scaleY: 1
    }
  });

  Backbone.FlyingTurtle = Backbone.Turtle.extend({
    defaults: _.extend(_.deepClone(Backbone.Turtle.prototype.defaults), {
      name: "flying-turtle"
    }),
    animations: _.deepClone(Backbone.Turtle.prototype.animations),
    fallbackSprite: Backbone.Turtle,
    onUpdate: function(dt) {
      var cur = this.getStateInfo(),
          animation = this.getAnimation(),
          attrs = {};
      if (cur.mov2 == null && cur.mov == "walk" && this.world.get("state") == "play") {
          attrs.state = this.buildState("fall", cur.dir);
          attrs.yVelocity = -this.animations["fall-right"].yVelocity;
      }
      if (!_.isEmpty(attrs)) this.set(attrs);
      return true;
    },
    squish: function(sprite) {
      var cur = this.getStateInfo();
      var newSprite = new this.fallbackSprite({
        x: this.get("x"),
        y: this.get("y"),
        state: "walk-" + cur.dir
      });
      newSprite.set("id", this.world.buildIdFromName(newSprite.get("name")));
      this.world.add(newSprite);
      this.world.remove(this);
      this.cancelUpdate = true;
    }
  });
  animations = Backbone.FlyingTurtle.prototype.animations;
  animations["idle-left"].sequences = animations["idle-right"].sequences =
    animations["fall-left"].sequences = animations["fall-right"].sequences =
    animations["ko-left"].sequences = animations["ko-right"].sequences = [8];
  animations["walk-left"].sequences = animations["walk-right"].sequences = [8, 9];

  Backbone.RedTurtle = Backbone.Turtle.extend({
    defaults: _.extend(_.deepClone(Backbone.Turtle.prototype.defaults), {
      name: "red-turtle"
    }),
    animations: _.deepClone(Backbone.Turtle.prototype.animations)
  });
  animations = Backbone.RedTurtle.prototype.animations;
  animations["idle-left"].sequences = animations["idle-right"].sequences =
    animations["fall-left"].sequences = animations["fall-right"].sequences =
    animations["ko-left"].sequences = animations["ko-right"].sequences = [108];
  animations["walk-left"].sequences = animations["walk-right"].sequences = [108, 109];
  animations["squished-left"].sequences = animations["squished-right"].sequences =
    animations["walk-slide-left"].sequences = animations["walk-slide-right"].sequences =
    animations["fall-slide-left"].sequences = animations["fall-slide-right"].sequences = [112];
  animations["wake-left"].sequences = animations["wake-right"].sequences = [112, 113];

  Backbone.RedFlyingTurtle = Backbone.FlyingTurtle.extend({
    defaults: _.extend(_.deepClone(Backbone.FlyingTurtle.prototype.defaults), {
      name: "red-flying-turtle"
    }),
    animations: _.deepClone(Backbone.FlyingTurtle.prototype.animations),
    fallbackSprite: Backbone.RedTurtle
  });
  animations = Backbone.RedFlyingTurtle.prototype.animations;
  animations["idle-left"].sequences = animations["idle-right"].sequences =
    animations["fall-left"].sequences = animations["fall-right"].sequences =
    animations["ko-left"].sequences = animations["ko-right"].sequences = [110];
  animations["walk-left"].sequences = animations["walk-right"].sequences = [110, 111];
  animations["squished-left"].sequences = animations["squished-right"].sequences =
    animations["walk-slide-left"].sequences = animations["walk-slide-right"].sequences =
    animations["fall-slide-left"].sequences = animations["fall-slide-right"].sequences = [112];
  animations["wake-left"].sequences = animations["wake-right"].sequences = [112, 113];

  Backbone.Beetle = Backbone.Turtle.extend({
    defaults: _.extend(_.deepClone(Backbone.Turtle.prototype.defaults), {
      name: "beetle"
    }),
    animations: _.deepClone(Backbone.Turtle.prototype.animations)
  });
  animations = Backbone.Beetle.prototype.animations;
  animations["idle-left"].sequences = animations["idle-right"].sequences =
    animations["fall-left"].sequences = animations["fall-right"].sequences =
    animations["ko-left"].sequences = animations["ko-right"].sequences = [33];
  animations["walk-left"].sequences = animations["walk-right"].sequences = [33, 32];
  animations["squished-left"].sequences = animations["squished-right"].sequences =
    animations["walk-slide-left"].sequences = animations["walk-slide-right"].sequences = 
    animations["fall-slide-left"].sequences = animations["fall-slide-right"].sequences = 
    animations["wake-left"].sequences = animations["wake-right"].sequences = [34];

  Backbone.Spike = Backbone.Mushroom.extend({
    defaults: _.extend(_.deepClone(Backbone.Mushroom.prototype.defaults), {
      name: "spike"
    }),
    animations: _.deepClone(Backbone.Mushroom.prototype.animations),
    squish: function() {}
  });
  animations = Backbone.Spike.prototype.animations;
  animations["idle-left"].sequences = animations["idle-right"].sequences =
    animations["fall-left"].sequences = animations["fall-right"].sequences =
    animations["ko-left"].sequences = animations["ko-right"].sequences = [133];
  animations["walk-left"].sequences = animations["walk-right"].sequences = [133, 132];

}).call(this);