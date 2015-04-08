/* global angular */
'use strict'; // jshint ignore:line


angular.module('lumx.search-filter', [])
    .directive('lxSearchFilter', ['$timeout', function($timeout)
    {
        return {
            restrict: 'E',
            templateUrl: 'search-filter.html',
            scope: {
                model: '=?',
                theme: '@',
                placeholder: '@'
            },
            link: function(scope, element, attrs)
            {
                var $input = element.find('.search-filter__input'),
                    $label = element.find('.search-filter__label'),
                    $searchFilter = element.find('.search-filter'),
                    $searchFilterContainer = element.find('.search-filter__container');

                scope.closed = angular.isDefined(attrs.closed);

                if (angular.isUndefined(scope.theme))
                {
                    scope.theme = 'light';
                }

                attrs.$observe('filterWidth', function(filterWidth)
                {
                    $searchFilterContainer.css({ width: filterWidth });
                });

                // Events
                $input
                    .on('blur', function()
                    {
                        if (angular.isDefined(attrs.closed) && !$input.val())
                        {
                            $searchFilter.velocity({ 
                                width: 40
                            }, {
                                duration: 400,
                                easing: 'easeOutQuint',
                                queue: false
                            });
                        }
                    });

                $label.on('click', function()
                {
                    if (angular.isDefined(attrs.closed))
                    {
                        $searchFilter.velocity({ 
                            width: attrs.filterWidth ? attrs.filterWidth: 240
                        }, {
                            duration: 400,
                            easing: 'easeOutQuint',
                            queue: false
                        });

                        $timeout(function()
                        {
                            $input.focus();
                        }, 401);
                    }
                    else
                    {
                        $input.focus();
                    }
                });

                scope.clear = function()
                {
                    scope.model = undefined;

                    $input.focus();
                };
            }
        };
    }]);
