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
    app;

app = express();
app.set('port', process.env.PORT || 8666);
libmojito.extend(app);

app.use(libmojito.middleware());

app.use(app.router);

app.get('/ok', function (req, res) {
    res.send('OK');
});
app.use(function (err, req, res, next) {
    return res.send('Error: ' + err);
});

app.get('/status', function (req, res) {
    res.send('200 OK');
});

app.get('/:mojit/:action', libmojito.dispatch('{mojit}.{action}'));
app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});
module.exports = app;

