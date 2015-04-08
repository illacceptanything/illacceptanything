(function() {

  /**
   *
   * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
   *
   * Copyright (c) 2014 Martin Drapeau
   * https://github.com/martindrapeau/backbone-game-engine
   *
   */
   
  // Load this file to persist in local storage.
  // It will replace Backbone.World's save and fetch methods with
  // naive implementations.

  Backbone.World.prototype.fetch = function(options) {
      options || (options = {});

      var data = localStorage.getItem(this.id);
      if (data) {
        console.log("===== LOADING LOCAL STORAGE ====");
        this.set(JSON.parse(data));
      }

      if (_.isFunction(options.success)) options.success();

      return this;
  };
  
  Backbone.World.prototype.save = function(attributes, options) {
      options || (options = {});

      
      this.set({
        state: "play",
        sprites: this.sprites.map(function(sprite) {
          return sprite.toSave.apply(sprite);
        }),
        savedOn: new Date().toJSON()
      }, {silent: true});

      console.log("===== SAVING LOCAL STORAGE ====");
      localStorage.setItem(this.id, JSON.stringify(this.toJSON()));

      if (_.isFunction(options.success)) options.success();
      
      return this;
  };

}).call(this);