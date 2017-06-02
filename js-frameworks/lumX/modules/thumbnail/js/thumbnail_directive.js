/* global angular */
/* global Image */
'use strict'; // jshint ignore:line


angular.module('lumx.thumbnail', [])
    .controller('LxThumbnailController', ['$scope', function($scope)
        {
            this.init = function(element)
            {
                $scope.element = element;
            };

            this.prepareImage = function()
            {
                $scope.isLoading = true;

                var img = new Image();

                img.src = $scope.thumbnailSrc;

                $scope.element.css({
                    width: $scope.thumbnailWidth + 'px',
                    height: $scope.thumbnailHeight + 'px'
                });

                img.onload = function()
                {
                    $scope.originalWidth = img.width;
                    $scope.originalHeight = img.height;

                    addImage();

                    $scope.isLoading = false;
                };
            };

            function addImage()
            {
                var imageSizeWidthRatio = $scope.thumbnailWidth / $scope.originalWidth,
                    imageSizeWidth = $scope.thumbnailWidth,
                    imageSizeHeight = $scope.originalHeight * imageSizeWidthRatio;

                if (imageSizeHeight < $scope.thumbnailHeight)
                {
                    var resizeFactor = $scope.thumbnailHeight / imageSizeHeight;

                    imageSizeHeight = $scope.thumbnailHeight;
                    imageSizeWidth = resizeFactor * imageSizeWidth;
                }

                $scope.element.css({
                    'background': 'url(' + $scope.thumbnailSrc + ') no-repeat',
                    'background-position': 'center',
                    'background-size': imageSizeWidth + 'px ' + imageSizeHeight + 'px',
                    'overflow': 'hidden'
                });
            }
        }])
    .directive('lxThumbnail', function()
    {
        return {
            restrict: 'E',
            template: '<div class="thumbnail" ng-class="{ \'thumbnail--is-loading\': isLoading }"></div>',
            replace: true,
            controller: 'LxThumbnailController',
            scope: {
                thumbnailSrc: '@',
                thumbnailWidth: '@',
                thumbnailHeight: '@'
            },
            link: function(scope, element, attrs, ctrl)
            {
                ctrl.init(element);

                attrs.$observe('thumbnailSrc', function()
                {
                    if (attrs.thumbnailSrc)
                    {
                        ctrl.prepareImage();
                    }
                });

                attrs.$observe('thumbnailWidth', function()
                {
                    if (attrs.thumbnailWidth)
                    {
                        ctrl.prepareImage();
                    }
                });

                attrs.$observe('thumbnailHeight', function()
                {
                    if (attrs.thumbnailHeight)
                    {
                        ctrl.prepareImage();
                    }
                });
            }
        };
    });