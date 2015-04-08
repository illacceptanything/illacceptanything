/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('device', function (Y, NAME) {
    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {
            var device = ac.context.device, css = '/static/device/assets/simple';
            if (device === 'iphone') {
                // Attach viewport meta-data
                ac.assets.addBlob('<meta name = "viewport" content = "width = device-width">', 'top');
                ac.assets.addBlob('<meta name = "viewport" content = "initial-scale = 1.0">', 'top');
                // Modify the style sheet name.
                css += '.' + device;
            }
            // Attach the style sheet.
            css += '.css';
            ac.assets.addCss(css, 'top');
            // Push data to the template. 
            ac.done({
                title: "Device Assets",
                colors: [
                    {id: "green", rgb: "#616536"},
                    {id: "brown", rgb: "#593E1A"},
                    {id: "grey",  rgb: "#777B88"},
                    {id: "blue",  rgb: "#3D72A4"},
                    {id: "red",   rgb: "#990033"}
                ]
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon']});
