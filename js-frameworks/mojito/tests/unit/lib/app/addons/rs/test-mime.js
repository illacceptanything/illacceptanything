/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use(
    'base',
    'oop',
    'addon-rs-mime',
    'test',
    function(Y) {
    
    var suite = new YUITest.TestSuite('mojito-addon-rs-mime-tests'),
        libpath = require('path'),
        mojitoRoot = libpath.join(__dirname, '../../../../../../lib'),
        A = Y.Assert;


    function MockRS(config) {
        MockRS.superclass.constructor.apply(this, arguments);
    }
    MockRS.NAME = 'MockResourceStore';
    MockRS.ATTRS = {};
    Y.extend(MockRS, Y.Base, {
        initializer: function(cfg) {
            this.RVs = {};
        },

        addResourceVersion: function(res) {
            this.RVs[[res.affinity, res.selector, res.id].join('/')] = res;
        }
    });


    function cmp(x, y, msg, path) {
        if (Y.Lang.isArray(x)) {
            A.isArray(x, msg || 'first arg should be an array');
            A.isArray(y, msg || 'second arg should be an array');
            A.areSame(x.length, y.length, msg || 'arrays are different lengths');
            for (var i = 0; i < x.length; i += 1) {
                cmp(x[i], y[i], msg);
            }
            return;
        }
        if (Y.Lang.isObject(x)) {
            A.isObject(x, msg || 'first arg should be an object');
            A.isObject(y, msg || 'second arg should be an object');
            A.areSame(Object.keys(x).length, Object.keys(y).length, msg || 'object keys are different lengths');
            for (var i in x) {
                if (x.hasOwnProperty(i)) {
                    cmp(x[i], y[i], msg);
                }
            }
            return;
        }
        A.areSame(x, y, msg || 'args should be the same');
    }


    function makeSource(dir, dirType, subdir, file, isFile) {
        var source = {
            fs: {
                fullPath: libpath.join(dir, subdir, file),
                rootDir: dir,
                rootType: dirType,
                subDir: subdir,
                subDirArray: subdir.split('/'),
                isFile: isFile,
                ext: libpath.extname(file)
            },
            pkg: {
                name: 'unittest',
                version: '999.666.999',
                depth: 999
            }
        };
        source.fs.basename = libpath.basename(file, source.fs.ext);
        return source;
    }


    suite.add(new YUITest.TestCase({
        
        name: 'mime rs addon tests',
        

        'parse mime types of resources': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/conventions');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.mime, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var source = makeSource(fixtures+'/mojits/X', 'mojit', '.', 'controller.common.js', true);
            var res = {
                source: source,
                mojit: 'X',
                type: 'controller',
                name: 'controller',
                id: 'controller--controller',
                affinity: 'common',
                selector: '*'
            };
            store.addResourceVersion(res);
            res = store.RVs['common/*' + '/controller--controller'];
            cmp(res.source, source);
            A.isNotUndefined(res.mime);
            var expected = {
                type: 'application/javascript',
                charset: undefined
            };
            cmp(expected, res.mime);

            source = makeSource(fixtures+'/mojits/X', 'mojit', 'assets', 'x.css', true);
            res = {
                source: source,
                mojit: 'X',
                type: 'asset',
                name: 'x',
                id: 'asset-js-foo',
                affinity: 'common',
                selector: '*'
            };
            store.addResourceVersion(res);
            res = store.RVs['common/*' + '/asset-js-foo'];
            cmp(res.source, source);
            A.isNotUndefined(res.mime);
            expected = {
                type: 'text/css',
                charset: 'UTF-8'
            };
            cmp(expected, res.mime);

            source = makeSource(fixtures+'/mojits/X', 'mojit', 'assets', 'favicon.ico', true);
            res = {
                source: source,
                mojit: 'X',
                type: 'asset',
                name: 'x',
                id: 'asset-ico-favicon',
                affinity: 'common',
                selector: '*'
            };
            store.addResourceVersion(res);
            res = store.RVs['common/*' + '/asset-ico-favicon'];
            cmp(res.source, source);
            A.isNotUndefined(res.mime);
            expected = {
                type: 'image/x-icon',
                charset: undefined
            };
            cmp(expected, res.mime);
        }


    }));
    
    Y.Test.Runner.add(suite);
    
});
