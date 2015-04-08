/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('stateful', function(Y, NAME) {

    var time = new Date().getTime();

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {
            ac.done({id: ac.config.get('id')});
        },

        pitch: function(ac) {
            this.logit('pitch');
            this.ball = ac.params.merged('ball');
            ac.done();
        },

        catch: function(ac) {
            var self = this;
            this.logit('catch');
            ac.models.get('model').getData(function(err, data) {
                ac.done({
                    ball: self.ball,
                    time: time,
                    model: data.modelId
                }, 'json');
            });
        },


        logit: function(msg) {
            Y.log(msg + time, 'warn', NAME);
        }

    };

}, '0.0.1', {requires: [
    'mojito-models-addon',
    'mojito-config-addon',
    'mojito-params-addon'
]});
