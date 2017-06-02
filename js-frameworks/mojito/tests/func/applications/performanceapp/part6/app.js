/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
'use strict';

var debug = require('debug')('app'),
    express = require('express'),
    libmojito = require('../../../../../'),
    app;

app = express();
app.set('port', process.env.PORT || 8666);
libmojito.extend(app);

app.use(libmojito.middleware());
app.mojito.attachRoutes();

app.get('/status', function (req, res) {
    res.send('200 OK');
});

app.listen(app.get('port'), function () {
    debug('Server listening on port ' + app.get('port') + ' ' +
               'in ' + app.get('env') + ' mode');
});

module.exports = app;


