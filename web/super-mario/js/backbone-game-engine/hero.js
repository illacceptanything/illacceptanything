(function() {

  // Velocity and acceleration values in absolute values
  var walkVelocity = 160,
      walkMinVelocity = 60,
      walkAcceleration = 150,
      runVelocity = 220,
      runMinVelocity = 100,
      runAcceleration = 400,
      releaseDeceleration = 200,
      skidDeceleration = 400,
      jumpVelocity = 650,
      jumpDeceleration = 1400,
      jumpHoldDeceleration = 900,
      fallAcceleration = 1200,
      airTurnaroundDeceleration = 400,
      fallVelocity = 600,
      idleDelay = 2500,
      walkDelay = 100,
      koDelay = 100,
      runDelay = 50,
      attackDelay = 50,
      attackSequences = [37, 38, 39, 38],
      jumpAttackSequences = [41],
      skidAttackSequences = [25],
      walkSequences = [22, 23, 24, 23],
      runSequences = [22, 23, 24, 23],
      hurtDelay = 300,
      hurtBounceVelocity = -300,
      hurtSequences = [27];

  var animations = {
    "idle-left": {
      sequences: [21],
      delay: idleDelay,
      velocity: 0,
      acceleration: 0,
      scaleX: -1,
      scaleY: 1
    },
    "idle-right": {
      sequences: [21],
      delay: idleDelay,
      velocity: 0,
      acceleration: 0,
      scaleX: 1,
      scaleY: 1
    },
    "walk-left": {
      sequences: walkSequences,
      delay: walkDelay,
      velocity: -walkVelocity,
      minVelocity: -walkMinVelocity,
      acceleration: walkAcceleration,
      scaleX: -1,
      scaleY: 1
    },
    "walk-right": {
      sequences: walkSequences,
      delay: walkDelay,
      velocity: walkVelocity,
      minVelocity: walkMinVelocity,
      acceleration: walkAcceleration,
      scaleX: 1,
      scaleY: 1
    },
    "run-left": {
      sequences: runSequences,
      delay: runDelay,
      velocity: -runVelocity,
      minVelocity: -runMinVelocity,
      acceleration: runAcceleration,
      scaleX: -1,
      scaleY: 1
    },
    "run-right": {
      sequences: runSequences,
      delay: runDelay,
      velocity: runVelocity,
      minVelocity: runMinVelocity,
      acceleration: runAcceleration,
      scaleX: 1,
      scaleY: 1
    },
    "release-left": {
      sequences: walkSequences,
      delay: walkDelay,
      velocity: 0,
      acceleration: releaseDeceleration,
      scaleX: -1,
      scaleY: 1
    },
    "release-right": {
      sequences: walkSequences,
      delay: walkDelay,
      velocity: 0,
      acceleration: releaseDeceleration,
      scaleX: 1,
      scaleY: 1
    },
    "skid-left": {
      sequences: [25],
      velocity: 0,
      acceleration: skidDeceleration,
      scaleX: 1,
      scaleY: 1
    },
    "skid-right": {
      sequences: [25],
      velocity: 0,
      acceleration: skidDeceleration,
      scaleX: -1,
      scaleY: 1
    },
    "jump-left": {
      sequences: [26],
      delay: 1000,
      velocity: -walkVelocity,
      acceleration: airTurnaroundDeceleration,
      yStartVelocity: -jumpVelocity,
      yEndVelocity: fallVelocity,
      yAscentAcceleration: jumpDeceleration,
      yHoldAscentAcceleration: jumpHoldDeceleration,
      yDescentAcceleration: fallAcceleration,
      scaleX: -1,
      scaleY: 1
    },
    "jump-right": {
      sequences: [26],
      delay: 1000,
      velocity: walkVelocity,
      acceleration: airTurnaroundDeceleration,
      yStartVelocity: -jumpVelocity,
      yEndVelocity: fallVelocity,
      yAscentAcceleration: jumpDeceleration,
      yHoldAscentAcceleration: jumpHoldDeceleration,
      yDescentAcceleration: fallAcceleration,
      scaleX: 1,
      scaleY: 1
    },
    "ko-left": {
      sequences: [27],
      delay: koDelay,
      velocity: 0,
      yVelocity: fallVelocity,
      yAcceleration: fallAcceleration,
      scaleX: -1,
      scaleY: 1
    },
    "ko-right": {
      sequences: [27],
      delay: koDelay,
      velocity: 0,
      yVelocity: fallVelocity,
      yAcceleration: fallAcceleration,
      scaleX: 1,
      scaleY: 1
    },
    "dead-left": {
      sequences: [27],
      velocity: 0,
      scaleX: -1,
      scaleY: 1
    },
    "dead-right": {
      sequences: [27],
      velocity: 0,
      scaleX: 1,
      scaleY: 1
    }
  };
  animations["idle-attack-left"] = _.extend({}, animations["idle-left"], {sequences: attackSequences, delay: attackDelay});
  animations["idle-attack-right"] = _.extend({}, animations["idle-right"], {sequences: attackSequences, delay: attackDelay});
  animations["walk-attack-left"] = _.extend({}, animations["walk-left"], {sequences: attackSequences, delay: attackDelay});
  animations["walk-attack-right"] = _.extend({}, animations["walk-right"], {sequences: attackSequences, delay: attackDelay});
  animations["run-attack-left"] = _.extend({}, animations["run-left"], {sequences: attackSequences, delay: attackDelay});
  animations["run-attack-right"] = _.extend({}, animations["run-right"], {sequences: attackSequences, delay: attackDelay});
  animations["release-attack-left"] = _.extend({}, animations["release-left"], {sequences: attackSequences, delay: attackDelay});
  animations["release-attack-right"] = _.extend({}, animations["release-right"], {sequences: attackSequences, delay: attackDelay});
  animations["jump-attack-left"] = _.extend({}, animations["jump-left"], {sequences: jumpAttackSequences, delay: attackDelay});
  animations["jump-attack-right"] = _.extend({}, animations["jump-right"], {sequences: jumpAttackSequences, delay: attackDelay});
  animations["skid-attack-left"] = _.extend({}, animations["skid-left"], {sequences: skidAttackSequences, delay: attackDelay});
  animations["skid-attack-right"] = _.extend({}, animations["skid-right"], {sequences: skidAttackSequences, delay: attackDelay});

  var hurtAnimation = {sequences: hurtSequences, delay: hurtDelay};
  animations["idle-hurt-left"] = _.extend({}, animations["idle-left"], hurtAnimation);
  animations["idle-hurt-right"] = _.extend({}, animations["idle-right"], hurtAnimation);
  animations["walk-hurt-left"] = _.extend({}, animations["walk-left"], hurtAnimation);
  animations["walk-hurt-right"] = _.extend({}, animations["walk-right"], hurtAnimation);
  animations["run-hurt-left"] = _.extend({}, animations["run-left"], hurtAnimation);
  animations["run-hurt-right"] = _.extend({}, animations["run-right"], hurtAnimation);
  animations["release-hurt-left"] = _.extend({}, animations["release-left"], hurtAnimation);
  animations["release-hurt-right"] = _.extend({}, animations["release-right"], hurtAnimation);
  animations["jump-hurt-left"] = _.extend({}, animations["jump-left"], hurtAnimation);
  animations["jump-hurt-right"] = _.extend({}, animations["jump-right"], hurtAnimation);
  animations["skid-hurt-left"] = _.extend({}, animations["skid-left"], hurtAnimation);
  animations["skid-hurt-right"] = _.extend({}, animations["skid-right"], hurtAnimation);

  Backbone.Hero = Backbone.Character.extend({
    defaults: _.extend({}, Backbone.Character.prototype.defaults, {
      name: "hero",
      type: "character",
      hero: true,
      spriteSheet: undefined,
      width: 32,
      height: 64,
      paddingLeft: 0,
      paddingRight: 0,
      paddingTop: 32,
      paddingBottom: 0,
      state: "idle-right",
      velocity: 0,
      acceleration: 0,
      yVelocity: 0,
      yAcceleration: 0,
      collision: true,
      dead: false,
      health: 1,
      healthMax: 2,
      attackDamage: 1,
      coins: 0,
      ignoreInput: false,
      canAttack: false,
      canTurnInJump: false
    }),
    animations: animations,
    saveAttributes: _.union(
      Backbone.Character.prototype.saveAttributes,
      ["nextState", "velocity", "acceleration", "yVelocity", "yAcceleration"]
    ),
    initialize: function(attributes, options) {
      options || (options = {});
      Backbone.Character.prototype.initialize.apply(this, arguments);

      this.input = options.input;
    },
    onAttach: function() {
      if (this.input) {
        this.stopListening(this.input);
        this.listenTo(this.input, "change:right", _.partial(this.dirToggled, "right"));
        this.listenTo(this.input, "change:left", _.partial(this.dirToggled, "left"));
        this.listenTo(this.input, "change:buttonB", this.buttonBToggled);
        this.listenTo(this.input, "change:buttonA", this.buttonAToggled);
      }
    },
    onDetach: function() {
      if (this.input) this.stopListening(this.input);
    },
    toggleDirection: function(dirIntent) {
      return this.dirToggled(dirIntent);
    },
    ignoreInput: function() {
      if (this.get("ignoreInput") || this.get("dead")) return true;
      var cur = this.getStateInfo();
      if (cur.mov == "ko" || cur.mov == "dead" || cur.mov2 == "hurt") return true;
      return false;
    },
    // User input toggled in right or left direction.
    // Can be pressed or depressed
    dirToggled: function(dirIntent) {
      if (this.ignoreInput()) return this;

      if (dirIntent != "left" && dirIntent != "right")
        throw "Invalid or missing dirIntent. Must be left or right."

      var cur = this.getStateInfo(),
          now = _.now(),
          opoIntent = dirIntent == "right" ? "left" : "right",
          dirPressed = this.input ? this.input[dirIntent+"Pressed"]() : false,
          opoPressed = this.input ? this.input[opoIntent+"Pressed"]() : false,
          run = this.input ? this.input.buttonBPressed() : false,
          velocity = this.get("velocity"),
          attrs = {};

      if (dirPressed) {
        // Pressed. Intent to move in that direction
        if (cur.mov == "jump") {
          if (this.get("canTurnInJump"))
            attrs.state = this.buildState("jump", cur.mov2, dirIntent);
          if (dirIntent != cur.dir && velocity)
            attrs.nextState = this.buildState("skid", cur.mov2, opoIntent);
          else
            attrs.nextState = this.buildState(run ? "run" : "walk", cur.mov2, dirIntent);
        } else if (cur.dir == dirIntent || cur.mov == "idle") {
          // Start walking or running
          attrs.state = this.buildState(run ? "run" : "walk", cur.mov2, dirIntent);
          var animation = this.getAnimation(attrs.state);
          if (animation.minVelocity && Math.abs(velocity) < Math.abs(animation.minVelocity))
            attrs.velocity = animation.minVelocity;
          this.startWalk = now;
        } else if (cur.dir == opoIntent) {
          // Skid trying to stop before turning
          attrs.state = this.buildState("skid", cur.mov2, opoIntent);
          attrs.nextState = this.buildState(run ? "run" : "walk", cur.mov2, dirIntent);
        }
      } else if (opoPressed) {
      // Depressed but opposite direction still pressed. Intent = turnaround.
      // Handle by calling the opposite direction press event.
      this.dirToggled(opoIntent);
      } else {
        // Depressed. Intent = stop to idle
        if (cur.mov == "jump") {
          attrs.nextState = this.buildState("release", cur.mov2, dirIntent);
        } else {
          attrs.state = this.buildState("release", cur.mov2, dirIntent);
          attrs.nextState = this.buildState("idle", cur.mov2, dirIntent);
          if (now < this.startWalk + 250)
            attrs.velocity = velocity = velocity * 0.5;
        }
      }

      if (!_.isEmpty(attrs)) this.set(attrs);

      return this;
    },
    // Attack and run
    buttonBToggled: function() {
      if (this.ignoreInput()) return this;

      var cur = this.getStateInfo(),
          pressed = this.input ? this.input.buttonBPressed() : false;

      if (pressed && cur.mov == "walk") {
        cur.mov = "run";
        this.set("state",  this.buildState(cur.mov, cur.mov2, cur.dir));
        this.cancelUpdate = true;
      } else if (!pressed && cur.mov == "run") {
        cur.mov = "walk";
        this.set("state",  this.buildState(cur.mov, cur.mov2, cur.dir));
        this.cancelUpdate = true;
      }

      if (!this.get("canAttack")) return this;

      if (pressed && cur.mov2 != "attack") {
        var nextState = this.get("nextState");
        if (nextState) {
          var nex = this.getStateInfo(nextState);
          nextState = this.buildState(nex.mov, "attack", nex.dir);
        }
        this.startNewAnimation(this.buildState(cur.mov, "attack", cur.dir), {nextState: nextState}, this.endAttack);
        this.cancelUpdate = true;
      } else if (!pressed && cur.mov2 == "attack") {
        this.endAttack();
      }

      return this;
    },
    endAttack: function() {
      var cur = this.getStateInfo(),
          attrs = {state: this.buildState(cur.mov, cur.dir)},
          nextState = this.get("nextState"),
          nex = nextState ? this.getStateInfo(nextState) : null;
      if (nextState) attrs.nextState = this.buildState(nex.mov, nex.dir);
      this.whenAnimationEnds = null;
      this.set(attrs);
      return this;
    },
    knockout: function(sprite, dir) {
      dir || (dir = cur.dir);
      var cur = this.getStateInfo(),
          opo = dir == "left" ? "right" : "left",
          state = this.buildState("ko", opo);
      
      this.set({
        state: state,
        velocity: this.animations[state].velocity,
        yVelocity: -this.animations[state].yVelocity,
        nextState: this.buildState("dead", null, opo),
        dead: true,
        collision: false
      });
      this.cancelUpdate = true;
      return this;
    },
    hurt: function(sprite, dir) {
      this.set({
        state: this.buildState("jump", "hurt", dir == "left" ? "right" : "left"),
        nextState: this.buildState("idle", null, dir),
        yVelocity: hurtBounceVelocity,
        velocity: hurtBounceVelocity * (dir == "left" ? -1 : 1) / 2,
        sequenceIndex: 0
      });
      return this;
    },
    isAttacking: function() {
      return this.attributes.state.indexOf("-attack") > 0;
    },
    hit: function(sprite, dir, dir2) {
      if (this._handlingSpriteHit) return this;
      this._handlingSpriteHit = sprite;

      var cur = this.getStateInfo(),
          type = sprite.get("type");

      if (type == "artifact") {
        switch (sprite.get("name")) {
          case "a-coin":
            this.cancelUpdate = true;
            this.set("coins", this.get("coins") + 1);
            break;
        }
      } else if (type == "character" && cur.mov2 != "hurt") {
        if (this.isAttacking() && cur.dir == dir) {
          sprite.trigger("hit", this, cur.opo);
        } else {
          if (sprite.isAttacking()) {
            this.cancelUpdate = true;
            var attackDamage = sprite.get("attackDamage") || 1;
            this.set({health: Math.max(this.get("health") - attackDamage, 0)}, {sprite: sprite, dir: dir, dir2: dir2});
          }
        }
      }

      this._handlingSpriteHit = undefined;
      return this;
    },
    // Jump
    buttonAToggled: function() {
      if (this.ignoreInput()) return this;

      var state = this.get("state"),
          cur = this.getStateInfo(),
          attrs = {};

      if (this.input && this.input.buttonAPressed() && cur.mov != "jump") {
        // Set new state (keep old as next)
        attrs.state = this.buildState("jump", cur.mov2, cur.dir);
        attrs.nextState = state;

        // Determine vertical velocity as a factor of horizontal velocity
        var jumpAnimation = this.getAnimation(attrs.state),
            velocity = this.get("velocity"),
            walkVelocity = this.getAnimation("walk-right").velocity,
            runVelocity = this.getAnimation("run-right").velocity,
            ratio = Math.abs((Math.abs(velocity) > walkVelocity ? velocity : walkVelocity) / runVelocity);
        attrs.yVelocity = Math.round(jumpAnimation.yStartVelocity * (ratio + (1-ratio)/2));

        var heroWidth = this.get("width"),
            tileHeight = this.get("height"),
            heroHeight = tileHeight - this.get("paddingTop") - this.get("paddingBottom"),
            heroBottomY = Math.round(this.get("y") - 4) + tileHeight - this.get("paddingBottom"),
            heroTopY = heroBottomY - heroHeight,
            heroLeftX = this.get("x"),
            topLeftTile = heroTopY > 0 ? this.world.findAt(heroLeftX + heroWidth*0.4, heroTopY, "tile", this, true) : null,
            topRightTile = heroTopY > 0 ? this.world.findAt(heroLeftX + heroWidth*0.6, heroTopY, "tile", this, true) : null;
        if (topLeftTile || topRightTile) attrs.yVelocity = -2*60;

        // Keep the horizontal velocity
        jumpAnimation.minY = (this.get("y") - this.world.height()) * ratio;
      }
      if (!_.isEmpty(attrs)) this.set(attrs);

      return this;
    },
    sequenceDelay: function(animation) {
      var velocity = this.get("velocity");
      return animation.velocity && velocity ?
        animation.delay * animation.velocity / velocity :
        animation.delay;
    },
    update: function(dt) {
      // Movements are only possible inside a world
      if (!this.world) return true;
      this.cancelUpdate = false;

      // Velocity and state
      var hero = this,
          dead = this.get("dead"),
          input = !this.get("ignoreInput") ? this.input : null,
          velocity = this.get("velocity") || 0,
          yVelocity = this.get("yVelocity") || 0,
          yAcceleration = null,
          x = this.get("x"),
          y = this.get("y"),
          state = this.get("state"),
          cur = this.getStateInfo(),
          animation = this.getAnimation(),
          nextState = this.get("nextState"),
          nex = this.getStateInfo(nextState),
          nextAnimation = nextState ? (this.getAnimation(nextState) || {}) : null,
          attrs = {};

      attrs.sequenceIndex = this.updateSequenceIndex();

      switch (cur.mov + "-" + cur.dir) {
        case "walk-right":
        case "run-right":
        case "release-left":
        case "skid-left":
          if (velocity < animation.velocity)
            velocity += Math.round(animation.acceleration * (dt/1000));
          if (velocity >= animation.velocity) {
            velocity = animation.velocity;
            if (nextState) {
              attrs.state = nextState;
              animation = nextAnimation;
              if (animation.minVelocity && Math.abs(velocity) < Math.abs(animation.minVelocity))
                velocity = animation.minVelocity;
              attrs.nextState = null;
            }
          }
          attrs.velocity = velocity;
          break;

        case "walk-left":
        case "run-left":
        case "release-right":
        case "skid-right":
          if (velocity > animation.velocity)
            velocity -= Math.round(animation.acceleration * (dt/1000));
          if (velocity <= animation.velocity) {
            velocity = animation.velocity;
            if (nextState) {
              attrs.state = nextState;
              animation = nextAnimation;
              if (animation.minVelocity && Math.abs(velocity) < Math.abs(animation.minVelocity))
                velocity = animation.minVelocity;
              attrs.nextState = null;
            }
          }
          attrs.velocity = velocity;
          break;
      }

      switch (cur.mov) {
        case "idle":
          // TO DO: This should never happen - but seems to. Figure out why...
          if (velocity != 0) {
            if (input && input.rightPressed())
              this.toggleDirection("right");
            else if (input && input.leftPressed())
              this.toggleDirection("left");
            else
              attrs.velocity = velocity = 0;
          }
          break;

        case "jump":
          // Update vertical velocity. Determine proper vertical acceleration.
          if (yVelocity < animation.yEndVelocity) {
            yAcceleration = yVelocity < 0 ? animation.yAscentAcceleration : animation.yDescentAcceleration;
            if (yVelocity < 0 && input && input.buttonAPressed() && y > animation.minY)
              yAcceleration = animation.yHoldAscentAcceleration;
            yVelocity += yAcceleration * (dt/1000);
          }
          if (yVelocity >= animation.yEndVelocity)
            yVelocity = animation.yEndVelocity;
          attrs.yVelocity = yVelocity;

          // Update horizontal velocity if trying to turnaround
          if (input && input.leftPressed() && velocity > -Math.abs(animation.velocity)) {
            velocity -= Math.abs(animation.acceleration) * (dt/1000);
            attrs.velocity = velocity;
          } else if (input && input.rightPressed() && velocity < Math.abs(animation.velocity)) {
            velocity += Math.abs(animation.acceleration) * (dt/1000);
            attrs.velocity = velocity;
          }
          break;

        case "ko":
          if (yVelocity < animation.yVelocity)
            yVelocity += animation.yAcceleration * (dt/1000);

          if (yVelocity >= animation.yVelocity)
            yVelocity = animation.yVelocity;
          attrs.yVelocity = yVelocity;
          break;
      }

      // Collision detection
      var collision = this.get("collision"),
          tileWidth = this.get("width"),
          tileHeight = this.get("height"),
          paddingLeft = this.get("paddingLeft"),
          paddingRight = this.get("paddingRight"),
          paddingBottom = this.get("paddingBottom"),
          paddingTop = this.get("paddingTop"),
          heroWidth = tileWidth - paddingLeft - paddingRight,
          heroHeight = tileHeight - paddingTop - paddingBottom,
          heroLeftX = Math.round(x + velocity * (dt/1000)) + paddingLeft,
          heroRightX = heroLeftX + heroWidth,
          relativeVelocity = 0;

      var heroBottomY, heroTopY,
          bottomPlatform, sprite, i;
      function updateTopBottom() {
        heroBottomY = Math.round(y + yVelocity * (dt/1000)) + tileHeight - paddingBottom;
        heroTopY = heroBottomY - heroHeight;
        hero.buildCollisionMap(heroTopY, heroRightX, heroBottomY, heroLeftX);
        if (collision)
          hero.world.findCollisions(hero.collisionMap, null, hero, true);
      }
      updateTopBottom();

      if (yVelocity >= 0) {
        // Standing or falling, implement gravity
        var bottomWorld = this.world.height() + tileHeight,
            floor = this.get("floor") || bottomWorld,
            bottomY = Math.min(floor, bottomWorld);

        for (i = 0; i < this.collisionMap.bottom.sprites.length; i++) {
          sprite = this.collisionMap.bottom.sprites[i];
          bottomY = Math.min(bottomY, sprite.getTop(true));
          if (sprite.get("type") == "platform") bottomPlatform = sprite;
        }

        if (cur.mov == "jump" && cur.mov2 == null) attrs.sequenceIndex = 1;

        if (heroBottomY >= bottomWorld) {
          attrs.y = y = bottomY - tileHeight + paddingBottom;
          attrs.velocity = velocity = 0;
          attrs.yVelocity = yVelocity = 0;
          attrs.state = this.buildState("dead", cur.dir);
          attrs.dead = true;
        }

        function land(bottomY) {
          attrs.yVelocity = yVelocity = 0;
          attrs.y = y = bottomY - tileHeight + paddingBottom;
          updateTopBottom();
          attrs.state = nextState;
          if (nex.move == "walk" || nex.move == "run")
            attrs.nextState = hero.buildState(input && input.buttonBPressed() ? "run" : "walk", cur.mov2, nex.dir);
          else if (nex.mov == "skid")
            attrs.nextState = hero.buildState(input && input.buttonBPressed() ? "run" : "walk", cur.mov2, nex.opo);
          else if(nex.mov == "release")
            attrs.nextState = hero.buildState("idle", cur.mov2, nex.dir);
          else if (nex.mov == "dead") {
            attrs.velocity = velocity = 0;
          }
        }

        if (yVelocity > 0 && heroBottomY >= bottomY) {
          // Stop falling
          land(bottomY);
          for (i = 0; i < this.collisionMap.bottom.sprites.length; i++)
            this.collisionMap.bottom.sprites[i].trigger("hit", this, "top", cur.dir);
          if (this.cancelUpdate) return true;
        } else if (cur.mov != "jump" && yVelocity == 0 && heroBottomY < bottomY) {
          // Start falling if no obstacle below
          attrs.nextState = state;
          attrs.state = this.buildState("jump", cur.mov2, cur.dir);
        } else if (yVelocity == 0 && heroBottomY == bottomY) {
          if (bottomPlatform)
            relativeVelocity = bottomPlatform.get("velocity");
        }

      } else {
        // Velocity is negative (going up). Stop if obstacle above.
        var topY = Math.max(-400, this.get("ceiling") || -400);
        for (i = 0; i < this.collisionMap.top.sprites.length; i++)
          if (!dead && heroTopY > 0 )
            topY = Math.max(topY, this.collisionMap.top.sprites[i].getBottom(true));

        if (cur.mov == "jump" && cur.mov2 == null) attrs.sequenceIndex = 0;

        if (heroTopY < topY) {
          attrs.yVelocity = yVelocity = 0;
          attrs.y = y = topY - paddingTop;
          updateTopBottom();
          for (i = 0; i < this.collisionMap.top.sprites.length; i++)
            this.collisionMap.top.sprites[i].trigger("hit", this, "bottom", cur.dir);
          if (this.cancelUpdate) return true;
        }
      }

      if (velocity <= 0) {
        // Stop if obstacle left
        var leftX = 0;
        for (i = 0; i < this.collisionMap.left.sprites.length; i++)
          if (heroTopY > 0 )
            leftX = Math.max(leftX, this.collisionMap.left.sprites[i].getRight(true));

        if (heroLeftX <= leftX) {
          attrs.velocity = velocity = 0;
          attrs.x = x = leftX - paddingLeft;
          for (i = 0; i < this.collisionMap.left.sprites.length; i++)
            this.collisionMap.left.sprites[i].trigger("hit", this, "right", cur.mov2);
          if (this.cancelUpdate) return true;
        }
      }

      if (velocity >= 0) {
        // Stop if obstacle to the right
        var rightX = this.world.width();
        for (i = 0; i < this.collisionMap.right.sprites.length; i++)
          if (heroTopY > 0 )
            rightX = Math.min(rightX, this.collisionMap.right.sprites[i].getLeft(true));

        if (heroRightX >= rightX) {
          attrs.velocity = velocity = 0;
          attrs.x = x = rightX - heroWidth - paddingLeft;
          for (i = 0; i < this.collisionMap.right.sprites.length; i++)
            this.collisionMap.right.sprites[i].trigger("hit", this, "left", cur.mov2);
          if (this.cancelUpdate) return true;
        }
      }

      if (velocity || relativeVelocity) attrs.x = x = x + Math.round((velocity + relativeVelocity) * (dt/1000));
      if (yVelocity) attrs.y = y = y + Math.round(yVelocity * (dt/1000));

      // Set modified attributes
      if (!_.isEmpty(attrs)) this.set(attrs);

      if (this.debugPanel)
        this.debugPanel.set({velocity: velocity});

      return true;
    }
  });

}).call(this);