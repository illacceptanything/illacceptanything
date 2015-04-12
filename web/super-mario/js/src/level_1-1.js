/**
 *
 * Backbone Game Engine - An elementary HTML5 canvas game engine using Backbone.
 *
 * Copyright (c) 2014 Martin Drapeau
 * https://github.com/martindrapeau/backbone-game-engine
 *
 */
window._world = {
  id: 3,
  tiles: [],
  x: 0,
  y: 0,
  tileWidth: 32,
  tileHeight: 32,
  width: 212,
  height: 17,
  backgroundColor: "rgba(66, 66, 255, 1)",
  name: "level_1-1",
  sprites: [
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 480,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 512,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 544,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 576,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 608,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 640,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 672,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 704,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 736,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 768,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 800,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 832,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 864,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 896,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 928,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 960,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 992,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1024,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1056,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1088,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1120,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1152,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1184,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1216,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1248,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1280,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1312,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1344,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1376,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1408,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1408,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 480,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 512,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 544,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 576,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 608,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 640,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 672,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 704,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 736,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 768,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 800,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 832,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 864,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 896,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 928,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 960,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 992,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1024,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1056,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1088,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1120,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1152,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1184,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1216,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1248,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1280,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1312,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1344,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1376,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1440,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1440,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1472,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1472,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1504,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1504,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1536,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1536,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1568,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1568,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1600,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1600,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1632,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1632,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1664,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1664,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1696,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1696,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1728,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1728,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1760,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1760,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1792,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1792,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1824,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1824,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1856,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1856,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1888,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1888,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1920,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1920,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1952,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1952,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1984,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 1984,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2016,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2016,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2048,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2048,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2080,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2080,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2112,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2112,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2144,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2144,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2176,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2176,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2272,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2272,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2304,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2304,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2336,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2336,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2368,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2368,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2400,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2400,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2432,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2432,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2464,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2464,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2496,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2496,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2528,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2528,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2560,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2560,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2592,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2592,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2624,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2624,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2656,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2656,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2688,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2688,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2720,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2720,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2848,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2848,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2880,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2880,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2912,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2912,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2944,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2944,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2976,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 2976,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3008,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3008,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3040,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3040,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3072,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3072,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3104,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3104,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3136,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3136,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3168,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3168,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3200,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3200,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3232,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3232,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3264,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3264,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3296,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3296,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3328,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3328,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3360,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3360,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3392,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3392,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3424,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3424,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3456,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3456,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3488,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3488,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3520,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3520,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3552,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3552,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3584,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3584,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3616,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3616,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3648,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3648,
      "y": 480
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 3584,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 3488,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 3392,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 3488,
      "y": 224
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 3232,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 3008,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 3200,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 3008,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2976,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2944,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2912,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2784,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2752,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2720,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2688,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2656,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2624,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2592,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2560,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2528,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 2464,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 2496,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 2048,
      "y": 320
    },
    {
      "name": "tube1",
      "state": "idle",
      "animationIndex": 0,
      "x": 1824,
      "y": 352
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1824,
      "y": 384
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1824,
      "y": 416
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1824,
      "y": 448
    },
    {
      "name": "tube2",
      "state": "idle",
      "animationIndex": 0,
      "x": 1856,
      "y": 352
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1856,
      "y": 384
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1856,
      "y": 416
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1856,
      "y": 448
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1504,
      "y": 384
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1504,
      "y": 416
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1504,
      "y": 448
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1248,
      "y": 448
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1248,
      "y": 416
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1472,
      "y": 448
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1472,
      "y": 416
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1472,
      "y": 384
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1216,
      "y": 416
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1216,
      "y": 448
    },
    {
      "name": "tube1",
      "state": "idle",
      "animationIndex": 0,
      "x": 1216,
      "y": 384
    },
    {
      "name": "tube1",
      "state": "idle",
      "animationIndex": 0,
      "x": 1472,
      "y": 352
    },
    {
      "name": "tube2",
      "state": "idle",
      "animationIndex": 0,
      "x": 1504,
      "y": 352
    },
    {
      "name": "tube2",
      "state": "idle",
      "animationIndex": 0,
      "x": 1248,
      "y": 384
    },
    {
      "name": "tube2",
      "state": "idle",
      "animationIndex": 0,
      "x": 928,
      "y": 416
    },
    {
      "name": "tube1",
      "state": "idle",
      "animationIndex": 0,
      "x": 896,
      "y": 416
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 896,
      "y": 448
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 928,
      "y": 448
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 736,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 672,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 704,
      "y": 224
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 1,
      "x": 512,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 640,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 704,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 768,
      "y": 352
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 608,
      "y": 128
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 864,
      "y": 160
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 1152,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 640,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 896,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 928,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 960,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 1184,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 1216,
      "y": 128
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1248,
      "y": 128
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 992,
      "y": 160
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 672,
      "y": 128
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 608,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 864,
      "y": 192
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1152,
      "y": 160
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 1184,
      "y": 160
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 1216,
      "y": 160
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 896,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 928,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 960,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 640,
      "y": 160
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 672,
      "y": 160
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 992,
      "y": 192
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 1248,
      "y": 160
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 768,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 1344,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 1376,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 800,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 1408,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 1312,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 736,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 512,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 480,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 576,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 544,
      "y": 448
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 544,
      "y": 416
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 1600,
      "y": 384
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 1568,
      "y": 416
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 1536,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1664,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 1632,
      "y": 416
    },
    {
      "name": "bush5",
      "state": "idle",
      "animationIndex": 0,
      "x": 1600,
      "y": 448
    },
    {
      "name": "bush6",
      "state": "idle",
      "animationIndex": 0,
      "x": 1632,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1568,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 1600,
      "y": 416
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 2080,
      "y": 448
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 2080,
      "y": 416
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 2048,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 2112,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 1920,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 1952,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 1984,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 2304,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 2336,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 2016,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 1888,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 2272,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 2848,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 2880,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 2912,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 2944,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 3072,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 3104,
      "y": 416
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 3168,
      "y": 416
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 3200,
      "y": 448
    },
    {
      "name": "bush6",
      "state": "idle",
      "animationIndex": 0,
      "x": 3168,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 3104,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 3136,
      "y": 416
    },
    {
      "name": "bush5",
      "state": "idle",
      "animationIndex": 0,
      "x": 3136,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 3456,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 3488,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 3520,
      "y": 448
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 3616,
      "y": 416
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 3648,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 3584,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 3616,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 3552,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 3424,
      "y": 448
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 32,
      "y": 480
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 448,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 384,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 352,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 128,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 96,
      "y": 416
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 0,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 32,
      "y": 416
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 64,
      "y": 384
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 32,
      "y": 448
    },
    {
      "name": "bush6",
      "state": "idle",
      "animationIndex": 0,
      "x": 96,
      "y": 448
    },
    {
      "name": "bush5",
      "state": "idle",
      "animationIndex": 0,
      "x": 64,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 64,
      "y": 416
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 64,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 96,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 128,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 160,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 192,
      "y": 480
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 256,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 288,
      "y": 160
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 320,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 256,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 288,
      "y": 192
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 320,
      "y": 192
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 3136,
      "y": 384
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3680,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3680,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3712,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3712,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3744,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3744,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3776,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3776,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3808,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3808,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3840,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3840,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3872,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3872,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3904,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3904,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3936,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3936,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3968,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 3968,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4000,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4000,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4032,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4032,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4064,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4064,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4096,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4096,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4128,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4128,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4160,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4160,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4192,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4192,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4224,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4224,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4256,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4256,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4288,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4288,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4320,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4320,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4352,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4352,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4384,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4384,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4416,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4416,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4448,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4448,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4480,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4480,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4512,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4512,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4544,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4544,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4576,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4576,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4608,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4608,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4640,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4640,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4672,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4672,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4704,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4704,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4736,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4736,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4768,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4768,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4800,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4800,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4832,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4832,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4960,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4960,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4992,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 4992,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5024,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5024,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5056,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5056,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5088,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5088,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5120,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5120,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5152,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5152,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5184,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5184,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5216,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5216,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5248,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5248,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5280,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5280,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5312,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5312,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5344,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5344,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5376,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5376,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5408,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5408,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5440,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5440,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5472,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5472,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5504,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5504,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5536,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5536,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5568,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5568,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5600,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5600,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5632,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5632,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5664,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5664,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5696,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5696,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5728,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5728,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5760,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5760,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5792,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5792,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5824,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5824,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5856,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5856,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5888,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5888,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5920,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5920,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6080,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6080,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6112,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6112,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6144,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6144,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6176,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6176,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6208,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6208,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6240,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6240,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6272,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6272,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6304,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6304,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6368,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6368,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6400,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6400,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6432,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6432,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6464,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6464,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6496,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6496,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6528,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6528,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6560,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6560,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6592,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6592,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6624,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6624,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6656,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6656,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6688,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6688,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6720,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6720,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6752,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 6752,
      "y": 480
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6208,
      "y": 384
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6688,
      "y": 416
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 6720,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 6272,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 6656,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 6144,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 6176,
      "y": 416
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 6240,
      "y": 416
    },
    {
      "name": "bush5",
      "state": "idle",
      "animationIndex": 0,
      "x": 6208,
      "y": 448
    },
    {
      "name": "bush6",
      "state": "idle",
      "animationIndex": 0,
      "x": 6240,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 6176,
      "y": 448
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 6208,
      "y": 416
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 6688,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 6624,
      "y": 448
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6464,
      "y": 448
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6496,
      "y": 448
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6496,
      "y": 416
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6464,
      "y": 416
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6560,
      "y": 448
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6560,
      "y": 416
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6592,
      "y": 448
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6592,
      "y": 416
    },
    {
      "name": "brick-castle",
      "state": "idle",
      "animationIndex": 0,
      "x": 6464,
      "y": 384
    },
    {
      "name": "brick-castle",
      "state": "idle",
      "animationIndex": 0,
      "x": 6592,
      "y": 384
    },
    {
      "name": "brick-castle",
      "state": "idle",
      "animationIndex": 0,
      "x": 6560,
      "y": 320
    },
    {
      "name": "brick-castle",
      "state": "idle",
      "animationIndex": 0,
      "x": 6528,
      "y": 320
    },
    {
      "name": "brick-castle",
      "state": "idle",
      "animationIndex": 0,
      "x": 6496,
      "y": 320
    },
    {
      "name": "brick",
      "state": "idle",
      "animationIndex": 0,
      "x": 6528,
      "y": 352
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6432,
      "y": 160
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 6464,
      "y": 160
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 6464,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 6432,
      "y": 192
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 6400,
      "y": 192
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 6400,
      "y": 160
    },
    {
      "name": "flag-pole1",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 128
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 416
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 384
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 320
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 192
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 160
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5920,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5856,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5888,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5920,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5856,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5888,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5920,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5920,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5888,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5920,
      "y": 320
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 320
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 320
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 320
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 320
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 288
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 288
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 288
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5952,
      "y": 288
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 256
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 256
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 256
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 5760,
      "y": 128
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 5472,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 5760,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 5472,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 5504,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 5536,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 5568,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 5792,
      "y": 160
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 5824,
      "y": 160
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 5856,
      "y": 160
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 5600,
      "y": 192
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 5856,
      "y": 128
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 5600,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5824,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5792,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5568,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5536,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5504,
      "y": 160
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 5472,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 5408,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 5376,
      "y": 352
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 3,
      "x": 5440,
      "y": 352
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 5408,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 5376,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 5344,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4768,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4800,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4832,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4832,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4800,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4832,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4960,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4960,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4992,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4992,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4960,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5024,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4384,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4384,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4384,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4384,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4352,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4320,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4288,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4320,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4352,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4480,
      "y": 352
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4480,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4480,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4480,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4512,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4512,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4512,
      "y": 384
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4544,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4544,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4576,
      "y": 448
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 5216,
      "y": 128
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5248,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4896,
      "y": 160
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 5280,
      "y": 128
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 4928,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 5216,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 5248,
      "y": 160
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 4896,
      "y": 192
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 5280,
      "y": 160
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 4928,
      "y": 192
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 4320,
      "y": 160
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 4064,
      "y": 192
    },
    {
      "name": "cloud6",
      "state": "idle",
      "animationIndex": 0,
      "x": 3744,
      "y": 160
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 4256,
      "y": 160
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 4032,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 4000,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 3968,
      "y": 192
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 3712,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 4224,
      "y": 160
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 3936,
      "y": 192
    },
    {
      "name": "cloud4",
      "state": "idle",
      "animationIndex": 0,
      "x": 3680,
      "y": 160
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 3680,
      "y": 128
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 3936,
      "y": 160
    },
    {
      "name": "cloud1",
      "state": "idle",
      "animationIndex": 0,
      "x": 4224,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4256,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4288,
      "y": 128
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4032,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4000,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 3968,
      "y": 160
    },
    {
      "name": "cloud2",
      "state": "idle",
      "animationIndex": 0,
      "x": 3712,
      "y": 128
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 4320,
      "y": 128
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 4064,
      "y": 160
    },
    {
      "name": "cloud3",
      "state": "idle",
      "animationIndex": 0,
      "x": 3744,
      "y": 128
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 3776,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 4128,
      "y": 352
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 4160,
      "y": 352
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 3872,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 3840,
      "y": 448
    },
    {
      "name": "bush7",
      "state": "idle",
      "animationIndex": 0,
      "x": 3808,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 4416,
      "y": 448
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 4448,
      "y": 448
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 0,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 0,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 32,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 64,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 96,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 128,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 160,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 192,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 224,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 224,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 256,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 256,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 288,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 288,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 320,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 320,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 352,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 352,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 384,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 384,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 416,
      "y": 480
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 416,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 448,
      "y": 512
    },
    {
      "name": "ground",
      "state": "idle",
      "animationIndex": 0,
      "x": 448,
      "y": 480
    },
    {
      "name": "bush8",
      "state": "idle",
      "animationIndex": 0,
      "x": 416,
      "y": 448
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 3872,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 3904,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 3936,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 4096,
      "y": 224
    },
    {
      "name": "brick-top",
      "state": "idle",
      "animationIndex": 0,
      "x": 4192,
      "y": 224
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 5,
      "x": 4128,
      "y": 224
    },
    {
      "name": "question-block",
      "state": "idle",
      "animationIndex": 5,
      "x": 4160,
      "y": 224
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4736,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4768,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4800,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4832,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4864,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4960,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4992,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5024,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5056,
      "y": 448
    },
    {
      "name": "tube1",
      "state": "idle",
      "animationIndex": 0,
      "x": 5216,
      "y": 416
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 5216,
      "y": 448
    },
    {
      "name": "tube2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5248,
      "y": 416
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 5248,
      "y": 448
    },
    {
      "name": "bush1",
      "state": "idle",
      "animationIndex": 0,
      "x": 5120,
      "y": 448
    },
    {
      "name": "bush3",
      "state": "idle",
      "animationIndex": 0,
      "x": 5184,
      "y": 448
    },
    {
      "name": "bush2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5152,
      "y": 416
    },
    {
      "name": "bush4",
      "state": "idle",
      "animationIndex": 0,
      "x": 5152,
      "y": 448
    },
    {
      "name": "bush9",
      "state": "idle",
      "animationIndex": 0,
      "x": 5088,
      "y": 448
    },
    {
      "name": "tube3",
      "state": "idle",
      "animationIndex": 0,
      "x": 5728,
      "y": 448
    },
    {
      "name": "tube4",
      "state": "idle",
      "animationIndex": 0,
      "x": 5760,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5792,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5824,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5824,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5856,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5888,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5984,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 448
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6016,
      "y": 224
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6048,
      "y": 224
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 448
    },
    {
      "name": "mario",
      "state": "idle-right",
      "animationIndex": 0,
      "x": 64,
      "y": 418,
      "velocity": 0,
      "acceleration": 0,
      "yVelocity": 0,
      "yAcceleration": 0
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 1340,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 1559.25,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 1623.25,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 5591.25,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 5527.25,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 2656,
      "y": 160
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 2784,
      "y": 160
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 4000,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 4064,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 4128,
      "y": 416
    },
    {
      "name": "block2",
      "state": "idle",
      "animationIndex": 0,
      "x": 4352,
      "y": 384
    },
    {
      "name": "cloud5",
      "state": "idle",
      "animationIndex": 0,
      "x": 4288,
      "y": 160
    },
    {
      "name": "tube1",
      "state": "idle",
      "animationIndex": 0,
      "x": 5728,
      "y": 416
    },
    {
      "name": "tube2",
      "state": "idle",
      "animationIndex": 0,
      "x": 5760,
      "y": 416
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 352
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 288
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 256
    },
    {
      "name": "flag-pole2",
      "state": "idle",
      "animationIndex": 0,
      "x": 6336,
      "y": 224
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 3392,
      "y": 416
    },
    {
      "name": "turtle",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 3648,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 3296,
      "y": 416
    },
    {
      "name": "mushroom",
      "state": "walk-left",
      "animationIndex": 0,
      "x": 864,
      "y": 416
    }
  ],
  savedOn: "2014-04-24T01:00:45.204Z",
  state: "play"
};