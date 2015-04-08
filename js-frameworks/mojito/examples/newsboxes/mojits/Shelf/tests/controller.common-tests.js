/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI,YUITest*/


YUI.add('shelf-tests', function(Y, NAME) {

    var suite = new YUITest.TestSuite(NAME),
        controller = null,
        A = YUITest.Assert;

    function getAc() {
        var noop = function() {},
            _definitions = {
                'bbcnews': {
                    'name': 'BBC World News',
                    'url': 'http://feeds.bbci.co.uk/news/world/rss.xml'
                },
                'yahoonews': {
                    'name': 'Yahoo! Internet News',
                    'url': 'http://rss.news.yahoo.com/rss/internet'
                },
                'techcrunch': {
                    'name': 'TechCrunch',
                    'url': 'http://feeds.feedburner.com/TechCrunch'
                },
                'yahoopersonaltech': {
                    'name': 'Yahoo! Gadgets & Personal Tech',
                    'url': 'http://rss.news.yahoo.com/rss/personaltech'
                },
                'allthingsd': {
                    'name': 'AllThingsD',
                    'url': 'http://allthingsd.com/feed/'
                },
                'yahoostocks': {
                    'name': 'Yahoo! Stock Markets News',
                    'url': 'http://rss.news.yahoo.com/rss/stocks'
                },
                'bbcbusiness': {
                    'name': 'BBC Business News',
                    'url': 'http://feeds.bbci.co.uk/news/business/rss.xml'
                },
                'yahooomg': {
                    'name': 'Yahoo! OMG',
                    'url': 'http://rss.omg.yahoo.com/-/news/latest'
                },
                'yui': {
                    'name': 'YUI Blog',
                    'url': 'http://feeds.yuiblog.com/YahooUserInterfaceBlog'
                },
                'yql': {
                    'name': 'YQL Blog',
                    'url': 'http://yqlblog.net/blog/feed/'
                },
                'ysearch': {
                    'name': 'Yahoo Search Blog',
                    'url': 'http://www.ysearchblog.com/feed/'
                }
            };

        return {
            config: {
                getDefinition: function() {
                    return _definitions;
                }
            },
            i13n: {
                stampPageView: noop,
                trackLink: noop
            },
            models: {
                feeds: {
                    load: function(err, data, cb) {
                        cb(err, data);
                    },
                    fetch: function(err, cb) {
                        cb(err, Y.Object.values(_definitions));
                    }
                }
            }
        };
    }

    suite.add(new YUITest.TestCase({

        name: 'ShelfController tests',

        setUp: function() {
            A.isNull(controller);
            controller = Y.mojito.controllers["shelf"];
            A.isNotNull(controller);
        },

        tearDown: function() {
            controller = null;
        },

        'test 0': function() {
            A.isNotNull(controller);
        },

        'test init config has 11 feeds': function() {
            var ac = getAc();
            ac.composite = {
                done: function(vudata) {
                    A.areSame(11, vudata.tiles.length);
                }
            };

            controller.index(ac);
        },

        'BBC is 1st feed passed to ac.done()': function() {
            var ac = getAc();
            ac.composite = {
                done: function(vudata) {
                    A.areSame('BBC World News', vudata.tiles[0].name);
                    A.areSame('http://feeds.bbci.co.uk/news/world/rss.xml',
                        vudata.tiles[0].url);
                }
            };
            controller.index(ac);
        },

        'omg is 8th': function() {
            var ac = getAc();
            ac.composite = {
                done: function(vudata) {
                    A.areSame('Yahoo! OMG', vudata.tiles[7].name);
                }
            };
            controller.index(ac);
        }

    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: [
    'mojito-test',
    'shelf',
    'oop'
]});
