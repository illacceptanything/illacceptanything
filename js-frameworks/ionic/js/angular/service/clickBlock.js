IonicModule
.factory('$ionicClickBlock', [
  '$document',
  '$ionicBody',
  '$timeout',
function($document, $ionicBody, $timeout) {
  var CSS_HIDE = 'click-block-hide';
  var cbEle, fallbackTimer, pendingShow;

  function preventClick(ev) {
    ev.preventDefault();
    ev.stopPropagation();
  }

  function addClickBlock() {
    if (pendingShow) {
      if (cbEle) {
        cbEle.classList.remove(CSS_HIDE);
      } else {
        cbEle = $document[0].createElement('div');
        cbEle.className = 'click-block';
        $ionicBody.append(cbEle);
        cbEle.addEventListener('touchstart', preventClick);
        cbEle.addEventListener('mousedown', preventClick);
      }
      pendingShow = false;
    }
  }

  function removeClickBlock() {
    cbEle && cbEle.classList.add(CSS_HIDE);
  }

  return {
    show: function(autoExpire) {
      pendingShow = true;
      $timeout.cancel(fallbackTimer);
      fallbackTimer = $timeout(this.hide, autoExpire || 310);
      addClickBlock();
    },
    hide: function() {
      pendingShow = false;
      $timeout.cancel(fallbackTimer);
      removeClickBlock();
    }
  };
}]);
