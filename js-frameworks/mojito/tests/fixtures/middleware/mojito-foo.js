
module.exports = function (midConfig) {
    return function (req, res, next) {
        res.midConfig = midConfig;
        next();
    };
};
