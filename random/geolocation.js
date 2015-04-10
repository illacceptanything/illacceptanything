(function() {

  var init = function() {
    if (detectMobile) {
      //
      console.log();
      getHtmlLocation;
    } else {
      getFallBackLocation;
    }

  },

    detectMobile = function() {
      userAgent = (navigator.userAgent || navigator.vendor || window.opera);
      if (userAgent.match(/[android|kindle|lge|silk|iphone|ipad|ipod|blackberry|webos|meego|symbian|maemo||windows.phone]/gi)) {
        return true;
      }
    },

    getHtml5Location = function() {
      var error;

      if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(setPosition, setError);
      } else {
        error.code = 'NO_API';
        setError(error.code);
      }
    },

    getGoogleLocation = function() {
      var objInfo = JSON.stringify(google.loader.ClientLocation, null, '\t');
    },


    getSkyhookLocation = function() {
      //https://context.skyhookwireless.com/accelerator/ip?version=2.0&ip=148.141.31.6&prettyPrint=true&key=eJwz5DQ0AAFjE1NDzmo3A3MjQ1MDU10LCycDXRNjC0tdC2czS11LYwNDAwtTJxMXQ8daAAg7CsI&user=eval
    },

    getMaxMindLocation = function() {
      geoip2.city(onSuccess, onError, options);
    },

    getFallBackLocation = function() {
      getGoogleLocation;
      getSkyhookLocation;
      getMaxMindLocation;
    },

    setCoords = function(coords) {
      latitude = position.coords.latitude;
      longitude = position.coords.longitude;
      source = 'html5Api';
    },

    setError = function(error) {
      switch (error.code) {
        case NO_API:
          html5Error = 'noApi';
          break;
        case error.PERMISSION_DENIED:
          html5Error = 'permissionDenied';
          break;
        case error.POSITION_UNAVAILABLE:
          html5Error = 'positionUnavailable';
          break;
        case error.TIMEOUT:
          html5Error = 'timeout';
          break;
        case error.UNKNOWN_ERROR:
          html5Error = 'unknown';
          break;
        default:
          html5Error = 'meltdown';
      }
      getFallBackLocation;
    };


    init;

})();
