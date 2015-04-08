
/*

Physical Device Testing Scenarios
---------------------------------
- focusing inputs below the keyboard should scroll them into the middle of the view
- focusing inputs that are above the keyboard should not scroll, but still resize the scrollable content area
- focusing inputs should resize the scroll view so the user can scroll to inputs at the bottom of the page
- clicking the label of an input should focus that input
- focusing an input that is mostly offscreen should scroll into view using js scrolling, not the browser scrolling it into view
- focusing an input while another input already has focus should not (visibly) close and re-open the keyboard
- focusing an input that is above the keyboard while another input already has focus should not do anything
- focusing an input that is below the keyboard while another input already has focus should scroll it into view
- the header should not move when an input is focused
- entering an input on a popup or modal should resize and un-resize that scrollview
- opening a popup or a modal while the keyboard is up should un-resize the scrollview before opening the modal or popup
- changing the orientation of the device should not break any of the above^
- quickly tap different text inputs then end up tapping an element that isn't a text input, the scroll resize should go away

- focusing inputs at the bottom of the page should scroll into view normally (broken on iOS 7.0 w/o height meta tag)
- on iOS in safari, shrinking the view should account for the button-bar at the bottom (currently not working)

Tentative:
- height=device-height not needed on iOS 6.1
- height=device-height needed on iOS 7.0 Cordova
  ** without it, fires 4 resize events when the keyboard comes up, and the scroll view resizes incorrectly, with it, does not fire resize events? **
- height=device-height not needed on iOS 7.1


Tested On
-----------------------
- iOS 7.1 Safari
- iOS 7.1 Cordova
- iOS 7.0 Safari
- iOS 7.0 Cordova
- iOS 6.1 Safari
- iOS 6.1 Cordova
- Android 4.4 Browser
- Android 4.4 Cordova
- Android 4.2 Browser
- Android 4.2 Cordova


iOS 7.1 Cordova with AND without viewport height DOES resize, DOES NOT fire resize event
iOS 7.1 Safari with AND without viewport height DOES NOT resize

iOS 7.0 Cordova with viewport height DOES resize, DOES fire resize event
iOS 7.0 Cordova without viewport height DOES resize, DOES NOT fire resize event
iOS 7.0 Safari with AND without viewport height DOES NOT resize

iOS 6.1 Cordova with AND without viewport height DOES NOT resize
iOS 6.1 Safari without viewport height DOES NOT resize

NOTES:
 -iOS 7.1 Safari with viewport height screws up ionic layout
 -iOS 7.0 Safari with viewport height, the scroll view does not resize properly on keyboardhide
 -iOS 7.0 Cordova without viewport height, scroll view does not resize properly switching inputs at bottom of page
 -iOS 6.1 Cordova and Safari don't work well with viewport height

RECOMMENDATIONS:
 -iOS 7.1 Cordova no viewport height, keyboard is not over webview
 -iOS 7.1 Safari no viewport height, keyboard is over webview

 -iOS 7.0 Cordova yes viewport height, keyboard is not over webview
 -iOS 7.0 Safari no viewport height, keyboard is over webview

 -iOS 6.1 Cordova no viewport height, keyboard is over webview
 -iOS 6.1 Safari no viewport height, keyboard is over webview

Notes:
---------------------------------
iOS 7 keyboard is 216px tall without the accessory bar
iOS 7 keyboard is 260px tall with the accessory bar

Switching inputs fires focusOut on iOS, doesn't on Android

*/


describe('Ionic Keyboard', function() {
  var window;

  beforeEach(inject(function($window) {
    window = $window;
    window._setTimeout = window.setTimeout;
    window.setTimeout = function(){};
    _activeElement = null; // the element which has focus
    window.cordova = undefined;
    window.device = undefined;
    ionic.Platform.ua = '';
    ionic.Platform.platforms = null;
    ionic.Platform.setPlatform('');
    ionic.Platform.setVersion('');
    ionic.keyboard.isOpen = false;
    ionic.keyboard.height = null;
    ionic.Platform.isFullScreen = false;
    ionic.keyboard.landscape = false;
  }));

  afterEach(function(){
    window.setTimeout = window._setTimeout;
  });

  it('Should keyboardShow', function(){
    var element = document.createElement('textarea');
    var elementTop = 100;
    var elementBottom = 200;
    var keyboardHeight = 200;
    var deviceHeight = 500;
    var details = keyboardShow(element, elementTop, elementBottom, deviceHeight, keyboardHeight);

    expect( details.keyboardHeight ).toEqual(200);
  });

  it('Should keyboardHasPlugin', function() {
    expect( keyboardHasPlugin() ).toEqual(false);

    window.cordova = {};
    expect( keyboardHasPlugin() ).toEqual(false);

    window.cordova.plugins = {};
    expect( keyboardHasPlugin() ).toEqual(false);

    window.cordova.plugins.Keyboard = {};
    expect( keyboardHasPlugin() ).toEqual(true);
  });

  it('keyboardGetHeight() should use the keyboard plugin if it is available', function(){
    ionic.keyboard.height = 216;
    expect( keyboardGetHeight() ).toEqual(216);
  });

  it('keyboardGetHeight() should = 275 if Cordova Android and is fullscreen', function(){
    ionic.Platform.setPlatform('android');
    window.cordova = {};
    ionic.Platform.isFullScreen = true;

    expect( keyboardGetHeight() ).toEqual(275);
  });

  it('keyboardGetHeight() should = (keyboardViewportHeight - window.innerHeight) if Android and not fullscreen', function(){
    ionic.Platform.setPlatform('android');
    expect( ionic.Platform.isFullScreen ).toEqual(false);

    keyboardViewportHeight = 480;
    window.innerHeight = 280;

    expect( keyboardGetHeight() ).toEqual(200);
  });

  it('keyboardGetHeight() should = 0 if keyboardViewportHeight = window.innerHeight and Android and not fullscreen', function(){
    ionic.Platform.setPlatform('android');
    expect( ionic.Platform.isFullScreen ).toEqual(false);

    keyboardViewportHeight = 480;
    window.innerHeight = 480;

    expect( keyboardGetHeight() ).toEqual(0);
  });

  it('keyboardGetHeight() should = 206 if iOS and in landscape orientation', function(){
    ionic.Platform.setPlatform('iOS');
    ionic.keyboard.landscape = true;

    expect( keyboardGetHeight() ).toEqual(206);
  });

  it('keyboardGetHeight() should = 216 if iOS Safari', function(){
    ionic.Platform.setPlatform('iOS');

    expect( ionic.Platform.isWebView() ).toEqual(false);
    expect( keyboardGetHeight() ).toEqual(216);
  });

  it('keyboardGetHeight() should = 260 if iOS Cordova', function(){
    ionic.Platform.setPlatform('iOS');
    window.cordova = {};

    expect( ionic.Platform.isWebView() ).toEqual(true);
    expect( keyboardGetHeight() ).toEqual(260);
  });

  it('keyboardGetHeight() should = 275 if not Android or iOS', function(){
    ionic.Platform.setPlatform('WP8');

    expect( keyboardGetHeight() ).toEqual(275);
  });

  it('keyboardUpdateViewportHeight() should update when window.innerHeight > keyboardViewportHeight', function(){
    window.innerHeight = 460;
    keyboardViewportHeight = 320;
    keyboardUpdateViewportHeight();

    expect( keyboardViewportHeight ).toEqual(460);
  });

  it('keyboardUpdateViewportHeight() should not update when window.innerHeight < keyboardViewportHeight', function(){
    window.innerHeight = 100;
    keyboardViewportHeight = 320;
    keyboardUpdateViewportHeight();

    expect( keyboardViewportHeight ).toEqual(320);
  });

  it('Should scroll input into view if it is under the keyboard', function(){
    var element = document.createElement('textarea');
    var elementTop = 300;
    var elementBottom = 400;
    var keyboardHeight = 200;
    var deviceHeight = 260;
    var details = keyboardShow(element, elementTop, elementBottom, deviceHeight, keyboardHeight);

    expect( details.isElementUnderKeyboard ).toEqual(true);
  });

  it('Should not scroll input into view if it is not under the keyboard', function(){
    var element = document.createElement('textarea');
    var elementTop = 100;
    var elementBottom = 200;
    var keyboardHeight = 200;
    var deviceHeight = 500;
    window.innerHeight = 500;
    var details = keyboardShow(element, elementTop, elementBottom, deviceHeight, keyboardHeight);

    expect( details.isElementUnderKeyboard ).toEqual(false);
  });

});
