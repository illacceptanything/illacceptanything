/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('mojito-intl-addon', 'test', 'datatype-date', function(Y) {

    var suite = new Y.Test.Suite('mojito-intl-addon tests'),
        Assert = Y.Assert,
        Mock = Y.Mock;

    suite.add(new Y.Test.Case({

        name: 'intl tests',

        'test lang() gets translation from Y.Intl': function() {
            var command = {},
                adapter = null,
                ac = {
                    context: { lang: 'foo' },
                    instance: {
                        controller: 'controller-yui-module-name',
                        langs: { foo: true },
                        closestLang: 'foo'
                    }
                };

            var mockYIntl = Mock();
            Mock.expect(mockYIntl, {
                method: 'setLang',
                args: [ac.instance.controller, 'foo'],
                returns: 'true'
            });
            Mock.expect(mockYIntl, {
                method: 'get',
                args: [ac.instance.controller, 'key'],
                returns: 'translation'
            });
            Mock.expect(mockYIntl, {
                method: 'lookupBestLang',
                args: [Y.Mock.Value.String, Y.Mock.Value.Object],
                returns: 'foo'
            });

            var yIntl = Y.Intl;
            Y.Intl = mockYIntl;
            Y.Intl._mod = function () {
                return {};
            };

            var addon = new Y.mojito.addons.ac.intl(command, adapter, ac);
            var value = addon.lang('key');

            Y.Intl = yIntl;

            Assert.areEqual('translation', value, 'The return value of Y.Intl.get() was not used');
            Mock.verify(mockYIntl);
        },

        'test lang() formats translation from Y.Intl': function() {
            var command = {},
                adapter = null,
                ac = {
                    context: { lang: 'foo' },
                    instance: {
                        controller: 'controller-yui-module-name',
                        langs: { foo: true },
                        closestLang: 'foo'
                    }
                };

            var mockYIntl = Mock();
            Mock.expect(mockYIntl, {
                method: 'setLang',
                args: [ac.instance.controller, 'foo'],
                returns: 'true'
            });
            Mock.expect(mockYIntl, {
                method: 'get',
                args: [ac.instance.controller, 'key'],
                returns: 'translation {0} {1}'
            });
            Mock.expect(mockYIntl, {
                method: 'lookupBestLang',
                args: [Y.Mock.Value.String, Y.Mock.Value.Object],
                returns: 'foo'
            });

            var yIntl = Y.Intl;
            Y.Intl = mockYIntl;

            Y.Intl._mod = function () {
                return {};
            };

            var addon = new Y.mojito.addons.ac.intl(command, adapter, ac);
            var value = addon.lang('key', ['param1', 'param2']);

            Y.Intl = yIntl;

            Assert.areEqual('translation param1 param2', value, 'The return value of Y.Intl.get() was not formatted');
            Mock.verify(mockYIntl);
        },

        'test formatDate() delegates to Y.DataType.Date.format': function() {
            var command = {},
                adapter = null,
                ac,
                argDate = new Date();

            ac = {
                context: { lang: 'foo' },
                instance: {
                    controller: 'controller-yui-module-name',
                    langs: { foo: true },
                    closestLang: 'foo'
                }
            };

            var mockYIntl = Mock();
            Mock.expect(mockYIntl, {
                method: 'setLang',
                args: ['datatype-date-format', 'foo'],
                returns: 'true'
            });
            Mock.expect(mockYIntl, {
                method: 'lookupBestLang',
                args: [Y.Mock.Value.String, Y.Mock.Value.Object],
                returns: 'foo'
            });

            var yIntl = Y.Intl;
            Y.Intl = mockYIntl;
            Y.Intl._mod = function () {
                return {};
            };

            var mockYDataTypeDate = Mock();
            Mock.expect(mockYDataTypeDate, {
                method: 'format',
                args: [argDate, Mock.Value(function(o) {
                    Assert.areSame("%x", o.format, "Unexpected date formatting specifier");
                })],
                returns: 'formattedDate'
            });

            var yDataTypeDate = Y.DataType.Date;
            Y.DataType.Date = mockYDataTypeDate;

            var addon = new Y.mojito.addons.ac.intl(command, adapter, ac);
            var value;
            try {
                value = addon.formatDate(argDate);
            } finally {
                Y.Intl = yIntl;
                Y.DataType.Date = yDataTypeDate;
            }

            Assert.areEqual('formattedDate', value, 'The return value of Y.DataType.Date.format() was not used');
            Mock.verify(mockYIntl);
            Mock.verify(mockYDataTypeDate);
        }

    }));

    Y.Test.Runner.add(suite);

});
