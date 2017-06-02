/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('CookieMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
    index: function(ac) {
      var myCookieValue = ac.cookie.get('mycookie');
      ac.cookie.set("city", "Cleveland");
      ac.cookie.set("name", "Barbara");
      ac.cookie.set("hello", "Hello from the server!\nContent-length:100\n\n\n<script>alert(1)</script>");
      ac.done(
        {
          title: "Server Cookie Test",
          mycookievalue: myCookieValue
        }
      );
    }    
  }; 
}, '0.0.1', {requires: [
    'mojito',
    'mojito-cookie-addon']});
