/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('youtube', function (Y, NAME) {

/**
 * The youtube module.
 *
 * @module youtube
 */
    var youtubeMap = function (ac, data) {
        Y.log("youtubeMap called");
        var res = [];

        Y.Array.each(data, function (itm, idx, arr) {
            Y.log(itm);
            var
                title = itm.title,
                id = itm.id.split("http://gdata.youtube.com/feeds/base/videos/")[1];
            Y.log("youtubevid id:" + id);
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
            ac.models.get('youtube').getData({}, function (data) {
                Y.log("youtube -index - model.getData:");
                Y.log(data);
                var res = [];
                res = youtubeMap(ac, data);

                Y.log("youtubmojit results:");
                Y.log(res);


                // populate youtube template
                ac.done({
                    title: "YUI YouTube Videos",
                    results: res
                });
            });
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon', 'youtube-model-yql']});
