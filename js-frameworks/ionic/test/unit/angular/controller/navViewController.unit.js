describe('$ionicNavView controller', function() {
  beforeEach(module('ionic'));

  it('should associate nav-bar with sibling nav-view', inject(function($rootScope, $compile) {
    var containerEle = angular.element('<div>');
    var navBarEle = angular.element('<ion-nav-bar>');
    var navViewEle = angular.element('<ion-nav-view>');

    containerEle.append(navBarEle);
    containerEle.append(navViewEle);

    $compile(containerEle)($rootScope);

    var navViewCtrl = navViewEle.data('$ionNavViewController');
    var navBarCtrl = containerEle.data('$ionNavBarController');

    navViewCtrl.update({});

    expect(navViewCtrl.isPrimary()).toBe(true);

    navViewCtrl.beforeEnter({
      title: 'My title'
    });

    expect(navBarCtrl.title()).toBe('My title');
  }));


  it('should associate nav-bar with a sibling element that has a child nav-view', inject(function($rootScope, $compile) {
    var containerEle = angular.element('<div>');
    var navBarEle = angular.element('<ion-nav-bar>');
    var tabsEle = angular.element('<tabs>');
    var tabEle = angular.element('<tab>');
    var navViewEle = angular.element('<ion-nav-view>');

    containerEle.append(navBarEle);
    containerEle.append(tabsEle);
    tabsEle.append(tabEle);
    tabEle.append(navViewEle);

    $compile(containerEle)($rootScope);

    var navViewCtrl = navViewEle.data('$ionNavViewController');
    var navBarCtrl = containerEle.data('$ionNavBarController');

    navViewCtrl.update({});
    expect(navViewCtrl.isPrimary()).toBe(true);

    navViewCtrl.beforeEnter({
      title: 'My title'
    });

    expect(navBarCtrl.title()).toBe('My title');
  }));


  it('should associate nav-bar with a sibling element that has a child nav-view', inject(function($rootScope, $compile) {
    var containerEle = angular.element('<div>');
    var navBarEle = angular.element('<ion-nav-bar>');
    var tabsEle = angular.element('<tabs>');
    var tabEle = angular.element('<tab>');
    var navViewEle = angular.element('<ion-nav-view>');

    containerEle.append(navBarEle);
    containerEle.append(tabsEle);
    tabsEle.append(tabEle);
    tabEle.append(navViewEle);

    $compile(containerEle)($rootScope);

    var navViewCtrl = navViewEle.data('$ionNavViewController');
    var navBarCtrl = containerEle.data('$ionNavBarController');

    navViewCtrl.update({});
    expect(navViewCtrl.isPrimary()).toBe(true);

    navViewCtrl.beforeEnter({
      title: 'My title'
    });

    expect(navBarCtrl.title()).toBe('My title');
  }));


});
