/* global angular */
'use strict'; // jshint ignore:line


angular.module('lumx.ripple', [])
    .directive('lxRipple', ['$timeout', function($timeout)
    {
        return {
            restrict: 'A',
            link: function(scope, element, attrs)
            {
                var timeout;

                element
                    .css({
                        position: 'relative',
                        overflow: 'hidden'
                    })
                    .bind('mousedown', function(e)
                    {
                        var ripple;

                        if (element.find('.ripple').length === 0)
                        {
                            ripple = angular.element('<span/>', {
                                class: 'ripple'
                            });

                            if (attrs.lxRipple)
                            {
                                ripple.addClass('bgc-' + attrs.lxRipple);
                            }

                            element.prepend(ripple);
                        }
                        else
                        {
                            ripple = element.find('.ripple');
                        }

                        ripple.removeClass('ripple--is-animated');

                        if (!ripple.height() && !ripple.width())
                        {
                            var diameter = Math.max(element.outerWidth(), element.outerHeight());

                            ripple.css({ height: diameter, width: diameter });
                        }

                        var x = e.pageX - element.offset().left - ripple.width() / 2;
                        var y = e.pageY - element.offset().top - ripple.height() / 2;

                        ripple.css({ top: y+'px', left: x+'px' }).addClass('ripple--is-animated');

                        timeout = $timeout(function()
                        {
                            ripple.removeClass('ripple--is-animated');
                        }, 651);
                    });

                scope.$on('$destroy', function()
                {
                    $timeout.cancel(timeout);
                });
            }
        };
    }]);
