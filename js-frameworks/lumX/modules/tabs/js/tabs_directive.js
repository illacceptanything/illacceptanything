/* global angular */
'use strict'; // jshint ignore:line


angular.module('lumx.tabs', [])
    .controller('LxTabsController', ['$scope', '$sce', '$timeout', '$window', function($scope, $sce, $timeout, $window)
    {
        var tabs = [],
            links,
            indicator;

        $scope.activeTab = angular.isUndefined($scope.activeTab) ? 0 : $scope.activeTab;

        this.init = function(element)
        {
            links = element.find('.tabs__links');
            indicator = element.find('.tabs__indicator');
        };

        this.getScope = function()
        {
            return $scope;
        };

        this.addTab = function(tabScope)
        {
            tabs.push(tabScope);

            $timeout(function()
            {
                setIndicatorPosition();
            });

            return (tabs.length - 1);
        };

        this.removeTab = function(tabScope)
        {
            var idx = tabs.indexOf(tabScope);

            if (idx !== -1)
            {
                for (var tabIdx = idx + 1; tabIdx < tabs.length; ++tabIdx)
                {
                    --tabs[tabIdx].index;
                }

                tabs.splice(idx, 1);

                if (idx === $scope.activeTab)
                {
                    $scope.activeTab = 0;
                    $timeout(function()
                    {
                        setIndicatorPosition(idx);
                    });
                }
                else if(idx < $scope.activeTab)
                {
                    var old = $scope.activeTab;
                    $scope.activeTab = old - 1;

                    $timeout(function()
                    {
                        setIndicatorPosition(old);
                    });
                }
                else
                {
                    $timeout(function()
                    {
                        setIndicatorPosition();
                    });
                }
            }
        };

        function getTabs()
        {
            return tabs;
        }

        function setActiveTab(index)
        {
            $timeout(function()
            {
                $scope.activeTab = index;
            });
        }

        function setLinksColor(newTab)
        {
            links.find('.tabs-link').removeClass('tc-' + $scope.indicator);
            links.find('.tabs-link').eq(newTab).addClass('tc-' + $scope.indicator);
        }

        function setIndicatorPosition(oldTab)
        {
            var direction;

            if ($scope.activeTab > oldTab)
            {
                direction = 'right';
            }
            else
            {
                direction = 'left';
            }

            var tabsWidth = links.outerWidth(),
                activeTab = links.find('.tabs-link').eq($scope.activeTab),
                activeTabWidth = activeTab.outerWidth(),
                indicatorLeft = activeTab.position().left,
                indicatorRight = tabsWidth - (indicatorLeft + activeTabWidth);

            if (angular.isUndefined(oldTab))
            {
                indicator.css({
                    left: indicatorLeft,
                    right: indicatorRight
                });
            }
            else
            {
                var animationProperties = {
                    duration: 200,
                    easing: 'easeOutQuint'
                };

                if (direction === 'left')
                {
                    indicator.velocity({
                        left: indicatorLeft
                    }, animationProperties);

                    indicator.velocity({
                        right: indicatorRight
                    }, animationProperties);
                }
                else
                {
                    indicator.velocity({
                        right: indicatorRight
                    }, animationProperties);

                    indicator.velocity({
                        left: indicatorLeft
                    }, animationProperties);
                }
            }
        }

        $scope.$watch('activeTab', function(newIndex, oldIndex)
        {
            if (newIndex !== oldIndex)
            {
                $timeout(function()
                {
                    setLinksColor(newIndex);
                    setIndicatorPosition(oldIndex);
                });
            }
        });

        angular.element($window).bind('resize', function()
        {
            setIndicatorPosition();
        });

        // Public API
        $scope.getTabs = getTabs;
        $scope.setActiveTab = setActiveTab;
    }])
    .directive('lxTabs', function()
    {
        return {
            restrict: 'E',
            controller: 'LxTabsController',
            templateUrl: 'tabs.html',
            transclude: true,
            replace: true,
            scope: {
                activeTab: '=?',
                linksTc: '@',
                linksBgc: '@',
                indicator: '@',
                noDivider: '@',
                zDepth: '@',
                layout: '@'
            },
            link: function(scope, element, attrs, ctrl)
            {
                ctrl.init(element);

                if (angular.isUndefined(scope.linksTc))
                {
                    scope.linksTc = 'dark';
                }

                if (angular.isUndefined(scope.linksBgc))
                {
                    scope.linksBgc = 'white';
                }

                if (angular.isUndefined(scope.indicator))
                {
                    scope.indicator = 'blue-500';
                }

                if (angular.isUndefined(scope.zDepth))
                {
                    scope.zDepth = '0';
                }

                if (angular.isUndefined(scope.layout))
                {
                    scope.layout = 'full';
                }
            }
        };
    })
    .directive('lxTab', function()
    {
        return {
            require: '^lxTabs',
            restrict: 'E',
            scope: {
                heading: '@',
                icon: '@'
            },
            templateUrl: 'tab.html',
            transclude: true,
            replace: true,
            link: function(scope, element, attrs, ctrl)
            {
                scope.data = ctrl.getScope();
                scope.index = ctrl.addTab(scope);

                scope.$on('$destroy', function(scope)
                {
                    ctrl.removeTab(scope.currentScope);
                });
            }
        };
    })
    .directive('lxTabLink', function()
    {
        return {
            require: '^lxTabs',
            restrict: 'A',
            link: function(scope, element)
            {
                if (scope.activeTab === element.parent().index())
                {
                    element.addClass('tc-' + scope.indicator);
                }

                element
                    .on('mouseenter', function()
                    {
                        if (scope.activeTab !== element.parent().index())
                        {
                            element.addClass('tc-' + scope.indicator);
                        }
                    })
                    .on('mouseleave', function()
                    {
                        if (scope.activeTab !== element.parent().index())
                        {
                            element.removeClass('tc-' + scope.indicator);
                        }
                    });
            }
        };
    });
