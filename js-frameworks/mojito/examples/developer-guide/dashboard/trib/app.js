/*
* Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
* Copyrights licensed under the New BSD License.
* See the accompanying LICENSE file for terms.
*/


/*jslint node:true*/

'use strict';

var debug = require('debug')('app'),
    express = require('express'),
    libmojito = require('../../../../'),
    customContextualizerMiddleware = require('./middleware/mojito-contextualizer.js'),
    app;

app = express();
app.set('port', process.env.PORT || 8666);
libmojito.extend(app);

app.use(customContextualizerMiddleware());
app.use(libmojito.middleware());

// To use the routing configured in `routes.json`, which
// is deprecated, you uncomment this line.
// app.mojito.attachRoutes();

app.get('/', function (req, res, next) {
    req.params.view_type = "yui";
    next();
}, libmojito.dispatch('tribframe.index'));

app.get('/mojito', function (req, res, next) {
    req.params.view_type = "mojito";
    next();
}, libmojito.dispatch('tribframe.index'));

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});
module.exports = app;

