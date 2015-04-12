(function() {

  /**
   *
   * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
   *
   * Copyright (c) 2014 Martin Drapeau
   * https://github.com/martindrapeau/backbone-game-engine
   *
   */
   
  function drawRoundRect(ctx, x, y, width, height, borderRadius, fill, stroke) {
    if (typeof stroke == "undefined" ) stroke = true;
    if (typeof borderRadius === "undefined") borderRadius = 5;
    ctx.save();
    ctx.beginPath();
    ctx.moveTo(x + borderRadius, y);
    ctx.lineTo(x + width - borderRadius, y);
    ctx.quadraticCurveTo(x + width, y, x + width, y + borderRadius);
    ctx.lineTo(x + width, y + height - borderRadius);
    ctx.quadraticCurveTo(x + width, y + height, x + width - borderRadius, y + height);
    ctx.lineTo(x + borderRadius, y + height);
    ctx.quadraticCurveTo(x, y + height, x, y + height - borderRadius);
    ctx.lineTo(x, y + borderRadius);
    ctx.quadraticCurveTo(x, y, x + borderRadius, y);
    ctx.closePath();
    if (stroke) {
      ctx.strokeStyle = stroke;
      ctx.stroke();
    }
    if (fill) {
      ctx.fillStyle = fill;
      ctx.fill();
    }
    ctx.restore();
  }

  function drawRect(ctx, x, y, width, height, fill, stroke) {
    ctx.save();
    if (fill) {
      ctx.fillStyle = fill;
      ctx.fillRect(x, y, width, height);
    }
    if (stroke) {
      ctx.strokeStyle = stroke;
      ctx.strokeRect(x, y, width, height);
    }
    ctx.restore();
  }

  function drawCircle(ctx, x, y, borderRadius, fill, stroke) {
    ctx.save();
    ctx.beginPath();
    ctx.arc(x, y, borderRadius, 0, 2*Math.PI, false);
    if (stroke) {
      ctx.strokeStyle = stroke;
      ctx.stroke();
    }
    if (fill) {
      ctx.fillStyle = fill;
      ctx.fill();
    }
    ctx.restore();
  }

  _.extend(window, {
    drawRect: drawRect,
    drawRoundRect: drawRoundRect,
    drawCircle: drawCircle
  });

}).call(this);