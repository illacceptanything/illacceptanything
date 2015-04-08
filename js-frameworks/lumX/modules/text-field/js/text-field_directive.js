/* global angular */
'use strict'; // jshint ignore:line


angular.module('lumx.text-field', [])
    .filter('unsafe', ['$sce', function($sce)
    {
        return $sce.trustAsHtml;
    }])
    .directive('lxTextField', ['$timeout', function($timeout)
    {
        return {
            restrict: 'E',
            scope: {
                label: '@',
                disabled: '&',
                error: '&',
                valid: '&',
                fixedLabel: '&',
                icon: '@',
                theme: '@'
            },
            templateUrl: 'text-field.html',
            replace: true,
            transclude: true,
            link: function(scope, element, attrs, ctrl, transclude)
            {
                if (angular.isUndefined(scope.theme))
                {
                    scope.theme = 'light';
                }

                var modelController,
                    $field;

                scope.data = {
                    focused: false,
                    model: undefined
                };

                function focusUpdate()
                {
                    scope.data.focused = true;
                    scope.$apply();
                }

                function blurUpdate()
                {
                    scope.data.focused = false;
                    scope.$apply();
                }

                function modelUpdate()
                {
                    scope.data.model = modelController.$modelValue || $field.val();
                }

                function valueUpdate()
                {
                    modelUpdate();
                    scope.$apply();
                }

                function updateTextareaHeight()
                {
                    $timeout(function()
                    {
                        $field
                            .removeAttr('style')
                            .css({ height: $field[0].scrollHeight + 'px' });
                    });
                }

                transclude(function()
                {
                    $field = element.find('textarea');

                    if ($field[0])
                    {
                        updateTextareaHeight();

                        $field.on('cut paste drop keydown', function()
                        {
                            updateTextareaHeight();
                        });
                    }
                    else
                    {
                        $field = element.find('input');
                    }

                    $field.addClass('text-field__input');
                    $field.on('focus', focusUpdate);
                    $field.on('blur', blurUpdate);
                    $field.on('propertychange change click keyup input paste', valueUpdate);

                    modelController = $field.data('$ngModelController');

                    scope.$watch(function()
                    {
                        return modelController.$modelValue;
                    }, modelUpdate);
                });
            }
        };
    }]);
