/**
 * @ngdoc service
 * @name $ionicModal
 * @module ionic
 * @description
 *
 * Related: {@link ionic.controller:ionicModal ionicModal controller}.
 *
 * The Modal is a content pane that can go over the user's main view
 * temporarily.  Usually used for making a choice or editing an item.
 *
 * Put the content of the modal inside of an `<ion-modal-view>` element.
 *
 * **Notes:**
 * - A modal will broadcast 'modal.shown', 'modal.hidden', and 'modal.removed' events from its originating
 * scope, passing in itself as an event argument. Both the modal.removed and modal.hidden events are
 * called when the modal is removed.
 *
 * - This example assumes your modal is in your main index file or another template file. If it is in its own
 * template file, remove the script tags and call it by file name.
 *
 * @usage
 * ```html
 * <script id="my-modal.html" type="text/ng-template">
 *   <ion-modal-view>
 *     <ion-header-bar>
 *       <h1 class="title">My Modal title</h1>
 *     </ion-header-bar>
 *     <ion-content>
 *       Hello!
 *     </ion-content>
 *   </ion-modal-view>
 * </script>
 * ```
 * ```js
 * angular.module('testApp', ['ionic'])
 * .controller('MyController', function($scope, $ionicModal) {
 *   $ionicModal.fromTemplateUrl('my-modal.html', {
 *     scope: $scope,
 *     animation: 'slide-in-up'
 *   }).then(function(modal) {
 *     $scope.modal = modal;
 *   });
 *   $scope.openModal = function() {
 *     $scope.modal.show();
 *   };
 *   $scope.closeModal = function() {
 *     $scope.modal.hide();
 *   };
 *   //Cleanup the modal when we're done with it!
 *   $scope.$on('$destroy', function() {
 *     $scope.modal.remove();
 *   });
 *   // Execute action on hide modal
 *   $scope.$on('modal.hidden', function() {
 *     // Execute action
 *   });
 *   // Execute action on remove modal
 *   $scope.$on('modal.removed', function() {
 *     // Execute action
 *   });
 * });
 * ```
 */
IonicModule
.factory('$ionicModal', [
  '$rootScope',
  '$ionicBody',
  '$compile',
  '$timeout',
  '$ionicPlatform',
  '$ionicTemplateLoader',
  '$q',
  '$log',
function($rootScope, $ionicBody, $compile, $timeout, $ionicPlatform, $ionicTemplateLoader, $q, $log) {

  /**
   * @ngdoc controller
   * @name ionicModal
   * @module ionic
   * @description
   * Instantiated by the {@link ionic.service:$ionicModal} service.
   *
   * Be sure to call [remove()](#remove) when you are done with each modal
   * to clean it up and avoid memory leaks.
   *
   * Note: a modal will broadcast 'modal.shown', 'modal.hidden', and 'modal.removed' events from its originating
   * scope, passing in itself as an event argument. Note: both modal.removed and modal.hidden are
   * called when the modal is removed.
   */
  var ModalView = ionic.views.Modal.inherit({
    /**
     * @ngdoc method
     * @name ionicModal#initialize
     * @description Creates a new modal controller instance.
     * @param {object} options An options object with the following properties:
     *  - `{object=}` `scope` The scope to be a child of.
     *    Default: creates a child of $rootScope.
     *  - `{string=}` `animation` The animation to show & hide with.
     *    Default: 'slide-in-up'
     *  - `{boolean=}` `focusFirstInput` Whether to autofocus the first input of
     *    the modal when shown.  Default: false.
     *  - `{boolean=}` `backdropClickToClose` Whether to close the modal on clicking the backdrop.
     *    Default: true.
     *  - `{boolean=}` `hardwareBackButtonClose` Whether the modal can be closed using the hardware
     *    back button on Android and similar devices.  Default: true.
     */
    initialize: function(opts) {
      ionic.views.Modal.prototype.initialize.call(this, opts);
      this.animation = opts.animation || 'slide-in-up';
    },

    /**
     * @ngdoc method
     * @name ionicModal#show
     * @description Show this modal instance.
     * @returns {promise} A promise which is resolved when the modal is finished animating in.
     */
    show: function(target) {
      var self = this;

      if (self.scope.$$destroyed) {
        $log.error('Cannot call ' +  self.viewType + '.show() after remove(). Please create a new ' +  self.viewType + ' instance.');
        return;
      }

      var modalEl = jqLite(self.modalEl);

      self.el.classList.remove('hide');
      $timeout(function() {
        $ionicBody.addClass(self.viewType + '-open');
      }, 400);

      if (!self.el.parentElement) {
        modalEl.addClass(self.animation);
        $ionicBody.append(self.el);
      }

      if (target && self.positionView) {
        self.positionView(target, modalEl);
        // set up a listener for in case the window size changes
        ionic.on('resize',function() {
          ionic.off('resize',null,window);
          self.positionView(target,modalEl);
        },window);
      }

      modalEl.addClass('ng-enter active')
             .removeClass('ng-leave ng-leave-active');

      self._isShown = true;
      self._deregisterBackButton = $ionicPlatform.registerBackButtonAction(
        self.hardwareBackButtonClose ? angular.bind(self, self.hide) : noop,
        PLATFORM_BACK_BUTTON_PRIORITY_MODAL
      );

      ionic.views.Modal.prototype.show.call(self);

      $timeout(function() {
        modalEl.addClass('ng-enter-active');
        ionic.trigger('resize');
        self.scope.$parent && self.scope.$parent.$broadcast(self.viewType + '.shown', self);
        self.el.classList.add('active');
        self.scope.$broadcast('$ionicHeader.align');
      }, 20);

      return $timeout(function() {
        //After animating in, allow hide on backdrop click
        self.$el.on('click', function(e) {
          if (self.backdropClickToClose && e.target === self.el) {
            self.hide();
          }
        });
      }, 400);
    },

    /**
     * @ngdoc method
     * @name ionicModal#hide
     * @description Hide this modal instance.
     * @returns {promise} A promise which is resolved when the modal is finished animating out.
     */
    hide: function() {
      var self = this;
      var modalEl = jqLite(self.modalEl);

      self.el.classList.remove('active');
      modalEl.addClass('ng-leave');

      $timeout(function() {
        modalEl.addClass('ng-leave-active')
               .removeClass('ng-enter ng-enter-active active');
      }, 20);

      self.$el.off('click');
      self._isShown = false;
      self.scope.$parent && self.scope.$parent.$broadcast(self.viewType + '.hidden', self);
      self._deregisterBackButton && self._deregisterBackButton();

      ionic.views.Modal.prototype.hide.call(self);

      // clean up event listeners
      if (self.positionView) {
        ionic.off('resize',null,window);
      }

      return $timeout(function() {
        $ionicBody.removeClass(self.viewType + '-open');
        self.el.classList.add('hide');
      }, self.hideDelay || 320);
    },

    /**
     * @ngdoc method
     * @name ionicModal#remove
     * @description Remove this modal instance from the DOM and clean up.
     * @returns {promise} A promise which is resolved when the modal is finished animating out.
     */
    remove: function() {
      var self = this;
      self.scope.$parent && self.scope.$parent.$broadcast(self.viewType + '.removed', self);

      return self.hide().then(function() {
        self.scope.$destroy();
        self.$el.remove();
      });
    },

    /**
     * @ngdoc method
     * @name ionicModal#isShown
     * @returns boolean Whether this modal is currently shown.
     */
    isShown: function() {
      return !!this._isShown;
    }
  });

  var createModal = function(templateString, options) {
    // Create a new scope for the modal
    var scope = options.scope && options.scope.$new() || $rootScope.$new(true);

    options.viewType = options.viewType || 'modal';

    extend(scope, {
      $hasHeader: false,
      $hasSubheader: false,
      $hasFooter: false,
      $hasSubfooter: false,
      $hasTabs: false,
      $hasTabsTop: false
    });

    // Compile the template
    var element = $compile('<ion-' + options.viewType + '>' + templateString + '</ion-' + options.viewType + '>')(scope);

    options.$el = element;
    options.el = element[0];
    options.modalEl = options.el.querySelector('.' + options.viewType);
    var modal = new ModalView(options);

    modal.scope = scope;

    // If this wasn't a defined scope, we can assign the viewType to the isolated scope
    // we created
    if (!options.scope) {
      scope[ options.viewType ] = modal;
    }

    return modal;
  };

  return {
    /**
     * @ngdoc method
     * @name $ionicModal#fromTemplate
     * @param {string} templateString The template string to use as the modal's
     * content.
     * @param {object} options Options to be passed {@link ionic.controller:ionicModal#initialize ionicModal#initialize} method.
     * @returns {object} An instance of an {@link ionic.controller:ionicModal}
     * controller.
     */
    fromTemplate: function(templateString, options) {
      var modal = createModal(templateString, options || {});
      return modal;
    },
    /**
     * @ngdoc method
     * @name $ionicModal#fromTemplateUrl
     * @param {string} templateUrl The url to load the template from.
     * @param {object} options Options to be passed {@link ionic.controller:ionicModal#initialize ionicModal#initialize} method.
     * options object.
     * @returns {promise} A promise that will be resolved with an instance of
     * an {@link ionic.controller:ionicModal} controller.
     */
    fromTemplateUrl: function(url, options, _) {
      var cb;
      //Deprecated: allow a callback as second parameter. Now we return a promise.
      if (angular.isFunction(options)) {
        cb = options;
        options = _;
      }
      return $ionicTemplateLoader.load(url).then(function(templateString) {
        var modal = createModal(templateString, options || {});
        cb && cb(modal);
        return modal;
      });
    }
  };
}]);
