var marqueeThis = function(id) {
  var styleTag = document.createElement("style");
  styleTag.innerHTML = "@-webkit-keyframes marquee { from { margin-left: 0; } to { margin-left: 100%; } };"
  document.head.appendChild(styleTag);

  var el = document.getElementById(id);
  el.style["-webkit-animation"] = "marquee 8s infinite";
  el.style["-webkit-animation-timing-function"] = "linear";
};

