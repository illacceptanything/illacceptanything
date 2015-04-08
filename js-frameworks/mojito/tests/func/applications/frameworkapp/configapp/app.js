/*
* Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
* Copyrights licensed under the New BSD License.
* See the accompanying LICENSE file for terms.
*/


/*jslint node:true*/

'use strict';

var debug = require('debug')('app'),
    express = require('express'),
    libmojito = require('../../../../..'),
    libmm = require('minimist'),
    apputil = require('./util'),
    app,
    argv,
    context,
    dispatcherMiddleware;

argv = libmm(process.argv.slice(2));
context = argv.context || '';
context = apputil.parseContext(context);

app = express();
app.set('port', process.env.PORT || 8666);
libmojito.extend(app, {
    context: context
});

app.use(libmojito.middleware());
app.mojito.attachRoutes();

dispatcherMiddleware = libmojito.dispatch('{type}.{action}');
app.get('/:type/:action', dispatcherMiddleware);
app.post('/:type/:action', dispatcherMiddleware);
app.put('/:type/:action', dispatcherMiddleware);
app.head('/:type/:action', dispatcherMiddleware);
app['delete']('/:type/:action', dispatcherMiddleware);

app.get('/status', function (req, res) {
    res.send('200 OK');
});

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});

module.exports = app;

