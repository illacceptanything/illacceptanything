/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('gallerymocked-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
            this.data = { json: [ "gallery-test" ] };  
        },
        getData: function (params, tablePath, callback) {
            callback(null, this.data); 
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
