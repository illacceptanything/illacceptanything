/*jslint anon:true, sloppy:true, nomen:true, stupid:true*/

YUI.add('addons-viewengine-ejs', function (Y, NAME) {

    var ejs = require('ejs'),
        fs = require('fs');
    function EjsAdapter(viewId) {
        this.viewId = viewId;
    }
    EjsAdapter.prototype = {

        render: function (data, mojitType, tmpl, adapter, meta, more) {
            var me = this,
                handleRender = function (output) {
                    output.addListener('data', function (c) {
                        adapter.flush(c, meta);
                    });
                    output.addListener('end', function () {
                        if (!more) {
                            adapter.done('', meta);
                        }
                    });
                },
                result = ejs.render(this.compiler(tmpl), data);
            Y.log('Rendering template "' + tmpl + '"', 'mojito', NAME);
            console.log(result);
            adapter.done(result, meta);
        },
        compiler: function (tmpl) {
            return fs.readFileSync(tmpl, 'utf8');
        }
    };
    Y.namespace('mojito.addons.viewEngines').ejs = EjsAdapter;
}, '0.1.0', {requires: []});
