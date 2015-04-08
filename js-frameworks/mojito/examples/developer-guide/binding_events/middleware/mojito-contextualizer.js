module.exports = function() {
    var url = require('url'),
        query;
    return function(req, res, next) {

        if (!req.context) {
            req.context = {};
        }
        query = url.parse(req.url, true).query || {};
        req.context.environment = query.environment || '';
        next();
    };
};
