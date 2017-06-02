/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('youtube', function (Y, NAME) {

/**
 * The Youtube module.
 *
 * @module Youtube
 */
    var youtubeMap = function (data) {
        Y.log("youtube: youtubeMap called", "info", NAME);
        Y.log("youtube: data", "info", NAME);
        Y.log(data, "info", NAME);
        var res = [];

        Y.Array.each(data, function (itm, idx, arr) {
            Y.log(itm, "info", NAME);
            var title = itm.title,
                id = itm.id.split("http://gdata.youtube.com/feeds/base/videos/")[1];
            //Y.log("youtubevid id:" + id, "info", NAME);
            res[idx] = {
                title: title,
                id: id
            };
        });
        return res;
    };
    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function (ac) {
            ac.models.get('youtube').getData({}, function (err, data) {
                Y.log("Youtube controller.server.js -index - model.getData:", "info", NAME);
                Y.log(data, "info", NAME);
                var res = [], title = "YUI YouTube Videos";

                Y.log("youtubmojit results:", "info", NAME);
                Y.log(res, "info", NAME);
                if (err) {
                    ac.error(err);
                } else {
                    // Create data structure from Web service response.
                    res = youtubeMap(data);
                    // Populate and render template.
                    ac.done({
                        title: title,
                        results: res
                    });
                }
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon', 'YoutubeModelYQL']});
