
/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('calendarmocked-model-yql', function (Y, NAME) {
    Y.mojito.models[NAME] = {
        init: function (config) {
            this.config = config;
            this.data = [ { entry: { summary: { content: "Test Calendar Event" }, link: { href: "https://www.google.com/calendar/feeds/fcde7kbrqnu7iccq9ofi9lqqf8%40group.calendar.google.com/public/basic" }, title: { content: "This is a test event." }}}]; 
        },
        getData: function (params, callback) {
            callback(null, this.data);
        }
    };
}, '0.0.1', {requires: ['yql', 'substitute']});
