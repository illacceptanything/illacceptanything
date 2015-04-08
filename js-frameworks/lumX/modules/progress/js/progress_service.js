/* global angular */
/* global document */
'use strict'; // jshint ignore:line


angular.module('lumx.progress', [])
    .service('LxProgressService', ['$timeout', '$interval', function($timeout, $interval)
    {
        var progressCircularIsShown = false,
            progressCircular,
            progressCircularSvg,
            progressCircularPath,
            progressLinearIsShown = false,
            progressLinear,
            progressLinearBackground,
            progressLinearFirstBar,
            progressLinearSecondBar;

        function init()
        {
            // Circular
            progressCircular = document.createElement('div');
            progressCircular.setAttribute('class', 'progress-circular');

            progressCircularSvg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
            progressCircularSvg.setAttribute('class', 'progress-circular__svg');

            progressCircularPath = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
            progressCircularPath.setAttribute('class', 'progress-circular__path');
            progressCircularPath.setAttribute('cx', '50');
            progressCircularPath.setAttribute('cy', '50');
            progressCircularPath.setAttribute('r', '20');
            progressCircularPath.setAttribute('fill', 'none');
            progressCircularPath.setAttribute('stroke-miterlimit', '10');

            progressCircularSvg.appendChild(progressCircularPath);
            progressCircular.appendChild(progressCircularSvg);

            // Linear
            progressLinear = angular.element('<div/>', { 'class': 'progress-linear' });
            progressLinearBackground = angular.element('<div/>', { 'class': 'progress-linear__background' });
            progressLinearFirstBar = angular.element('<div/>', { 'class': 'progress-linear__bar progress-linear__bar--first' });
            progressLinearSecondBar = angular.element('<div/>', { 'class': 'progress-linear__bar progress-linear__bar--second' });

            progressLinear
                .append(progressLinearBackground)
                .append(progressLinearFirstBar)
                .append(progressLinearSecondBar);
        }

        function showCircular(color, container)
        {
            if (!progressCircularIsShown)
            {
                showCircularProgress(color, container);
            }
        }

        function hideCircular()
        {
            if (progressCircularIsShown)
            {
                hideCircularProgress();
            }
        }

        function showCircularProgress(color, container)
        {
            progressCircularIsShown = true;

            progressCircularPath.setAttribute('stroke', color);

            if (angular.isDefined(container))
            {
                document.querySelector(container).appendChild(progressCircular);
            }
            else
            {
                document.getElementsByTagName('body')[0].appendChild(progressCircular);
            }

            $timeout(function()
            {
                progressCircular.setAttribute('class', 'progress-circular progress-circular--is-shown');
            });
        }

        function hideCircularProgress()
        {
            progressCircular.setAttribute('class', 'progress-circular');

            $timeout(function()
            {
                progressCircular.remove();

                progressCircularIsShown = false;
            }, 400);
        }

        function showLinear(color, container)
        {
            if (!progressLinearIsShown)
            {
                showLinearProgress(color, container);
            }
        }

        function hideLinear()
        {
            if (progressLinearIsShown)
            {
                hideLinearProgress();
            }
        }

        function showLinearProgress(color, container)
        {
            progressLinearIsShown = true;

            progressLinearBackground.css({ backgroundColor: color });
            progressLinearFirstBar.css({ backgroundColor: color });
            progressLinearSecondBar.css({ backgroundColor: color });

            if (angular.isDefined(container))
            {
                progressLinear.appendTo(container);
            }
            else
            {
                progressLinear.appendTo('body');
            }

            $timeout(function()
            {
                progressLinear.addClass('progress-linear--is-shown');
            });
        }

        function hideLinearProgress()
        {
            progressLinear.removeClass('progress-linear--is-shown');

            $timeout(function()
            {
                progressLinear.remove();

                progressLinearIsShown = false;
            }, 400);
        }

        init();

        return {
            circular: {
                show: showCircular,
                hide: hideCircular
            },
            linear: {
                show: showLinear,
                hide: hideLinear
            }
        };
    }])
    .directive('lxProgress', function()
    {
        return {
            restrict: 'E',
            scope: {
                type: '@',
                color: '@'
            },
            templateUrl: 'progress.html'
        };
    });
