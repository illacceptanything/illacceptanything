/* global angular */

angular.module('lumx.utils', [
    'lumx.utils.transclude',
    'lumx.utils.transclude-replace'
]);

angular.module('lumx', [
    'lumx.utils',
    'lumx.ripple',
    'lumx.notification',
    'lumx.dropdown',
    'lumx.text-field',
    'lumx.dialog',
    'lumx.select',
    'lumx.scrollbar',
    'lumx.thumbnail',
    'lumx.tabs',
    'lumx.tooltip',
    'lumx.file-input',
    'lumx.progress',
    'lumx.search-filter',
    'lumx.date-picker'
]);