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
    app,
    dispatcherMiddleware;

app = express();
app.set('port', process.env.PORT || 8666);
libmojito.extend(app);

app.use(libmojito.middleware());
app.mojito.attachRoutes();

// "default": {
//     "verbs": ["get", "post", "put", "head", "delete"],
//     "path": "/:mojit-base/:mojit-action",
//     "call": "{mojit-base}.{mojit-action}"
// }
dispatcherMiddleware = libmojito.dispatch('{mojitBase}.{mojitAction}');
app.get('/:mojitBase/:mojitAction', dispatcherMiddleware);
app.post('/:mojitBase/:mojitAction', dispatcherMiddleware);
app.put('/:mojitBase/:mojitAction', dispatcherMiddleware);
app.head('/:mojitBase/:mojitAction', dispatcherMiddleware);
app['delete']('/:mojitBase/:mojitAction', dispatcherMiddleware);

app.get('/status', function (req, res) {
    res.send('200 OK');
});

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});

module.exports = app;

