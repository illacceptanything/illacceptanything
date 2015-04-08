(function() {

  /**
  *
  * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
  *
  * Copyright (c) 2014 Martin Drapeau
  * https://github.com/martindrapeau/backbone-game-engine
  *
  */

  // Ensures the canvas is always full size and/or centered.
  //
  // On mobile:
  // Works in conjunction with meta tag viewport where width is set to the canvas width.
  // <meta name="viewport" content="width=960, user-scalable=no"/>
  // Will modify the canvas height to fit the device aspect ratio, never exceeding the
  // origin canvas height.
  // Set keepRatio to true to maintain your aspect ratio. In such a case, the viewport
  // is adjusted to the height of the canvas. The canvas is kept centered on screen (with
  // black bars left and right to fill empty space).
  //
  // On Desktop:
  // The viewport meta tag is ignored. Instead, the canvas is kept centered. The height
  // may be reduced if the window height is less than the canvas (to avoid scrolling).
  // Set keepRatio to maintain the canvas height.

  function adjustViewport(canvas, keepRatio) {

    var viewport = typeof document.querySelector == "function" ? document.querySelector("meta[name=viewport]") : null,
        mobile = "onorientationchange" in window ||
          window.navigator.msMaxTouchPoints ||
          window.navigator.isCocoonJS;

    function onResize() {
      if (window.innerWidth > window.innerHeight) {
        // Landscape
        canvas.style.left = _.max([0, (window.innerWidth - canvas.width) / 2]) + "px";
        if (mobile && viewport)
          viewport.setAttribute("content", "width=" + Math.ceil(canvas.height * window.innerWidth / window.innerHeight) + ",user-scalable=no");
      } else {
        // Portrait
        canvas.style.left = "0px";
        if (mobile && viewport)
          viewport.setAttribute("content", "width=" + canvas.width + ",user-scalable=no");
      }
    }

    if (mobile && !keepRatio) {
      canvas.height = Math.round(Math.min(canvas.height, canvas.width * Math.min(window.innerHeight, window.innerWidth) / Math.max(window.innerHeight, window.innerWidth) ));
    } else {
      if (!keepRatio)
        canvas.height = Math.round(Math.min(canvas.height, window.innerHeight));
      window.addEventListener("resize", _.debounce(onResize, 300));
      setTimeout(onResize, 10);
    }

  }

  _.extend(window, {
    adjustViewport: adjustViewport
  });

}).call(this);