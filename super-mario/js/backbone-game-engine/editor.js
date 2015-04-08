(function() {

  /**
   *
   * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
   *
   * Copyright (c) 2014 Martin Drapeau
   * https://github.com/martindrapeau/backbone-game-engine
   *
   */

  var pageImg  = new Image();
  pageImg.src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAgCAYAAAAbifjMAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAABYSURBVEhLYxheoB6I1SFM0gFIsycQ/wdikg1B1kyWISDFyJpHDRk1BBugqiGpUJosQ2CaQIbsh/JBNFGAIm+Mah7VTBSguFQGAYo0wwBFmkGAIs1kAgYGANR+XZBqOruRAAAAAElFTkSuQmCC";

  var drawSpriteFn = function(context, options) {
    options || (options = {});
    var animation = this.getAnimation(),
        sequenceIndex = this.get("sequenceIndex") || 0;
    if (!animation || animation.sequences.length == 0) return;
    if (sequenceIndex >= animation.sequences.length) sequenceIndex = 0;

    var sequence = animation.sequences[sequenceIndex]
        frameIndex = _.isNumber(sequence) ? sequence : sequence.frame,
        frame = this.spriteSheet.frames[frameIndex];

    var width = options.tileWidth,
        height = options.tileHeight;
    if (this.attributes.width > this.attributes.height && this.attributes.width > options.tileWidth) {
      height = this.attributes.height * options.tileWidth / this.attributes.width;
    } else if (this.attributes.height > this.attributes.width && this.attributes.height > options.tileHeight) {
      width = this.attributes.width * options.tileHeight / this.attributes.height;
    }

    context.drawImage(
      this.spriteSheet.img,
      frame.x, frame.y, frame.width, frame.height,
      this.get("x"), this.get("y"), width, height
    );

    if (typeof this.onDraw == "function") this.onDraw(context, options);
    return this;
  };

  // World Editor
  // Allows the user to place tiles and characters in the World.
  Backbone.WorldEditor = Backbone.Model.extend({
    defaults: {
      x: 136,
      y: 550,
      width: 820,
      height: 140,
      tileWidth: 32,
      tileHeight: 32,
      padding: 1,
      backgroundColor: "#333",
      selectColor: "#f00",
      selected: undefined,
      spriteNames: [],
      page: 0,
      pages: 1
    },
    initialize: function(attributes, options) {
      options || (options = {});

      if (!attributes || !attributes.spriteNames) throw "Missing attribute spriteNames";

      var defs = [];
      if (attributes.spriteNames.length && typeof attributes.spriteNames[0] == "object") {
        for (var page = 0; page < attributes.spriteNames.length; page++)
          for (var s = 0; s < attributes.spriteNames[page].length; s++)
            defs.push({name: attributes.spriteNames[page][s], page: page});
        this.set("pages", attributes.spriteNames.length);
      } else {
        defs = _.map(attributes.spriteNames, function(name) {
          return {name: name, page: 0};
        });
      }
      this.sprites = new Backbone.SpriteCollection(defs);
      this.sprites.each(function(sprite) {
        sprite.draw = drawSpriteFn;
      });

      this.world = options.world;
      if (!this.world && !_.isFunction(this.world.add))
        throw "Missing or invalid world option.";

      this.changePageButton = new Backbone.Button({
        width: 32, height: 50, borderRadius: 2,
        img: pageImg, imgX: 0, imgY: 0, imgWidth: 16, imgHeight: 32, imgMargin: 10
      });
      this.changePageButton.on("tap", this.changePage, this);

      this.debugPanel = options.debugPanel;

      _.bindAll(this,
        "onTap", "onDragStart", "onDrag", "onDragEnd",
        "getSelectedSprite", "onMouseMove"
      );

      this.on("attach", this.onAttach);
      this.on("detach", this.onDetach);

      var editor = this;
      this.on("change:page", function() {
        editor.set("selected", undefined);
      });
    },
    onAttach: function() {
      var world = this.world,
          engine = this.engine;
      this.onDetach();

      this.listenTo(this.engine, "tap", this.onTap);
      this.listenTo(this.engine, "dragstart", this.onDragStart);
      this.listenTo(this.engine, "dragmove", this.onDrag);
      this.listenTo(this.engine, "dragend", this.onDragEnd);

      $(document).on("mousemove.Edit", this.onMouseMove);

      this.sprites.each(function(sprite) {
        if (sprite.attributes.type == "tile" || sprite.attributes.type == "character") {
          sprite.engine = engine;
          sprite.trigger("attach", engine);
        }
      });

      if (this.get("pages") > 1) {
        this.changePageButton.engine = engine;
        this.changePageButton.trigger("attach");
      }

      this.positionSprites();
    },
    onDetach: function() {
      this.stopListening(this.engine);
      $(document).off(".Edit");

      this.sprites.each(function(sprite) {
        if (sprite.attributes.type == "tile" || sprite.attributes.type == "character") {
          sprite.engine = undefined;
          sprite.trigger("detach");
        }
      });

      this.changePageButton.trigger("detach");
    },
    changePage: function() {
      var page = this.get("page") + 1;
      if (page >= this.get("pages")) page = 0;
      this.set("page", page);
    },
    positionSprites: function() {
      var sp = this.toJSON(),
          x = sp.x + sp.tileWidth + 4*sp.padding,
          y = sp.y + 2*sp.padding,
          page = 0;

      this.sprites.each(function(sprite) {
        if (sprite.attributes.page > page) {
          page = sprite.attributes.page;
          x = sp.x + sp.tileWidth + 4*sp.padding;
          y = sp.y + 2*sp.padding;
        }

        sprite.set({x: x, y: y});
        x += sp.tileWidth + 2*sp.padding;

        if (x >= sp.x + sp.width - 2) {
          x = sp.x + 2*sp.padding;
          y += sp.tileHeight + 2*sp.padding;
        }
      });

      if (this.get("pages") > 1)
        this.changePageButton.set({
          x: sp.x + sp.width - this.changePageButton.get("width"),
          y: sp.y + sp.height - this.changePageButton.get("height")
        });

      return this;
    },

    update: function(dt) {
      for (var i = 0; i < this.sprites.models.length; i++)
        this.sprites.models[i].update(dt);
      this.changePageButton.update(dt);
      return true;
    },
    draw: function(context) {
      var sp = this.toJSON();

      // Fill background
      drawRect(context, sp.x, sp.y, sp.width, sp.height, sp.backgroundColor);

      // Highlight selected sprite
      var st = this.sprites.findWhere({name: sp.selected}),
          sx = st ? st.get("x") - 2 : sp.x,
          sy = st ? st.get("y") - 2 : sp.y,
          sw = sp.tileWidth + 4,
          sh = sp.tileHeight + 4;
      drawRect(context, sx, sy, sw, sh, sp.selectColor);

      // Draw sprites
      this.sprites.each(function(sprite) {
        if (sprite.attributes.page == sp.page) sprite.draw(context, sp);
      });

      // Highlight tile position (on desktop)
      if (this.mx != undefined && this.my != undefined) {
        var tileWidth = this.world.get("tileWidth"),
            tileHeight = this.world.get("tileHeight"),
            x = this.mx - this.mx % tileWidth + this.world.get("x"),
            y = this.my - this.my % tileHeight + this.world.get("y");

        context.save();
        context.rect(
          this.world.get("viewportLeft"),
          this.world.get("viewportTop"),
          context.canvas.width - this.world.get("viewportRight"),
          context.canvas.height - this.world.get("viewportBottom")
        );
        context.clip();

        context.beginPath();
        context.strokeStyle = "#FF0000";
        context.setLineDash([5,2]);
        context.rect(x, y, tileWidth, tileHeight);
        context.stroke();

        context.restore();
      }
      
      if (this.get("pages") > 1)
        this.changePageButton.draw(context);
    },

    getSelectedSprite: function() {
      var selected = this.get("selected");
      if (!selected) return null
      return this.sprites.findWhere({name: selected});
    },
    onTap: function(e) {
      if (e.target != this.engine.canvas || e.canvasHandled) return;

      var editor = this,
          sp = this.toJSON(),
          x = e.canvasX,
          y = e.canvasY;

      // Sprite selection?
      if (x >= sp.x && y >= sp.y && x <= sp.x + sp.width && y <= sp.y + sp.height) {
        editor.set({selected: null});
        this.sprites.each(function(sprite) {
          var s = sprite.toJSON();
          if (s.page == sp.page && x >= s.x && y >= s.y && x <= s.x + sp.tileWidth && y <= s.y + sp.tileHeight) {
            editor.set({selected: s.name});
            return false;
          }
        });
        return;
      }

      // Sprite placement
      if (y < sp.y) {
        var sprite = this.getSelectedSprite();
        x -= this.world.get("x");
        y -= this.world.get("y");
        this.world.cloneAtPosition(sprite, x, y);
      }
    },

    // Pan the world
    onDragStart: function(e) {
      var world = this.world,
          sp = this.toJSON(),
          x = e.canvasX,
          y = e.canvasY;
      if (x >= sp.x && y >= sp.y && x <= sp.x + sp.width && y <= sp.y + sp.height) return;
      world.startDragWorldX = world.get("x");
      world.startDragWorldY = world.get("y");
    },
    onDrag: function (e) {
      var world = this.world;
      if (!_.isNumber(world.startDragWorldX) || !_.isNumber(world.startDragWorldX)) return false;
      var x = world.startDragWorldX + e.canvasDeltaX,
          y = world.startDragWorldY + e.canvasDeltaY;
      if (x > 0) {
        x = 0;
      } else {
        var min = -(world.get("width") * world.get("tileWidth") - world.engine.canvas.width);
        if (x < min) x = min;
      }
      if (y > 0) {
        y = 0;
      } else {
        var min = -(world.get("height") * world.get("tileHeight") - world.engine.canvas.height + world.get("viewportBottom"));
        if (min > 0) min = 0;
        if (y < min) y = min;
      }
      world.set({x: x, y: y});
    },
    onDragEnd: function(e) {
      var world = this.world;
      world.startDragWorldX = undefined;
      world.startDragWorldY = undefined;
    },

    onMouseMove: function(e) {
      var mx = this.mx = e.pageX - this.world.get("x") - this.engine.canvas.offsetLeft + this.engine.canvas.scrollLeft,
          my = this.my = e.pageY - this.world.get("y") - this.engine.canvas.offsetTop + this.engine.canvas.scrollTop,
          id = this.world.getWorldIndex({x: mx, y: my}),
          sprites = this.world.filterAt(mx, my),
          nameOrIds = _.map(sprites, function(sprite) {
            if (sprite.get("type") == "tile") return sprite.get("name")
            return sprite.get("id");
          });

      if (this.debugPanel)
        this.debugPanel.set({sprites: nameOrIds, mx: mx, my: my});
    }

  });

}).call(this);