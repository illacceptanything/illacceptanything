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


app.get('/', libmojito.dispatch('frame.index'));
app.get('/example1', libmojito.dispatch('frame.example1'));
app.get('/example2', libmojito.dispatch('frame.example2'));
app.post('/example2', libmojito.dispatch('frame.example2'));
app.get('/example3', libmojito.dispatch('frame.example3', { "from": "routing", "foo": "bar", "bar": "foo" }));
app.get('/example4', libmojito.dispatch('frame.example4', { "from": "routing", "foo3": "bar3" }));
app.post('/example4', libmojito.dispatch('frame.example4', { "from": "routing", "foo3": "bar3" }));

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});
module.exports = app;

