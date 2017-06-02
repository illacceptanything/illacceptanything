/*
* Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
* Copyrights licensed under the New BSD License.
* See the accompanying LICENSE file for terms.
*/


/*jslint node:true*/

'use strict';

var debug = require('debug')('app'),
    express = require('express'),
    mojito = require('../../../'),
    app;

app = express();
mojito.extend(app);
app.use(mojito.middleware());
app.mojito.attachRoutes();


// Example usage on how to execute anonymous mojit. 
app.get('/:mojitType/:mojitAction', function (req, res, next) {
    var type = req.params.mojitType,
        action = req.params.mojitAction;

    return mojito.dispatch(type + '.' + action)(req, res, next);
});

app.get('/status', function (req, res) {
    res.send('200 OK');
});

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});
