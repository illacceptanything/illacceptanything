/**
 * Pretty Keyboard Events v2.2.1
 * http://rightjs.org/plugins/keys
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
(function(a){a.Keys={version:"2.2.1"},a.Event.Keys={BACKSPACE:8,TAB:9,ENTER:13,ESC:27,SPACE:32,PAGEUP:33,PAGEDOWN:34,END:35,HOME:36,LEFT:37,UP:38,RIGHT:39,DOWN:40,INSERT:45,DELETE:46},[a.Document,a.Element,a.Window].each("include",{on:function(){var b=a.$A(arguments),c=b[0];if(typeof c==="string"){var d=c.split(/[\+\-\_ ]+/);d=(d[d.length-1]||"").toUpperCase();if(d in a.Event.Keys||/^[A-Z0-9]$/.test(d)){var e=/(^|\+|\-| )(meta|alt)(\+|\-| )/i.test(c),f=/(^|\+|\-| )(ctl|ctrl)(\+|\-| )/i.test(c),g=/(^|\+|\-| )(shift)(\+|\-| )/i.test(c),h=a.Event.Keys[d]||d.charCodeAt(0),i=b.slice(1),j=i.shift();typeof j==="string"&&(j=this[j]||function(){}),b=["keydown",function(a){var b=a._;if(b.keyCode===h&&(!e||b.metaKey||b.altKey)&&(!f||b.ctrlKey)&&(!g||b.shiftKey))return j.call(this,[a].concat(i))}]}}return this.$super.apply(this,b)}})})(RightJS)