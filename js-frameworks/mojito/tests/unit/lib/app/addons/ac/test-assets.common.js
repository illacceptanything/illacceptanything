//-*- x-counterpart: ../../../../../../source/lib/app/addons/ac/assets.common.js; -*-
/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/

YUI().use('mojito-assets-addon', 'test', 'array-extras', function(Y, NAME) {

    var suite = new Y.Test.Suite('mojito-assets-addon tests'),
        cases = {},
        A = Y.Assert,
        AA = Y.ArrayAssert,
        addon;

    cases = {
        name: 'basics',

        setUp: function() {
            var init = {
                    instance: {
                        type: 'foo',
                        config: {
                            assets: {}
                        }
                    }
                };

            addon = new Y.mojito.addons.ac.assets(init);
            YUI.namespace('_mojito._cache.compiled');
        },

        tearDown: function() {
            addon = null;
        },

        'test one top css with type and location': function() {
            var css, result;

            css = '<style>.foo { color:red; }</style>';
            addon.addAsset('css', 'top', css);
            result = addon.getAssets('top');

            A.isObject(result.top, 'missing top block');
            A.isObject(result.top.css, 'missing top.css block');
            A.isArray(result.top.css, 'top.css should be an array');
            A.areSame(1, result.top.css.length, 'bad top.css array length');
            A.areSame(css, result.top.css[0], 'bad css value');
        },

        'test inlined css is deduped': function() {
            var othertest = 'test one top css with type and location',
                css = '<style>.foo { color:red; }</style>';

            this[othertest]();

            A.areSame(1, addon.getAssets('top').top.css.length);
            addon.addAsset('css', 'top', css);
            A.areSame(1, addon.getAssets('top').top.css.length);
        },

//         'test inlined css get wrapped in style tags': function() {
//             var css = '.foo { color:red; }',
//                 expected = '<style>.foo { color:red; }</style>',
//                 tmp;
//
//             YUI.namespace('_mojito._cache.compiled.css.inline');
//             tmp = YUI._mojito._cache.compiled.css.inline[id]
//
//             addon.addAsset('css', 'top', css);
//             A.areSame(expected, addon.getAssets('top').top.css[0]);
//         },

        'test image preloading (for lcov, really)': function() {
            var imgs = [
                'http://yahoo.com',
                'http://yahoo.com'
            ];
            addon.preLoadImages(imgs);
        },

        'test one bottom css with type and location': function() {
            var css = '<style>.foo { color:red; }</style>';

            addon.addAsset('css', 'bottom', css);

            var res = addon.getAssets('bottom');

            A.isObject(res.bottom, 'missing bottom block');
            A.isUndefined(res.top, 'bottom.css should not exist');
            A.isObject(res.bottom.css, 'missing bottom.css block');
            A.isArray(res.bottom.css, 'bottom.css should be an array');
            A.areSame(1, res.bottom.css.length, 'bad bottom.css array length');
            A.areSame(css, res.bottom.css[0], 'bad css value');

        },

        'test two css with type and location': function() {
            var css1 = '<style>.foo { color:red; }</style>';
            var css2 = '<style>.bar { color:yellow; }</style>';

            addon.addAsset('css', 'top', css1);
            addon.addAsset('css', 'top', css2);

            var res = addon.getAssets('top');

            A.areSame(2, res.top.css.length, 'bad css.top array length');
            A.areSame(css1, res.top.css[0], 'bad css value');
            A.areSame(css2, res.top.css[1], 'bad css value');

        },

        'test two css with type and location, one top, one bottom': function() {
            var css1 = '<style>.foo { color:red; }</style>';
            var css2 = '<style>.bar { color:yellow; }</style>';

            addon.addAsset('css', 'top', css1);
            addon.addAsset('css', 'bottom', css2);

            var t = addon.getAssets('top');
            var b = addon.getAssets('bottom');

            A.areSame(1, t.top.css.length, 'bad css.top array length');
            A.areSame(1, t.top.css.length, 'bad css.bottom array length');
            A.areSame(css1, t.top.css[0], 'bad top css value');
            A.areSame(css2, b.bottom.css[0], 'bad bottom css value');

        },

        'test addCss() with location override': function() {
            var css = 'css';

            addon.addCss(css, 'bottom');

            var btm = addon.getAssets('bottom');

            A.areSame(1, btm.bottom.css.length, 'bad bottom array length');
            A.areSame(css, btm.bottom.css[0], 'bad css');
        },

        'test addCss() with no location provides default "top"': function() {
            var css = 'css';

            addon.addCss(css);

            var b = addon.getAssets('bottom');
            var t = addon.getAssets('top');

            A.isUndefined(b.bottom, 'bottom should not be defined');
            A.areSame(1, t.top.css.length, 'bad top array length');
            A.areSame(css, t.top.css[0], 'badd css');
        },

        'test addBlob() with location override': function() {

            var blob = '<div>&lt;span&gt;This is escaped markup&lt;/span&gt;</div>';

            addon.addBlob(blob, 'bottom');

            var btm = addon.getAssets('bottom');

            A.areSame(1, btm.bottom.blob.length, 'bad bottom array length');
            A.areSame(blob, btm.bottom.blob[0], 'bad blob');
        },

        'test addBlob() with no location provides default "bottom"': function() {

            var blob = '<div>&lt;span&gt;This is escaped markup&lt;/span&gt;</div>';

            addon.addBlob(blob)

            var b = addon.getAssets('bottom');
            var t = addon.getAssets('top');

            A.isUndefined(t.top, 'top should not be defined');
            A.areSame(1, b.bottom.blob.length, 'bad bottom array length');
            A.areSame(blob, b.bottom.blob[0], 'bad blob');
        },

        'test addJs() with location override': function() {
            var js = 'js';

            addon.addJs(js, 'top');

            var t = addon.getAssets('top');

            A.areSame(1, t.top.js.length, 'bad bottom array length');
            A.areSame(js, t.top.js[0], 'bad js');
        },

        'test addJs() with no location provides default "bottom"': function() {
            var js = 'js';

            addon.addJs(js);

            var b = addon.getAssets('bottom');
            var t = addon.getAssets('top');

            A.isUndefined(t.top, 'top should not be defined');
            A.areSame(1, b.bottom.js.length, 'bad bottom array length');
            A.areSame(js, b.bottom.js[0], 'bad js');
        },

        'test addCss() and addJs()': function() {
            var js1 = 'js1', js2 = 'js2',
                css1 = 'css1', css2 = 'css2';

            addon.addJs(js1);
            addon.addCss(css1);
            addon.addJs(js2);
            addon.addCss(css2);

            var t = addon.getAssets('top');
            var b = addon.getAssets('bottom');

            A.isUndefined(b.bottom.css, 'should not have bottom css');
            AA.itemsAreEqual([js1, js2], b.bottom.js, 'bad js values');
            A.isUndefined(t.top.js, 'should not have top js');
            AA.itemsAreEqual([css1, css2], t.top.css, 'bad css values');
        },

        'test assets mix properly': function() {

            addon.addJs('jsf1', 'top');
            addon.addJs('jsf2', 'top');

            addon.addCss('cssf1');      // css defaults to top
            addon.addJs('jsf3');        // js defaults to bottom

            var to = {
                    top: {
                        js: ['jst1','jst2'],
                        css: ['csst1','csst2','csst3']
                    },
                    bottom: {
                        js: ['jsb1']
                    }
                };

            var expected = {
                    top: {
                        js: ['jst1','jst2','jsf1','jsf2'],
                        css: ['csst1','csst2','csst3','cssf1']
                    },
                    bottom: {
                        js: ['jsb1', 'jsf3']
                    }
                };

            addon.mergeMetaInto({assets: to});

            AA.itemsAreEqual(expected.top.js, to.top.js);
            AA.itemsAreEqual(expected.top.css, to.top.css);
            AA.itemsAreEqual(expected.bottom.js, to.bottom.js);
            A.isUndefined(to.bottom.css, 'bad bottom.css');
        },

        'test assets unique properly': function() {

            // New values which should be added to the 'to' target.
            addon.addJs('jsf1', 'top');
            addon.addJs('jsf2', 'top');

            addon.addCss('cssf1');      // css defaults to top
            addon.addJs('jsf3');        // js defaults to bottom

            // Duplicates of values in 'to', should be eliminated, not added.
            addon.addJs('jsb1');
            addon.addCss('csst1');

            var to = {
                top: {
                    js: ['jst1','jst2'],
                    css: ['csst1','csst2','csst3']
                },
                bottom: {
                    js: ['jsb1']
                }
            };

            var expected = {
                top: {
                    js: ['jst1','jst2','jsf1','jsf2'],
                    css: ['csst1','csst2','csst3','cssf1']
                },
                bottom: {
                    js: ['jsb1', 'jsf3']
                }
            };

            addon.mergeMetaInto({assets: to});

            AA.itemsAreEqual(expected.top.js, to.top.js);
            AA.itemsAreEqual(expected.top.css, to.top.css);
            AA.itemsAreEqual(expected.bottom.js, to.bottom.js);
            A.isUndefined(to.bottom.css, 'bad bottom.css');
        },

        'addAsset() should return undefined if no content': function() {
            A.isUndefined(addon.addAsset()); // for coverage, func always void
        },

        'addAsset() should normalize relative uris from assetsRoot config': function() {
            var init = {
                    instance: {
                        assetsRoot: '/abc/def',
                        config: {}
                    }
                },
                addon = new Y.mojito.addons.ac.assets(init),
                expected = init.instance.assetsRoot + '/foo.js';

            addon.addAsset('js', 'top', './foo.js');

            A.areSame(expected, addon.getAssets().top.js[0]);
        },

        'addAsset() should dedupe': function() {
            var othertest = 'addAsset() should normalize relative uris from assetsRoot config';
            this[othertest]();

            addon.addAsset('js', 'top', './foo');// add again
            addon.addAsset('js', 'top', './foo');// add again

            A.areEqual(1, addon.getAssets().top.js.length);
        },

        'test asset initialization via constructor': function() {
            var init = {
                    instance: {
                        config: {}
                    }
                },
                addon,
                actual,
                expected;

            init.instance.config.assets = {
                top: {
                    js: ['jst1','jst2'],
                    css: ['csst1','csst2','csst3']
                },
                bottom: {
                    js: ['jsb1']
                }
            };

            expected = Y.merge(init.instance.config.assets); // copy

            addon = new Y.mojito.addons.ac.assets(init);
            actual = addon.getAssets();

            AA.itemsAreEqual(expected.top.js, actual.top.js);

            expected.foo = 9; // prove we made a copy
            A.areNotSame(expected.foo, actual.foo);

        },

        'test inline css': function() {
            YUI._mojito._cache.compiled.css = {
                'inline': {
                    'my-path': 'file-contents'
                }
            };
            addon.context = { runtime: 'client' };
            addon.mojitType = 'fruit';
            addon.addAsset('css', 'top', 'my-path');

            var results = addon.getAssets();
            A.isNotUndefined(results);
            var expected = { top: { blob: [ '<style type="text/css">\nfile-contents</style>\n' ] } };

            A.areEqual(Y.JSON.stringify(expected), Y.JSON.stringify(results));
        },

        'test renderLocations': function() {

            var fragments;

            // empty assets
            fragments = addon.renderLocations();

            A.isObject(fragments, 'should always be an object, even when empty');
            A.areEqual(0, Y.Object.keys(fragments).length, 'empty assets means nothing to render as fragment');

            // testing with some assets in place
            addon.addAsset('js', 'top', 'foo.js');
            addon.addAsset('js', 'top', 'bar.js');
            addon.addAsset('js', 'bottom', 'baz.js');
            addon.addAsset('css', 'top', 'foo.css');
            fragments = addon.renderLocations();

            A.areEqual(2, Y.Object.keys(fragments).length, 'bottom and top should be in place');
            A.isString(fragments.bottom, 'bottom fragment is missing');
            A.isString(fragments.top, 'top fragment is missing');

            // TODO: test the content of the fragments
            A.areEqual(2, fragments.top.match(/<script/g).length, 'two scripts tags should be generated at the top');
            A.areEqual(1, fragments.top.match(/<link/g).length, 'three link tags should be generated at the top');
            A.areEqual(1, fragments.bottom.match(/<script/g).length, 'bottom fragment should have one script tag');
            A.isNull(fragments.bottom.match(/<link/g), 'bottom fragment should have no link tag');
        }

    };

    suite.add(new Y.Test.Case(cases));
    Y.Test.Runner.add(suite);

});
