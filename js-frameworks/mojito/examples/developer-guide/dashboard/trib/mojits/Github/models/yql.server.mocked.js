/*jslint anon:true, sloppy:true, nomen:true, indent: 4, white: false*/
/*global YUI*/
YUI.add('statsmocked-model-yql', function (Y, NAME) {

    Y.mojito.models[NAME] = {

        init: function (config) {
            this.config = config;
            this.data = [ { json: { type: "CreateEvent", actor: { login: "testuser" }, payload: { href: "github.com" }}}]; 
        },
        getData: function (params, yqlTable, id, repo, callback) {
            callback(null, this.data);
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
