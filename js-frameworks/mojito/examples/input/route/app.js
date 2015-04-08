/*
* Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
* Copyrights licensed under the New BSD License.
* See the accompanying LICENSE file for terms.
*/


/*jslint node:true*/

'use strict';

var debug = require('debug')('app'),
    express = require('express'),
    libmojito = require('../../../'),
    app;

app = express();
app.set('port', process.env.PORT || 8666);
libmojito.extend(app);

app.use(libmojito.middleware());
app.mojito.attachRoutes();

// This shows an example how to setup route params for specific paths
// Previously, this was done via `routes.json` using the `params` property.
app.get('/', function (req, res, next) {
    // foo=fooval&bar=barval
    // req.params = req.params || {};
    req.params.foo = 'fooval';
    req.params.bar = 'barval';
    next();
}, libmojito.dispatch('route-grabber.index'));

app.get('/status', function (req, res) {
    res.send('200 OK');
});

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});
module.exports = app;

