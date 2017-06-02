/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('mymojit', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        default_ve: function (ac) {
            ac.done({
                "title": "Handlebars at work!",
                "view_engines": [
                    {"name": "EJS"},
                    {"name": "Jade"},
                    {"name": "Dust"},
                    {"name": "underscore" }
                ],
                "ul": { "title": 'Here are some of the other available rendering engines:' }
            });
        },
        added_ve: function (ac) {
            ac.done({
                "title": "EJS at work!",
                "view_engines": [ "Jade", "Dust", "underscore" ],
                "ul": { "title": 'In addition to Handlebars and EJS, you can also use these rendering engines:' }
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mymojit-model-foo']});
