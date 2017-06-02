YUI.add('PagedFlickr-tests', function(Y, NAME) {

    var suite = new YUITest.TestSuite(NAME),
        controller = null,
        A = YUITest.Assert,
        url = require('url'),
        qs = require('qs');

    suite.add(new YUITest.TestCase({

        name: 'PagedFlickr user tests',

        setUp: function() {
            controller = new Y.mojit.test.PagedFlickr.controller();
        },
        tearDown: function() {
            controller = null;
        },

        'test model data is passed into action context done': function() {
            var doneCalled = false,
                paramsCalled = false,
                urlMakeCalled = false,
                modelData = {my: 'data'},
                expectedData = {
                    images: modelData,
                    date: 'some date',
                    greeting: 'TITLE',
                    prev: {
                        title: 'PREV'
                    },
                    next: {
                        url: 'http://localhost:8666/flickr/page/2',
                        title: 'NEXT'
                    }
                },
                ac = Y.mojit.test.MockActionContext({
                    model: {
                        getFlickrImages: ['mojito', modelData]
                    }
                });

            ac.params = function(name) {
                paramsCalled = true;
                return name;
            };
            ac.i18n = {};
            ac.i18n.formatDate = function(date) {
                return('some date');
            };
            ac.i18n.lang = function(label) {
                return label;
            };
            ac.url = {};
            ac.url.make = function(id, action, params, verb) {
                urlMakeCalled = true;
                var query = qs.parse(params);
                return 'http://localhost:8666/flickr/page/' + query.page;
            };
            ac.done = function(data) {
                A.areSame(expectedData.images, data.images);
                A.areEqual(expectedData.date, data.date);
                A.areEqual(expectedData.greeting, data.greeting);
                A.areEqual(expectedData.prev.title, data.prev.title);
                A.isUndefined(data.prev.url);
                A.isUndefined(data.has_prev);
                A.areEqual(expectedData.next.url, data.next.url);
                A.areEqual(expectedData.next.title, data.next.title);
                doneCalled = true;
            };

            controller.index(ac);
            A.isTrue(doneCalled, 'ac.done never called');
            A.isTrue(paramsCalled, 'ac.params never called');
            A.isTrue(urlMakeCalled, 'ac.url.make never called');
        }

    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojit-test', 'PagedFlickr']});

