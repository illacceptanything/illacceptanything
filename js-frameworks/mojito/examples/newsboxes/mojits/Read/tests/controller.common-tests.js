/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, unparam:true*/
/*global YUI, YUITest*/


YUI.add('read-tests', function(Y, NAME) {
    'use strict';

    var suite = new YUITest.TestSuite(NAME),
        A = YUITest.Assert,

        boomtown_feedmeta = {
            'name': 'BoomTown',
            'url': 'http://kara.allthingsd.com/feed/',
            'id': 'boomtown'
        },
        boomtown_stories_in = [
            {
                'title': 'Viral Video: Liz Gannes May (or May Not) See Hubski Now!',
                'link': 'http://allthingsd.com/20111003/viral-video-liz-gannes-may-or-may-not-see-hubski-now/',
                'pubDate': 1317706064728,
                'description': 'Nice try, Steve Clausnitzer!'
            },
            {
                'title': 'Attention Shoppers: Coupons.com Grabs $30M in Funding From Greylock',
                'link': 'http://allthingsd.com/20111003/attention-shoppers-coupons-com-grabs-30m-in-funding-from-greylock/',
                'pubDate': 1317706064728,
                'description': 'VCs search for a bargain in longtime digital promotions site.'
            },
            {
                'title': 'HuffPost at One Biiiilllliiion Monthly Page Views: More Buying, More Launching, More Hiring',
                'link': 'http://allthingsd.com/20111003/huffpo-at-1b-monthly-page-views-more-buying-more-launching-more-hiring/',
                'pubDate': 1317706064728,
                'description': 'It\'s definitely better than one million!'
            },
            {
                'title': 'He\'s Back: Bob Pittman Named CEO of Clear Channel',
                'link': 'http://allthingsd.com/20111002/hes-back-bob-pittman-named-ceo-of-clear-channel/',
                'pubDate': 1317706064728,
                'description': 'Longtime media and Internet exec Bob Pittman has been named CEO of radio broadcast and outdoor advertising giant Clear Channel, the company announced today.'
            },
            {
                'title': 'Airbnb, Investor Chamath Palihapitiya Settle Differences; Employees Will Get Liquidity',
                'link': 'http://allthingsd.com/20111002/airbnb-investor-chamath-palihapitiya-settle-differences-with-employees-to-get-liquidity/',
                'pubDate': 1317706064728,
                'description': 'The former Facebook employee is back in the fold for Airbnb\'s financing, after further conversations with the apartment-sharing company regarding its upcoming financing round.'
            },
            {
                'title': 'As U.S.-Listed China Internet Stocks Dive, Renren CEO Smacks Alibaba on the Way Down (And Gets Smacked Back)',
                'link': 'http://allthingsd.com/20111002/as-u-s-listed-china-internet-stocks-dive-renren-ceo-smacks-alibaba-on-the-way-down-and-gets-smacked-back/',
                'pubDate': 1317706064728,
                'description': 'As Chinese Internet exec Joe Chen of Renren snipes at a competitor there, there\'s a bigger problem for that country\'s Web companies.'
            },
            {
                'title': 'Email: Chamath Palihapitiya Decries Airbnb\'s Recent $112M Funding for Founder Control and Cash-Out',
                'link': 'http://allthingsd.com/20111001/vcs-unite-chamath-palihapitiya-decries-airbnbs-recent-112m-funding-for-excessive-founder-control-and-cashout-in-email/',
                'pubDate': 1317706064728,
                'description': 'Here\'s some electric weekend reading for those interested in the push-and-pull between venture investors and start-ups in the frothy Web 2.0 environment.'
            },
            {
                'title': 'Only One Yahoo Fearless Leader Note This Week: Please Ignore the Unignorable Rumors!',
                'link': 'http://allthingsd.com/20110930/only-one-yahoo-fearless-leader-note-this-week-please-ignore-the-un-ignorable-rumors/',
                'pubDate': 1317706064728,
                'description': 'Here\'s the weekly internal management email from the Silicon Valley Internet giant (just because I can).'
            },
            {
                'title': 'Alibaba\'s Jack Ma at Stanford: \'We Are Very Interested\' in Buying the \'Whole\' of Yahoo',
                'link': 'http://allthingsd.com/20110930/jack-ma-at-stanford-we-are-very-interested-in-buying-yahoo/',
                'pubDate': 1317706064728,
                'description': 'In answer to a direct question about whether his company was going to buy Yahoo at a forum at Stanford University in Silicon Valley this afternoon, Alibaba Chairman and CEO Jack Ma said: \'We are very interested\' in buying all of it.'
            },
            {
                'title': 'Viral Video: Floppy Disk Drives Conduct \'Star Wars\' Symphony',
                'link': 'http://allthingsd.com/20110930/viral-video-floppy-disk-drive-in-star-wars-symphony/',
                'pubDate': 1317706064728,
                'description': 'It does not get any better than this: A pair of floppy disk drives playing the \'Imperial March\' from the classic movie \'Star Wars.\'\n\nAmirite?'
            }
        ],
        boomtown_vudata_out = {
            feedname: 'BoomTown',
            spaceid: '999',
            stories: [
                {
                    "title": "Viral Video: Liz Gannes May (or May Not) See Hubski Now!",
                    "link": "http://allthingsd.com/20111003/viral-video-liz-gannes-may-or-may-not-see-hubski-now/",
                    "pubDate": 1317706064728,
                    "description": "Nice try, Steve Clausnitzer!",
                    "prev": "?start=9",
                    "next": "?start=1",
                    "css_style": "xx-large"
                },
                {
                    "title": "Attention Shoppers: Coupons.com Grabs $30M in Funding From Greylock",
                    "link": "http://allthingsd.com/20111003/attention-shoppers-coupons-com-grabs-30m-in-funding-from-greylock/",
                    "pubDate": 1317706064728,
                    "description": "VCs search for a bargain in longtime digital promotions site.",
                    "prev": "?start=9",
                    "next": "?start=2",
                    "css_style": "xx-large"
                },
                {
                    "title": "HuffPost at One Biiiilllliiion Monthly Page Views: More Buying, More Launching, More Hiring",
                    "link": "http://allthingsd.com/20111003/huffpo-at-1b-monthly-page-views-more-buying-more-launching-more-hiring/",
                    "pubDate": 1317706064728,
                    "description": "It's definitely better than one million!",
                    "prev": "?start=1",
                    "next": "?start=3",
                    "css_style": "xx-large"
                },
                {
                    "title": "He's Back: Bob Pittman Named CEO of Clear Channel",
                    "link": "http://allthingsd.com/20111002/hes-back-bob-pittman-named-ceo-of-clear-channel/",
                    "pubDate": 1317706064728,
                    "description": "Longtime media and Internet exec Bob Pittman has been named CEO of radio broadcast and outdoor advertising giant Clear Channel, the company announced today.",
                    "prev": "?start=2",
                    "next": "?start=4",
                    "css_style": "xx-large"
                },
                {
                    "title": "Airbnb, Investor Chamath Palihapitiya Settle Differences; Employees Will Get Liquidity",
                    "link": "http://allthingsd.com/20111002/airbnb-investor-chamath-palihapitiya-settle-differences-with-employees-to-get-liquidity/",
                    "pubDate": 1317706064728,
                    "description": "The former Facebook employee is back in the fold for Airbnb's financing, after further conversations with the apartment-sharing company regarding its upcoming financing round.",
                    "prev": "?start=3",
                    "next": "?start=5",
                    "css_style": "xx-large"
                },
                {
                    "title": "As U.S.-Listed China Internet Stocks Dive, Renren CEO Smacks Alibaba on the Way Down (And Gets Smacked Back)",
                    "link": "http://allthingsd.com/20111002/as-u-s-listed-china-internet-stocks-dive-renren-ceo-smacks-alibaba-on-the-way-down-and-gets-smacked-back/",
                    "pubDate": 1317706064728,
                    "description": "As Chinese Internet exec Joe Chen of Renren snipes at a competitor there, there's a bigger problem for that country's Web companies.",
                    "prev": "?start=4",
                    "next": "?start=6",
                    "css_style": "xx-large"
                },
                {
                    "title": "Email: Chamath Palihapitiya Decries Airbnb's Recent $112M Funding for Founder Control and Cash-Out",
                    "link": "http://allthingsd.com/20111001/vcs-unite-chamath-palihapitiya-decries-airbnbs-recent-112m-funding-for-excessive-founder-control-and-cashout-in-email/",
                    "pubDate": 1317706064728,
                    "description": "Here's some electric weekend reading for those interested in the push-and-pull between venture investors and start-ups in the frothy Web 2.0 environment.",
                    "prev": "?start=5",
                    "next": "?start=7",
                    "css_style": "xx-large"
                },
                {
                    "title": "Only One Yahoo Fearless Leader Note This Week: Please Ignore the Unignorable Rumors!",
                    "link": "http://allthingsd.com/20110930/only-one-yahoo-fearless-leader-note-this-week-please-ignore-the-un-ignorable-rumors/",
                    "pubDate": 1317706064728,
                    "description": "Here's the weekly internal management email from the Silicon Valley Internet giant (just because I can).",
                    "prev": "?start=6",
                    "next": "?start=8",
                    "css_style": "xx-large"
                },
                {
                    "title": "Alibaba's Jack Ma at Stanford: 'We Are Very Interested' in Buying the 'Whole' of Yahoo",
                    "link": "http://allthingsd.com/20110930/jack-ma-at-stanford-we-are-very-interested-in-buying-yahoo/",
                    "pubDate": 1317706064728,
                    "description": "In answer to a direct question about whether his company was going to buy Yahoo at a forum at Stanford University in Silicon Valley this afternoon, Alibaba Chairman and CEO Jack Ma said: 'We are very interested' in buying all of it.",
                    "prev": "?start=7",
                    "next": "?start=9",
                    "css_style": "x-large"
                },
                {
                    "title": "Viral Video: Floppy Disk Drives Conduct 'Star Wars' Symphony",
                    "link": "http://allthingsd.com/20110930/viral-video-floppy-disk-drive-in-star-wars-symphony/",
                    "pubDate": 1317706064728,
                    "description": "It does not get any better than this: A pair of floppy disk drives playing the 'Imperial March' from the classic movie 'Star Wars.'\\n\\nAmirite?",
                    "prev": "?start=8",
                    "next": "?start=0",
                    "css_style": "xx-large"
                }
            ],
            navdots: [{}, {}, {}, {}, {}, {}, {}, {}, {}, {}]
        },
        controller;

    function getAc() {
        var conf = {
                assets: {
                    top: {
                        css: ['/static/Read/assets/read.css']
                    }
                },
                limit: 10
            },
            definitions = {
                'yahoocore': {
                    'name': 'Yahoo! Today',
                    'url': 'SELECT * FROM deadeye.content.articles.coke',
                    'model': 'core'
                },
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
                'boomtown': {
                    'name': 'BoomTown',
                    'url': 'http://kara.allthingsd.com/feed/'
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

                'yqltheater': {
                    'name': 'YQL Theater',
                    'url': 'http://feeds.yuiblog.com/yuiblog/yui-theater'
                },
                'foo2': {
                    'name': 'foo2',
                    'url': 'http://rss.omg.yahoo.com/-/news/latest'
                },
                'foo3': {
                    'name': 'foo3',
                    'url': 'http://rss.omg.yahoo.com/-/news/latest'
                },
                'foo4': {
                    'name': 'foo4',
                    'url': 'http://rss.omg.yahoo.com/-/news/latest'
                },
                'foo5': {
                    'name': 'foo5',
                    'url': 'http://rss.omg.yahoo.com/-/news/latest'
                }
            };

        return {
            config: {
                getDefinition: function(ignored) {
                    return definitions;
                },
                get: function() {
                    return conf;
                }
            },
            params: {
                merged: function(feedname) {
                    return boomtown_feedmeta.id;
                },
                url: function(keyname) {
                    return null;
                }
            },
            models: {
                get: function (model) {
                    return {
                        get: function(feedmeta, cb) {
                            cb(null, feedmeta, boomtown_vudata_out);
                        }
                    };
                }
            }
        };

    }

    suite.add(new YUITest.TestCase({

        name: 'read tests',

        setUp: function() {
            controller = Y.mojito.controllers["read"];
        },

        tearDown: function() {
            controller = null;
        },

        'sizing test 1': function() {
            A.areSame('xx-large', controller.test.size(100, 100));
        },

        'sizing test 2': function() {
            A.areSame('xx-large', controller.test.size(100, 160));
        },

        'sizing test 3': function() {
            A.areSame('x-large', controller.test.size(150, 100));
        },

        'sizing test 4': function() {
            A.areSame('xx-large', controller.test.size(71, 200));
        },

        'sizing test 5': function() {
            A.areSame('x-large', controller.test.size(71, 400));
        },

        'sizing test 6': function() {
            A.areSame('large', controller.test.size(71, 750));
        },

        'sizing test 6b': function() {
            A.areSame('large', controller.test.size(30, 750));
        },

        'sizing test 7': function() {
            A.areSame('medium', controller.test.size(1000, 100));
        },

        'sizing test 8': function() {
            A.areSame('medium', controller.test.size(1, 850));
        },

        'compose spaceid not set yet': function() {
            A.areSame(undefined, controller.test.compose(boomtown_feedmeta, boomtown_stories_in).spaceid);
        },

        'compose feedname': function() {
            A.areSame('BoomTown', controller.test.compose(boomtown_feedmeta, boomtown_stories_in).feedname);
        },

        'compose navdots for every story': function() {
            var vu = controller.test.compose(boomtown_feedmeta, boomtown_stories_in);

            A.areSame(10, vu.navdots.length);
            A.areSame(vu.stories.length, vu.navdots.length);
        },

        'compose adds css': function() {
            var vu = controller.test.compose(boomtown_feedmeta, boomtown_stories_in);
            Y.each(vu.stories, function(story, i) {
                A.isTypeOf('string', story.css_style);
            });
        },

        'compose prev points to last for 1st story': function() {
            var stories = boomtown_vudata_out.stories;
            A.areSame(10, stories.length);
            A.areSame('?start=' + (stories.length - 1), stories[0].prev);
        },

        'compose next points to 1st for last story': function() {
            var stories = boomtown_vudata_out.stories;
            A.areSame('?start=0', stories[stories.length - 1].next);
        },

        'compose next points to next story': function() {
            var stories = boomtown_vudata_out.stories,
                i,
                n = stories.length - 1;

            for (i = 0; i < n; i += 1) {
                A.areSame('?start=' + (i + 1), stories[i].next);
            }
        },

        'compose prev points to prev story': function() {
            var stories = boomtown_vudata_out.stories,
                i,
                n = stories.length;

            for (i = 2; i < n; i += 1) {
                A.areSame('?start=' + (i - 1), stories[i].prev);
            }
        },

        'oh noes, icanhazfeedzplz?': function() {
            var ac = getAc();
            ac.config.getDefinition = function() {
                return {};
            };

            ac.done = function(data) {
                A.areSame('oh noes!', data.title);
            };

            controller.index(ac);
        },

        'action test': function() {
            var ac = getAc();

            ac.done = function(data) {
                A.areSame('BoomTown', data.feedname);
                A.areSame('BoomTown', data.stories.feedname);
                A.areSame(10, data.stories.stories.length);
                A.areSame(10, data.stories.navdots.length);
            };

            controller.index(ac);
        }

    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: [
    'mojito-test',
    'read'
]});
