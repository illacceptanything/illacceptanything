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
      walkVelocity = 50,
      fallAcceleration = 1200,
      fallVelocity = 600;

  var animations = {
    "idle-left": {
      sequences: [0],
      delay: sequenceDelay,
      velocity: 0,
      scaleX: 1,
      scaleY: 1
    },
    "idle-right": {
      sequences: [0],
      delay: sequenceDelay,
      velocity: 0,
      scaleX: -1,
      scaleY: 1
    },
    "walk-left": {
      sequences: [1, 0],
      delay: sequenceDelay,
      velocity: -walkVelocity,
      scaleX: 1,
      scaleY: 1
    },
    "walk-right": {
      sequences: [1, 0],
      delay: sequenceDelay,
      velocity: walkVelocity,
      scaleX: -1,
      scaleY: 1
    },
    "fall-left": {
      sequences: [0],
      delay: sequenceDelay,
      velocity: -walkVelocity,
      yVelocity: fallVelocity,
      yAcceleration: fallAcceleration,
      scaleX: 1,
      scaleY: 1
    },
    "fall-right": {
      sequences: [0],
      delay: sequenceDelay,
      velocity: walkVelocity,
      yVelocity: fallVelocity,
      yAcceleration: fallAcceleration,
      scaleX: -1,
      scaleY: 1
    },
    "ko-left": {
      sequences: [0],
      delay: sequenceDelay,
      velocity: -walkVelocity,
      yVelocity: fallVelocity,
      yAcceleration: fallAcceleration,
      scaleX: 1,
      scaleY: -1
    },
    "ko-right": {
      sequences: [0],
      delay: sequenceDelay,
      velocity: walkVelocity,
      yVelocity: fallVelocity,
      yAcceleration: fallAcceleration,
      scaleX: -1,
      scaleY: -1
    }
  };

  var hurtAnimation = {
        sequences: [0],
        delay: 300,
        yVelocity: fallVelocity,
        yAcceleration: fallAcceleration
      };
  animations["idle-hurt-left"] = _.extend({}, animations["idle-left"], hurtAnimation);
  animations["idle-hurt-right"] = _.extend({}, animations["idle-right"], hurtAnimation);
  animations["walk-hurt-left"] = _.extend({}, animations["walk-left"], hurtAnimation);
  animations["walk-hurt-right"] = _.extend({}, animations["walk-right"], hurtAnimation);
  animations["fall-hurt-left"] = _.extend({}, animations["fall-left"], hurtAnimation);
  animations["fall-hurt-right"] = _.extend({}, animations["fall-right"], hurtAnimation);


  Backbone.Character = Backbone.Sprite.extend({
    defaults: _.extend({}, Backbone.Sprite.prototype.defaults, {
      name: "character",
      type: "character",
      width: 32,
      height: 32,
      zIndex: 1,
      spriteSheet: undefined,
      state: "walk-left",
      velocity: 0,
      yVelocity: 0,
      collision: true,
      static: false,
      visible: true,
      health: 1,
      attackDamage: 1,
      floor: null,
      ceiling: null,
      aiDelay: 1000
    }),
    animations: animations,
    initialize: function(attributes, options) {
      Backbone.Sprite.prototype.initialize.apply(this, arguments);
      options || (options = {});

      this.on("attach", this.onAttach, this);
      this.on("detach", this.onDetach, this);

      this.on("hit", this.hit, this);
      this.on("change:health", this.onHealthChange, this);
      this.on("beforeFall", this.onBeforeFall, this);
    },
    onAttach: function() {
      if (!this.engine) return;
      this.onDetach();
    },
    onDetach: function() {
    },
    onBeforeFall: function() {
      return this;
    },
    onHealthChange: function(model, health, options) {
      options || (options = {});
      var cur = this.getStateInfo(),
          dir = options.dir || cur.dir,
          opo = dir == "left" ? "right" : "left";
      
      if (health == 0)
        return this.knockout(options.sprite || null, dir, options.dir2 || null);
      else if (health < this.previous("health")) 
        return this.hurt(options.sprite || null, dir, options.dir2 || null);
      
      this.lastAIEvent = _.now();

      return this;
    },
    knockout: function(sprite, dir) {
      var opo = dir == "left" ? "right" : "left",
          state = this.buildState("ko", opo);
      this.whenAnimationEnds = null;
      this.set({
        state: state,
        velocity: this.animations[state].velocity,
        yVelocity: -this.animations[state].yVelocity,
        sequenceIndex: 0,
        collision: false
      });
      this.cancelUpdate = true;
      return this;
    },
    hurt: function(sprite, dir) {
      this.whenAnimationEnds = null;
      this.set({
        state: this.buildState("fall", "hurt", dir),
        velocity: this.animations["ko-"+dir].velocity,
        yVelocity: -this.animations["ko-"+dir].yVelocity,
        sequenceIndex: 0
      });
      return this;
    },
    hit: function(sprite, dir, dir2) {
      if (this._handlingSpriteHit) return this;
      this._handlingSpriteHit = sprite;

      var cur = this.getStateInfo();
      
      if (cur.mov2 == "hurt") return this;

      if (dir2 == "attack") {
        this.cancelUpdate = true;
        var attackDamage = sprite.get("attackDamage") || 1;
        this.set({health: Math.max(this.get("health") - attackDamage, 0)}, {sprite: sprite, dir: dir, dir2: dir2});
      } else if (cur.dir == dir && cur.mov2 == null) {
        this.cancelUpdate = true;
        this.set("state", this.buildState(cur.mov, cur.opo));
      }

      tis._handlingSpriteHit = undefined;
      return this;
    },
    startNewAnimation: function(state, attrs, done) {
      this.lastSequenceChangeTime = _.now();
      this.set(_.extend({
        state: state,
        sequenceIndex: 0
      }, attrs));
      this.whenAnimationEnds = done;
      return this;
    },
    updateSequenceIndex: function(dt) {
      var sequenceIndex = this.get("sequenceIndex"),
          animation = this.getAnimation(),
          delay = animation.delay || 0,
          now = _.now(),
          triggerAnimationEnd = false;

      if (!animation.sequences) {
        sequenceIndex = 0;
        this.lastSequenceChangeTime = now;
      } else if (sequenceIndex >= animation.sequences.length) {
        sequenceIndex = 0;
        this.lastSequenceChangeTime = now;
        triggerAnimationEnd = true;
      } else if (delay && now > this.lastSequenceChangeTime + delay) {
        sequenceIndex = sequenceIndex < animation.sequences.length-1 ? sequenceIndex + 1 : 0;
        this.lastSequenceChangeTime = now;
        if (sequenceIndex == 0) triggerAnimationEnd = true;
      }

      if (triggerAnimationEnd && typeof this.whenAnimationEnds == "function") {
        this.whenAnimationEnds.call(this);
        this.whenAnimationEnds = null;
      }

      return sequenceIndex;
    },
    ai: function(dt) {
      return this;
    },
    update: function(dt) {
      // Movements are only possible inside a world
      if (!this.world) return true;
      this.cancelUpdate = false;

      // Velocity and state
      var self = this,
          velocity = this.get("velocity") || 0,
          yVelocity = this.get("yVelocity") || 0,
          x = this.get("x"),
          y = this.get("y"),
          state = this.get("state"),
          cur = this.getStateInfo(),
          animation = this.getAnimation(),
          now = _.now(),
          aiDelay = this.get("aiDelay"),
          attrs = {};

      // Handle AI
      if (!this.lastAIEvent)
        this.lastAIEvent = now;
      else if (now > this.lastAIEvent + aiDelay) {
        this.ai(now - this.lastAIEvent);
        this.lastAIEvent = now;
        if (this.cancelUpdate) return true;
      }

      if ((cur.mov == "ko" || cur.mov2 == "hurt") &&
          this.get("sequenceIndex") == animation.sequences.length-1) {
        // No sequence change - stay on last one
      } else {
        attrs.sequenceIndex = this.updateSequenceIndex();
      }

      if (velocity != animation.velocity) velocity = animation.velocity || 0;

      if (cur.mov == "fall" || cur.mov == "ko" || cur.mov2 == "hurt") {
        if (yVelocity < animation.yVelocity)
          yVelocity += animation.yAcceleration * (dt/1000);

        if (yVelocity >= animation.yVelocity)
          yVelocity = animation.yVelocity;
        attrs.yVelocity = yVelocity;
      }


      // Collision detection
      var collision = this.get("collision"),
          tileWidth = this.get("width"),
          tileHeight = this.get("height"),
          paddingLeft = this.get("paddingLeft"),
          paddingRight = this.get("paddingRight"),
          paddingBottom = this.get("paddingBottom"),
          paddingTop = this.get("paddingTop"),
          charWidth = tileWidth - paddingLeft - paddingRight,
          charHeight = tileHeight - paddingTop - paddingBottom,
          charLeftX = Math.round(x + velocity * (dt/1000)) + paddingLeft,
          charRightX = charLeftX + charWidth,
          bottomWorld = this.world.height() + tileHeight,
          relativeVelocity = 0,
          bottomY = _.minNotNull([
            this.get("floor"),
            bottomWorld
          ]);

      var charBottomY, charTopY,
          bottomPlatform, sprite, i, type;
      function updateTopBottom() {
        charBottomY = Math.round(y + yVelocity * (dt/1000)) + tileHeight - paddingBottom,
        charTopY = charBottomY - charHeight,
        self.buildCollisionMap(charTopY, charRightX, charBottomY, charLeftX);
        if (collision)
          self.world.findCollisions(self.collisionMap, null, self, true);
      }
      updateTopBottom();

      for (i = 0; i < this.collisionMap.bottom.sprites.length; i++) {
        sprite = this.collisionMap.bottom.sprites[i];
        type = sprite.get("type")
        if (type == "tile" || type == "platform")
          bottomY = Math.min(bottomY, sprite.getTop(true));
        if (type == "platform") bottomPlatform = sprite;
      }

      if (yVelocity >= 0) {
        // Walking or Falling...
        if (charBottomY >= bottomY) {
          if (charBottomY >= bottomWorld) {
            this.world.remove(this);
            return false;
          }

          for (i = 0; i < this.collisionMap.bottom.sprites.length; i++)
            if (cur.mov != "ko")
              this.collisionMap.bottom.sprites[i].trigger("hit", this, "top");
          if (this.cancelUpdate) return this;

          // Stop falling because obstacle below
          attrs.yVelocity = yVelocity = 0;
          attrs.y = y = bottomY - tileHeight + paddingBottom;
          if (cur.mov == "fall")
            attrs.state = this.buildState("walk", cur.mov2, cur.dir);
          else if (cur.mov == "ko") {
            attrs.velocity = velocity = 0;
          }
          updateTopBottom();

          if (charBottomY == bottomY && bottomPlatform)
            relativeVelocity = bottomPlatform.get("velocity");

        } else if (cur.mov != "fall" && cur.mov != "ko" && charBottomY < bottomY) {
          // Start falling if no obstacle below
          attrs.state = this.buildState("fall", cur.mov2, cur.dir);

          if (cur.mov == "walk" && velocity != 0) {
            this.trigger("beforeFall");
            if (this.cancelUpdate) return true;
          }
        }

      } else if (yVelocity < 0) {
        // Jumping
        var topY = -400;
        for (i = 0; i < this.collisionMap.top.sprites.length; i++) {
          sprite = this.collisionMap.top.sprites[i];
          if (sprite.get("type") == "tile")
            topY = Math.max(topY, sprite.getBottom(true));
        }
        if (charTopY < topY) {
          attrs.yVelocity = yVelocity = 0;
          charTopY = topY;
          charBottomY = topY + charHeight;
          attrs.y = y = charBottomY - tileHeight;
          updateTopBottom();
        }

      }

      // When not in play mode, do not allow horizontal displacements or animations
      if (this.world.get("state") != "play") {
        attrs.velocity = velocity = 0;
        attrs.sequenceIndex = this.get("sequenceIndex");

      } else {
        
        // Walls and other obstacles
        if (velocity <= 0 && collision) {
          // Turn around if obstacle left
          var worldLeft = -tileWidth,
              leftX = worldLeft,
              leftCharacter;
          if (cur.mov != "ko" && cur.mov != "idle")
            for (i = 0; i < this.collisionMap.left.sprites.length; i++) {
              sprite = this.collisionMap.left.sprites[i];
              leftX = Math.max(leftX, sprite.getRight(true));
              if (sprite.get("type") == "character" &&
                  (!leftCharacter || sprite.getRight(true) > leftCharacter.getRight(true)))
                leftCharacter = sprite;
            }

          if (charLeftX <= leftX) {
            if (charLeftX <= worldLeft) {
              this.world.remove(this);
              return false;
            }
            if (leftCharacter && cur.mov2 != "hurt") {
              leftCharacter.trigger("hit", this, "right", cur.mov2);
              if (this.cancelUpdate) return true;
            }
            attrs.velocity = velocity = velocity * -1;
            attrs.state = this.buildState(cur.mov, cur.mov2, cur.opo);
            attrs.x = x = leftX - paddingLeft;
          }
        }

        if (velocity >= 0 && collision) {
          // Turn around if obstacle to the right
          var worldRight = this.world.width(),
              rightX = worldRight,
              rightCharacter;
          if (cur.mov != "ko" && cur.mov != "idle")
            for (i = 0; i < this.collisionMap.right.sprites.length; i++) {
              sprite = this.collisionMap.right.sprites[i];
              rightX = Math.min(rightX, sprite.getLeft(true));
              if (sprite.get("type") == "character" &&
                  (!rightCharacter || sprite.getLeft(true) < rightCharacter.getLeft(true)))
                rightCharacter = sprite;
            }

          if (charRightX >= rightX) {
            if (charRightX >= worldRight) {
              this.world.remove(this);
              return false;
            }
            if (rightCharacter && cur.mov2 != "hurt") {
              rightCharacter.trigger("hit", this, "left", cur.mov2);
              if (this.cancelUpdate) return true;
            }
            attrs.velocity = velocity = velocity * -1;
            attrs.state = this.buildState(cur.mov, cur.mov2, cur.opo);
            attrs.x = x = rightX - charWidth - paddingLeft;
          }
        }
      }

      if (velocity || relativeVelocity) attrs.x = x = x + Math.round((velocity + relativeVelocity) * (dt/1000));
      if (yVelocity) attrs.y = y = y + Math.round(yVelocity * (dt/1000));

      // Set modified attributes
      if (!_.isEmpty(attrs)) this.set(attrs);

      if (typeof this.onUpdate == "function") return this.onUpdate(dt);
      return true;
    },
    toggleDirection: function(dirIntent) {
      var cur = this.getStateInfo();
      this.set({state: this.buildState(cur.mov, cur.mov2, dirIntent)});
      return this;
    },
    getStateInfo: function(state) {
      var state = state || this.get("state"),
      pieces = state.split("-");
      if (pieces.length < 2) return {
        state: state,
        mov: state
      };

      var stateInfo = {};
      stateInfo.mov = pieces[0];
      stateInfo.mov2 = pieces.length == 3 ? pieces[1] : null;
      stateInfo.dir = pieces.length == 3 ? pieces[2] : pieces[1];
      stateInfo.opo = stateInfo.dir == "right" ? "left" : "right";
      return stateInfo;
    },
    isAttacking: function() {
      if (this.cancelUpdate) return false;
      var cur = this.getStateInfo();
      return cur.mov2 == "attack";
    },
    buildState: function(piece1, piece2, piece3) {
      var state = "";
      if (piece1) state += piece1;
      if (piece2) state += (state.length ? "-" : "") + piece2;
      if (piece3) state += (state.length ? "-" : "") + piece3;
      return state;
    },
    buildCollisionMap: function(top, right, bottom, left) {
      this.collisionMap || (this.collisionMap = {
        right: {x: 0, y: 0, dir: "right", sprites: [], sprite: null},
        left: {x: 0, y: 0, dir: "left", sprites: [], sprite: null},
        bottom: {x: 0, y: 0, dir: "bottom", sprites: [], sprite: null},
        top: {x: 0, y: 0, dir: "top", sprites: [], sprite: null}
      });

      var width = right - left,
          height = bottom - top;
      this.collisionMap.left.x = left;
      this.collisionMap.right.x = right;
      this.collisionMap.left.y = this.collisionMap.right.y = top + height*0.20;
      this.collisionMap.top.x = this.collisionMap.bottom.x = left + width*0.20;
      this.collisionMap.top.y = top;
      this.collisionMap.bottom.y = bottom;
      this.collisionMap.left.height = this.collisionMap.right.height = height*0.60;
      this.collisionMap.left.width = this.collisionMap.right.width = 0;
      this.collisionMap.top.width = this.collisionMap.bottom.width = width*0.60;
      this.collisionMap.top.height = this.collisionMap.bottom.height = 0;

      for (var m in this.collisionMap)
        if (this.collisionMap.hasOwnProperty(m)) {
          this.collisionMap[m].sprites.length = 0;
          this.collisionMap[m].sprite = null;
        }
    }
  });

}).call(this);