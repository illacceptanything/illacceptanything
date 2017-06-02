/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('youtubemocked-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
            this.data = [ { title: "Test Video", id: "http://gdata.youtube.com/feeds/base/videos/ 1" }]; 
        },
        getData: function (params, callback) {
            callback(null, this.data);
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
