(function() {

  /**
   *
   * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
   *
   * Copyright (c) 2014 Martin Drapeau
   * https://github.com/martindrapeau/backbone-game-engine
   *
   */
   
  Backbone.Pennie = Backbone.AnimatedTile.extend({
    defaults: {
      name: "pennie",
      type: "artifact",
      width: 32,
      height: 32,
      spriteSheet: "tiles",
      state: "idle",
      collision: true
    },
    animations: {
      idle: {
        sequences: [52, 52, 53, 54, 53, 52],
        delay: 50
      }
    },
    initialize: function(attributes, options) {
      Backbone.AnimatedTile.prototype.initialize.apply(this, arguments);
      this.on("hit", this.hit, this);
      this.on("squish", this.hit, this);
    },
    hit: function(sprite, dir, dir2) {
      if (sprite.get("hero")) {
        sprite.trigger("hit", this);
        _.defer(this.world.remove, this);
      }
    }
  });

  Backbone.PennieUg = Backbone.Pennie.extend({
    defaults: _.extend(_.deepClone(Backbone.Pennie.prototype.defaults), {
      name: "pennie-ug"
    }),
    animations: _.deepClone(Backbone.Pennie.prototype.animations)
  });
  Backbone.PennieUg.prototype.animations.idle.sequences = [168, 168, 169, 170, 169, 168];

  Backbone.Lever = Backbone.Pennie.extend({
    defaults: _.extend(_.deepClone(Backbone.Pennie.prototype.defaults), {
      name: "lever"
    }),
    animations: _.deepClone(Backbone.Pennie.prototype.animations),
    hit: function(sprite, dir, dir2) {}
  });
  Backbone.Lever.prototype.animations.idle.sequences = [55, 55, 56, 57, 56, 55];

	Backbone.FlyingPennie = Backbone.Sprite.extend({
    defaults: {
      name: "flying-pennie",
      type: "decoration",
      width: 32,
      height: 32,
      spriteSheet: "tiles",
      state: "anim",
      collision: false
    },
    animations: {
      anim: {
        sequences: [
          {frame: 52, x: 0, y: -32, scaleX: 1.00, scaleY: 1},
          {frame: 52, x: 0, y: -64, scaleX: 0.50, scaleY: 1},
          {frame: 53, x: 0, y: -90, scaleX: 0.50, scaleY: 1},
          {frame: 53, x: 0, y: -128, scaleX: 1.00, scaleY: 1},
          {frame: 53, x: 0, y: -128, scaleX: 0.50, scaleY: 1},
          {frame: 52, x: 0, y: -112, scaleX: 0.50, scaleY: 1},
          {frame: 52, x: 0, y: -90, scaleX: 1.00, scaleY: 1},
          {frame: 52, x: 0, y: -80, scaleX: 0.50, scaleY: 1},
          {frame: 53, x: 0, y: -80, scaleX: 0.50, scaleY: 1}
        ],
        delay: 50
      }
    },
    initialize: function(attributes, options) {
      options || (options = {});
      this.world = options.world;
      this.lastSequenceChangeTime = 0;
    },
    update: function(dt) {
      Backbone.Sprite.prototype.update.call(this, arguments);
      var animation = this.getAnimation(),
          sequenceIndex = this.get("sequenceIndex");

      if (sequenceIndex >= animation.sequences.length-1) {
        _.defer(this.world.remove, this);
        return false;
      }

      return true;
    }
	});

}).call(this);