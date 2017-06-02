/* global angular */
'use strict'; // jshint ignore:line


angular.module('lumx.file-input', [])
    .directive('lxFileInput', ['$timeout', function($timeout)
    {
        return {
            restrict: 'E',
            scope: {
                label: '@',
                value: '=',
                change: '&'
            },
            templateUrl: 'file-input.html',
            replace: true,
            link: function(scope, element)
            {
                var $input = element.find('input'),
                    $fileName = element.find('.input-file__filename');

                $input
                    .addClass('input-file__input')
                    .on('change', function()
                    {
                        $timeout(function()
                        {
                            setFileName($input.val());
                            element.addClass('input-file--is-focused');
                        });

                        // handle change function
                        if (angular.isDefined(scope.change))
                        {
                            // return the file element, the new value and the old value to the callback
                            scope.change({e: $input[0].files[0], newValue: $input.val(), oldValue: $fileName.text()});
                        }
                    })
                    .on('blur', function()
                    {
                        element.removeClass('input-file--is-focused');
                    });

                function setFileName(val)
                {
                    if (val)
                    {
                        $fileName.text(val.replace(/C:\\fakepath\\/i, ''));

                        element.addClass('input-file--is-active');
                    }
                }

                scope.$watch('value', function(value)
                {
                    setFileName(value);
                });
            }
        };
    }]);