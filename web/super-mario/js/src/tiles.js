(function() {

  /**
   *
   * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
   *
   * Copyright (c) 2014 Martin Drapeau
   * https://github.com/martindrapeau/backbone-game-engine
   *
   */
  
  Backbone.Tile = Backbone.Sprite.extend({
    defaults: {
      type: "tile",
      width: 32,
      height: 32,
      spriteSheet: "tiles",
      state: "idle",
      static: true,
      persist: true
    },
    initialize: function(attributes, options) {
      options || (options = {});
      this.world = options.world;
      this.lastSequenceChangeTime = 0;
    }
  });


  function extendSprite(cls, name, attributes, animations) {
    var newCls = _.classify(name);
    Backbone[newCls] = Backbone[cls].extend({
      defaults: _.extend(
        _.deepClone(Backbone[cls].prototype.defaults),
        {name: name},
        attributes || {}
      ),
      animations: _.extend(
        _.deepClone(Backbone[cls].prototype.animations),
        animations || {}
      )
    });
    return Backbone[newCls];
  }

  extendSprite("Tile", "ground", {collision: true}, {idle: {sequences: [0]}});

  extendSprite("Tile", "ground2", {collision: true}, {idle: {sequences: [31]}});

  extendSprite("Tile", "block", {collision: true}, {idle: {sequences: [3]}});

  extendSprite("Tile", "block2", {collision: true}, {idle: {sequences: [29]}});

  extendSprite("Tile", "tube1", {collision: true}, {idle: {sequences: [290]}});
  
  extendSprite("Tile", "tube2", {collision: true}, {idle: {sequences: [291]}});

  extendSprite("Tile", "tube1-mirror", {collision: true}, {idle: {sequences: [261]}});
  
  extendSprite("Tile", "tube2-mirror", {collision: true}, {idle: {sequences: [262]}});

  extendSprite("Tile", "tube1-ug", {collision: true}, {idle: {sequences: [406]}});
  
  extendSprite("Tile", "tube2-ug", {collision: true}, {idle: {sequences: [407]}});

  extendSprite("Tile", "bush1", {collision: false}, {idle: {sequences: [240]}});

  extendSprite("Tile", "bush2", {collision: false}, {idle: {sequences: [241]}});

  extendSprite("Tile", "bush3", {collision: false}, {idle: {sequences: [242]}});

  extendSprite("Tile", "cloud1", {collision: false}, {idle: {sequences: [580]}});

  extendSprite("Tile", "cloud2", {collision: false}, {idle: {sequences: [581]}});

  extendSprite("Tile", "cloud3", {collision: false}, {idle: {sequences: [582]}});

  extendSprite("Tile", "cloud-happy1", {collision: false}, {idle: {sequences: [585]}});

  extendSprite("Tile", "cloud-happy2", {collision: false}, {idle: {sequences: [586]}});

  extendSprite("Tile", "cloud-happy3", {collision: false}, {idle: {sequences: [587]}});

  extendSprite("Tile", "flag-pole1", {collision: false}, {idle: {sequences: [306]}});

  extendSprite("Tile", "cloud-small", {collision: true}, {idle: {sequences: [613]}});

  extendSprite("Tile", "ground-ug", {collision: true}, {idle: {sequences: [116]}});

  extendSprite("Tile", "ground2-ug", {collision: true}, {idle: {sequences: [147]}});

  extendSprite("Tile", "block-ug", {collision: true}, {idle: {sequences: [119]}});

  extendSprite("Tile", "block2-ug", {collision: true}, {idle: {sequences: [145]}});

  extendSprite("Tile", "tube3", {collision: true}, {idle: {sequences: [319]}});
  
  extendSprite("Tile", "tube4", {collision: true}, {idle: {sequences: [320]}});

  extendSprite("Tile", "tube3-mirror", {collision: true}, {idle: {sequences: [232]}});
  
  extendSprite("Tile", "tube4-mirror", {collision: true}, {idle: {sequences: [233]}});

  extendSprite("Tile", "tube3-ug", {collision: true}, {idle: {sequences: [435]}});
  
  extendSprite("Tile", "tube4-ug", {collision: true}, {idle: {sequences: [436]}});

  extendSprite("Tile", "bush4", {collision: false}, {idle: {sequences: [269]}});

  extendSprite("Tile", "bush5", {collision: false}, {idle: {sequences: [270]}});

  extendSprite("Tile", "bush6", {collision: false}, {idle: {sequences: [271]}});

  extendSprite("Tile", "cloud4", {collision: false}, {idle: {sequences: [609]}});

  extendSprite("Tile", "cloud5", {collision: false}, {idle: {sequences: [610]}});

  extendSprite("Tile", "cloud6", {collision: false}, {idle: {sequences: [611]}});

  extendSprite("Tile", "cloud-happy4", {collision: false}, {idle: {sequences: [614]}});

  extendSprite("Tile", "cloud-happy5", {collision: false}, {idle: {sequences: [615]}});

  extendSprite("Tile", "cloud-happy6", {collision: false}, {idle: {sequences: [616]}});

  extendSprite("Tile", "flag-pole2", {collision: false}, {idle: {sequences: [335]}});

  extendSprite("Tile", "tube1-out", {collision: true}, {idle: {sequences: [292]}});
  
  extendSprite("Tile", "tube2-out", {collision: true}, {idle: {sequences: [293]}});
  
  extendSprite("Tile", "tube3-out", {collision: true}, {idle: {sequences: [294]}});

  extendSprite("Tile", "tube1-out-mirror", {collision: true}, {idle: {sequences: [234]}});
  
  extendSprite("Tile", "tube2-out-mirror", {collision: true}, {idle: {sequences: [235]}});
  
  extendSprite("Tile", "tube3-out-mirror", {collision: true}, {idle: {sequences: [236]}});

  extendSprite("Tile", "tube1-out-ug", {collision: true}, {idle: {sequences: [408]}});
  
  extendSprite("Tile", "tube2-out-ug", {collision: true}, {idle: {sequences: [409]}});
  
  extendSprite("Tile", "tube3-out-ug", {collision: true}, {idle: {sequences: [410]}});

  extendSprite("Tile", "brick7-castle", {collision: true}, {idle: {sequences: [42]}});

  extendSprite("Tile", "bush7", {collision: false}, {idle: {sequences: [272]}});

  extendSprite("Tile", "bush8", {collision: false}, {idle: {sequences: [273]}});

  extendSprite("Tile", "bush9", {collision: false}, {idle: {sequences: [274]}});

  extendSprite("Tile", "water1", {collision: false}, {idle: {sequences: [583]}});

  extendSprite("Tile", "water2", {collision: false}, {idle: {sequences: [612]}});

  extendSprite("Tile", "water-bridge", {collision: true}, {idle: {sequences: [32]}});

  extendSprite("Tile", "lava1", {collision: false}, {idle: {sequences: [699]}});

  extendSprite("Tile", "lava2", {collision: false}, {idle: {sequences: [728]}});

  extendSprite("Tile", "lava-bridge", {collision: true}, {idle: {sequences: [148]}});

  extendSprite("Tile", "tube4-out", {collision: true}, {idle: {sequences: [321]}});
  
  extendSprite("Tile", "tube5-out", {collision: true}, {idle: {sequences: [322]}});
  
  extendSprite("Tile", "tube6-out", {collision: true}, {idle: {sequences: [323]}});

  extendSprite("Tile", "tube4-out-mirror", {collision: true}, {idle: {sequences: [263]}});
  
  extendSprite("Tile", "tube5-out-mirror", {collision: true}, {idle: {sequences: [264]}});
  
  extendSprite("Tile", "tube6-out-mirror", {collision: true}, {idle: {sequences: [265]}});

  extendSprite("Tile", "tube4-out-ug", {collision: true}, {idle: {sequences: [437]}});
  
  extendSprite("Tile", "tube5-out-ug", {collision: true}, {idle: {sequences: [438]}});
  
  extendSprite("Tile", "tube6-out-ug", {collision: true}, {idle: {sequences: [439]}});

  extendSprite("Tile", "brick-castle", {collision: true}, {idle: {sequences: [11]}});

  extendSprite("Tile", "brick2-castle", {collision: true}, {idle: {sequences: [12]}});

  extendSprite("Tile", "brick3-castle", {collision: true}, {idle: {sequences: [13]}});

  extendSprite("Tile", "brick4-castle", {collision: true}, {idle: {sequences: [14]}});

  extendSprite("Tile", "brick5-castle", {collision: true}, {idle: {sequences: [40]}});

  extendSprite("Tile", "brick6-castle", {collision: true}, {idle: {sequences: [41]}});

  extendSprite("Tile", "railing", {collision: false}, {idle: {sequences: [363]}});

  extendSprite("Tile", "platform1", {collision: true}, {idle: {sequences: [237]}});
  
  extendSprite("Tile", "platform2", {collision: true}, {idle: {sequences: [238]}});
  
  extendSprite("Tile", "platform3", {collision: true}, {idle: {sequences: [239]}});
  
  extendSprite("Tile", "platform-pole", {collision: false}, {idle: {sequences: [208]}});


  // Subclass Sprite to use a global timer - so all animated
  // sprites animate in sync.
  Backbone.AnimatedTile = Backbone.Tile.extend({
    initialize: function(attributes, options) {
      Backbone.Tile.prototype.initialize.apply(this, arguments);
      this.on("attach", this.onAttach, this);
      this.on("detach", this.onDetach, this);
    },
    onAttach: function() {
      if (!this.engine) return;
      this.onDetach();

      this.clock = this.engine.sprites.findWhere({name: "animatedTileClock"});

      if (!this.clock)
        this.clock = this.engine.add(new Backbone.Clock({name: "animatedTileClock", delay: 200}));

      this.listenTo(this.clock, "change:ticks", this.updateAnimationIndex);
    },
    onDetach: function() {
      if (this.clock) this.stopListening(this.clock);
      this.clock = undefined;
    },
    update: function(dt) {
      return true;
    },
    updateAnimationIndex: function() {
      var animation = this.getAnimation(),
          sequenceIndex = this.get("sequenceIndex") || 0;
      if (!animation) return;
      this.set("sequenceIndex", sequenceIndex < animation.sequences.length-1 ? sequenceIndex + 1 : 0);
    }
  });

  Backbone.Brick = Backbone.AnimatedTile.extend({
    defaults: _.extend({}, Backbone.Tile.prototype.defaults, {
      name: "brick",
      state: "idle",
      collision: true,
      static: false
    }),
    animations: {
      idle: {
        sequences: [2]
      },
      bounce: {
        sequences: [
          {frame: 2, x: 0, y: -8},
          {frame: 2, x: 0, y: -8},
          {frame: 2, x: 0, y: -4},
          {frame: 2, x: 0, y: 0}
        ],
        delay: 50
      }
    },
    initialize: function(attributes, options) {
      Backbone.AnimatedTile.prototype.initialize.apply(this, arguments);
      this.on("hit", this.hit, this);
    },
    hit: function(sprite, dir, dir2) {
      if (sprite.get("hero") && dir == "bottom") {
        var tile = this;
        this.set({state: "bounce", sequenceIndex: 0});
        this.world.setTimeout(function() {
          tile.set({state: "idle"});
        }, 200);
      } else if (dir == "top") {
        sprite.trigger("hit", this, "bottom");
      }

      return this;
    }
  });

  var NewTile = extendSprite("Brick", "brick-top");
  animations = NewTile.prototype.animations;
  animations.idle.sequences = [1];
  _.each(animations.bounce.sequences, function(sequence) {sequence.frame  = 1;});

  NewTile = extendSprite("Brick", "brick-ug");
  animations = NewTile.prototype.animations;
  animations.idle.sequences = [118];
  _.each(animations.bounce.sequences, function(sequence) {sequence.frame  = 118;});

  NewTile = extendSprite("Brick", "brick-top-ug");
  animations = NewTile.prototype.animations;
  animations.idle.sequences = [117];
  _.each(animations.bounce.sequences, function(sequence) {sequence.frame  = 117;});


  Backbone.QuestionBlock = Backbone.AnimatedTile.extend({
    defaults: _.extend({}, Backbone.Tile.prototype.defaults, {
      name: "question-block",
      state: "idle",
      collision: true,
      static: false
    }),
    animations: {
      idle: {
        sequences: [23, 23, 24, 25, 24, 23],
        delay: 150
      },
      bounce: {
        sequences: [
          {frame: 3, x: 0, y: -8},
          {frame: 3, x: 0, y: -8},
          {frame: 3, x: 0, y: -4},
          {frame: 3, x: 0, y: 0}
        ],
        delay: 50
      },
      empty: {
        sequences: [3]
      }
    },
    initialize: function(attributes, options) {
      Backbone.AnimatedTile.prototype.initialize.apply(this, arguments);
      this.on("hit", this.hit, this);
    },
    hit: function(sprite, dir, dir2) {
      if (!sprite || !sprite.get("hero") || dir != "bottom") return;
      if (this.get("state") != "idle") return;

      var tile = this;
      this.set({state: "bounce", sequenceIndex: 0});

      if (this.world)
        this.world.add(new Backbone.FlyingPennie({
          x: this.get("x"),
          y: this.get("y")
        }));

      setTimeout(function() {
        tile.set({state: "empty"});
      }, 200);
    }
  });

  NewTile = extendSprite("QuestionBlock", "question-block-ug");
  animations = NewTile.prototype.animations;
  animations.idle.sequences = [139, 139, 140, 141, 140, 139];
  _.each(animations.bounce.sequences, function(sequence) {sequence.frame  = 119;});
  animations.empty.sequences = [119];

  NewTile = extendSprite("QuestionBlock", "invisible-question-block");
  animations = NewTile.prototype.animations;
  animations.idle.sequences = [];
  _.each(animations.bounce.sequences, function(sequence) {sequence.frame  = 3;});
  animations.empty.sequences = [3];

  NewTile = extendSprite("QuestionBlock", "invisible-question-block-ug");
  animations = NewTile.prototype.animations;
  animations.idle.sequences = [];
  _.each(animations.bounce.sequences, function(sequence) {sequence.frame  = 119;});
  animations.empty.sequences = [119];


}).call(this);