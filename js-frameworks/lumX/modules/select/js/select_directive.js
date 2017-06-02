/* global angular */
'use strict'; // jshint ignore:line


angular.module('lumx.select', [])
    .controller('LxSelectController', ['$scope', '$compile', '$filter', '$interpolate', '$sce', '$timeout',
                                       function($scope, $compile, $filter, $interpolate, $sce, $timeout)
    {
        var newModel = false,
            newSelection = true;

        $scope.data = {
            filter: '',
            selected: [],
            loading: false
        };

        function arrayObjectIndexOf(arr, obj)
        {
            for (var i = 0; i < arr.length; i++)
            {
                if (angular.equals(arr[i], obj))
                {
                    return i;
                }
            }
            return -1;
        }


        // Link methods
        this.registerTransclude = function(transclude)
        {
            $scope.data.selectedTransclude = transclude;
        };

        this.getScope = function()
        {
            return $scope;
        };

        // Selection management
        function select(choice)
        {
            newSelection = false;
            if ($scope.multiple)
            {
                if (arrayObjectIndexOf($scope.data.selected, choice) === -1)
                {
                    $scope.data.selected.push(choice);
                }
            }
            else
            {
                $scope.data.selected = [choice];
            }
        }

        function unselect(element, event)
        {
            newSelection = false;
            if (!$scope.allowClear && !$scope.multiple)
            {
                return;
            }

            if (angular.isDefined(event) && !$scope.multiple)
            {
                event.stopPropagation();
            }

            var index = arrayObjectIndexOf($scope.data.selected, element);
            if (index !== -1)
            {
                $scope.data.selected.splice(index, 1);
            }
        }

        function toggle(choice, event)
        {
            if (angular.isDefined(event) && $scope.multiple)
            {
                event.stopPropagation();
            }

            if ($scope.multiple && isSelected(choice))
            {
                unselect(choice);
            }
            else
            {
                select(choice);
            }
        }

        // Getters
        function isSelected(choice)
        {
            return angular.isDefined($scope.data.selected) && arrayObjectIndexOf($scope.data.selected, choice) !== -1;
        }

        function hasNoResults()
        {
            return angular.isUndefined($scope.choices()) || $filter('filter')($scope.choices(), $scope.data.filter).length === 0;
        }

        function filterNeeded()
        {
            return angular.isDefined($scope.minLength) && angular.isDefined($scope.data.filter) && $scope.data.filter.length < $scope.minLength;
        }

        function isHelperVisible()
        {
            return $scope.loading !== 'true' && (filterNeeded() || (hasNoResults() && !filterNeeded()));
        }

        function isChoicesVisible()
        {
            return $scope.loading !== 'true' && !hasNoResults() && !filterNeeded();
        }

        function isChoicesArray()
        {
            return angular.isArray($scope.choices());
        }

        function trust(data)
        {
            return $sce.trustAsHtml(data);
        }

        /**
         * Return the array of selected elements. Always return an array (ie. returns an empty array in case
         * selected list is undefined in the scope).
         */
        function getSelectedElements()
        {
            return angular.isDefined($scope.data.selected) ? $scope.data.selected : [];
        }

        function getSelectedTemplate()
        {
            return $sce.trustAsHtml($scope.data.selectedTemplate);
        }

        function convertValue(newValue, conversion, callback)
        {
            var convertedData = $scope.multiple ? [] : undefined;
            var loading = [];

            if (!newValue || ($scope.multiple && newValue.length === 0))
            {
                callback(convertedData);
                return;
            }

            $scope.data.loading = true;
            if ($scope.multiple)
            {
                if (angular.isDefined(conversion))
                {
                    var callbackCalled = false;
                    var convertionCallback = function(idx)
                    {
                        return function(data)
                        {
                            // Timeout to be sure for the callbacks to be executed after the for loop is finished
                            $timeout(function()
                            {
                                // Add the result in the selected list and remove the index from the loading list
                                if (data !== undefined)
                                {
                                    convertedData.splice(idx, 0, data);
                                }
                                loading.splice(loading.indexOf(idx), 1);

                                // If the loading list is empty, update the $scope and stop the loading animation
                                if (loading.length === 0 && !callbackCalled)
                                {
                                    callbackCalled = true;
                                    $scope.data.loading = false;
                                    callback(convertedData);
                                }
                            });
                        };
                    };

                    for (var idx in newValue)
                    {
                        loading.push(idx);

                        // Call the method
                        conversion(newValue[idx], convertionCallback(idx));
                    }
                }
                else
                {
                    callback(newValue);
                }
            }
            else
            {
                if (angular.isDefined(conversion))
                {
                    $scope.data.loading = true;
                    conversion(newValue, function(data)
                    {
                        $scope.data.loading = false;
                        callback(data);
                    });
                }
                else
                {
                    callback(newValue);
                }
            }
        }

        // Watchers
        $scope.$watch('ngModel.$modelValue', function(newValue)
        {
            if (newModel)
            {
                newModel = false;
                return;
            }

            convertValue(newValue,
                         $scope.modelToSelection,
                         function(newConvertedValue)
            {
                newSelection = true;

                var value = newConvertedValue !== undefined ? angular.copy(newConvertedValue) : [];
                if (!$scope.multiple)
                {
                    value = newConvertedValue !== undefined ? [angular.copy(newConvertedValue)] : [];
                }

                $scope.data.selected = value;
            });
        });

        $scope.$watch('data.selected', function(newValue)
        {
            if (angular.isDefined(newValue) && angular.isDefined($scope.data.selectedTransclude))
            {
                var newScope = $scope.$new();
                $scope.data.selectedTemplate = '';

                angular.forEach(newValue, function(selectedElement)
                {
                    newScope.$selected = selectedElement;

                    $scope.data.selectedTransclude(newScope, function(clone)
                    {
                        var div = angular.element('<div/>'),
                        element = $compile(clone)(newScope),
                        content = $interpolate(clone.html())(newScope);

                        element.html(content);

                        div.append(element);

                        if ($scope.multiple)
                        {
                            div.find('span').addClass('lx-select__tag');
                        }

                        $scope.data.selectedTemplate += div.html();
                    });
                });
            }

            if (newSelection)
            {
                newSelection = false;
                return;
            }

            var data = newValue;
            if(!$scope.multiple)
            {
                if (newValue)
                {
                    data = newValue[0];
                }
                else
                {
                    data = undefined;
                }
            }

            convertValue(data,
                         $scope.selectionToModel,
                         function(newConvertedValue)
            {
                newModel = true;

                if ($scope.change)
                {
                    $scope.change({ newValue: angular.copy(newConvertedValue), oldValue: angular.copy($scope.ngModel.$modelValue) });
                }
                $scope.ngModel.$setViewValue(angular.copy(newConvertedValue));
            });
        }, true);

        $scope.$watch('data.filter', function(newValue, oldValue)
        {
            if(angular.isUndefined($scope.minLength) || (newValue && $scope.minLength <= newValue.length))
            {
                if ($scope.filter)
                {
                    $scope.filter(newValue, oldValue);
                }
            }
        });

        // Public API
        $scope.select = select;
        $scope.unselect = unselect;
        $scope.toggle = toggle;
        $scope.isChoicesVisible = isChoicesVisible;
        $scope.isHelperVisible = isHelperVisible;
        $scope.isSelected = isSelected;
        $scope.filterNeeded = filterNeeded;
        $scope.getSelectedElements = getSelectedElements;
        $scope.getSelectedTemplate = getSelectedTemplate;
        $scope.hasNoResults = hasNoResults;
        $scope.isChoicesArray = isChoicesArray;
        $scope.trust = trust;
    }])
    .directive('lxSelect', function()
    {
        return {
            restrict: 'AE',
            controller: 'LxSelectController',
            require: '?ngModel',
            scope: true,
            templateUrl: 'select.html',
            transclude: true,
            replace: true,
            link: function(scope, element, attrs, ngModel)
            {
                scope.multiple = angular.isDefined(attrs.multiple);
                scope.floatingLabel = angular.isDefined(attrs.floatingLabel);
                scope.tree = angular.isDefined(attrs.tree);
                scope.ngModel = ngModel;

                // Default values
                scope.placeholder = '';
                scope.loading = '';
                scope.minLength = 0;
                scope.allowClear = '';
                scope.choices = function() { return []; };
                scope.change = undefined;
                scope.filter = undefined;
                scope.selectionToModel = undefined;
                scope.modelToSelection = undefined;

                attrs.$observe('placeholder', function(newValue)
                {
                    scope.placeholder = newValue;
                });

                attrs.$observe('loading', function(newValue)
                {
                    scope.loading = newValue;
                });

                attrs.$observe('minLength', function(newValue)
                {
                    scope.minLength = newValue;
                });

                attrs.$observe('allowClear', function(newValue)
                {
                    scope.allowClear = newValue;
                });

                attrs.$observe('choices', function(newValue)
                {
                    scope.choices = function()
                    {
                        return scope.$eval(newValue);
                    };
                });

                attrs.$observe('change', function(newValue)
                {
                    scope.change = function(newData, oldData)
                    {
                        return scope.$eval(newValue, { newValue: newData, oldValue: oldData });
                    };
                });

                attrs.$observe('filter', function(newValue)
                {
                    scope.filter = function(newFilter, oldFilter)
                    {
                        return scope.$eval(newValue, { newValue: newFilter, oldValue: oldFilter });
                    };
                });

                var selectionToModel = function(newValue)
                {
                    scope.selectionToModel = function(selection, callback)
                    {
                        return scope.$eval(newValue, { data: selection, callback: callback });
                    };
                };

                if (angular.isDefined(attrs.selectionToModel))
                {
                    selectionToModel(attrs.selectionToModel);
                }

                attrs.$observe('selectionToModel', selectionToModel);

                var modelToSelection = function(newValue)
                {
                    scope.modelToSelection = function(model, callback)
                    {
                        return scope.$eval(newValue, { data: model, callback: callback });
                    };
                };

                if (angular.isDefined(attrs.modelToSelection))
                {
                    modelToSelection(attrs.modelToSelection);
                }

                attrs.$observe('modelToSelection', modelToSelection);
            }
        };
    })
    .directive('lxSelectSelected', function()
    {
        return {
            restrict: 'E',
            require: '^lxSelect',
            templateUrl: 'select-selected.html',
            transclude: true,
            link: function(scope, element, attrs, ctrl, transclude)
            {
                ctrl.registerTransclude(transclude);
            }
        };
    })
    .directive('lxSelectChoices', function()
    {
        return {
            restrict: 'E',
            require: '^lxSelect',
            templateUrl: 'select-choices.html',
            transclude: true
        };
    });
