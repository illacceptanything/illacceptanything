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

// "routeparamssimple": {
//     "verbs": ["get"],
//     "path": "/RouteParamsSimple",
//     "call": "RouteParams.routeParamsSimple",
//     "params": "foo=fooval&bar=barval"
// },
app.get('/RouteParamsSimple', libmojito.dispatch('RouteParams.routeParamsSimple', { foo: 'fooval', bar: 'barval'}));

// "mergeparamssimple": {
//     "verbs": ["post"],
//     "path": "/MergeParamsSimple",
//     "call": "MergeParams.mergeParamsSimple",
//     "params": {
//         "likes": "Beer"
//     }
// }
app.post('/MergeParamsSimple', function (req, res, next) {
    req.params = req.params || {};
    req.params.likes = 'Beer';
    next();
}, libmojito.dispatch('MergeParams.mergeParamsSimple'));


// "mergeparams": {
//     "verbs": ["post"],
//     "path": "/MergeParams",
//     "call": "MergeParams.mergeParams",
//     "params": "likes=Beer"
//  },
app.post('/MergeParams', function (req, res, next) {
    req.params = req.params || {};
    req.params.likes = 'Beer';
    next();
}, libmojito.dispatch('MergeParams.mergeParams'));

// "routeparams": {
//     "verbs": ["get"],
//     "path": "/RouteParams",
//     "call": "RouteParams.routeParams",
//     "params": {
//        "foo": "fooval",
//        "bar": "barval"
//     }
// },
app.get('/RouteParams', libmojito.dispatch('RouteParams.routeParams', {foo: 'fooval', bar: 'barval'}));


// "default": {
//     "verbs": ["get", "post", "put", "head", "delete"],
//     "path": "/:mojit-id/:mojit-action",
//     "call": "{mojit-id}.{mojit-action}"
// }
dispatcherMiddleware = libmojito.dispatch('{mojitType}.{mojitAction}');
app.get('/:mojitType/:mojitAction', dispatcherMiddleware);
app.post('/:mojitType/:mojitAction', dispatcherMiddleware);
app.put('/:mojitType/:mojitAction', dispatcherMiddleware);
app.head('/:mojitType/:mojitAction', dispatcherMiddleware);
app['delete']('/:mojitType/:mojitAction', dispatcherMiddleware);

app.get('/status', function (req, res) {
    res.send('200 OK');
});

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});


module.exports = app;

