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

// To use the routing configured in `routes.json`, which
// is deprecated, you uncomment this line.
// app.mojito.attachRoutes();

app.get('/status', function (req, res) {
    res.send('200 OK');
});

// Defining route `GET /` and executing (dispatching)
// the action `index` of the mojit instance `mapped_mojit`.
app.get('/', libmojito.dispatch('mapped_mojit.index'));

// Defining route `GET /index` and executing (dispatching)
// the action `index` of the mojit instance `mapped_mojit`.
app.get('/index', libmojito.dispatch('mapped_mojit.index'));

// Defining the route `POST /*` and executing (dispatching)
// the action `post_params` of the mojit instance `post-route`.
app.post('/show', libmojito.dispatch('mapped_mojit.show'));

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});
module.exports = app;
