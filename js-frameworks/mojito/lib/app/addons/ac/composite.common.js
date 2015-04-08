/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/


/**
 * @module ActionContextAddon
 */
YUI.add('mojito-composite-addon', function (Y, NAME) {

    /**
    * <strong>Access point:</strong> <em>ac.composite.*</em>
    * Provides methods for working with many Mojits.
    * @class Composite.common
    */
    function Addon(command, adapter, ac) {
        this.command = command;
        this.dispatch = ac._dispatch;
        this.ac = ac;
        this.adapter = adapter;
        this.queue = new Y.Parallel({
            context: this
        });
    }

    Addon.prototype = {

        namespace: 'composite',

        /**
         * Automatically dispatches all the children of this mojit and collects
         * their executed values into the view template, keyed by the child's
         * name within the mojit's configuration. For example, given the mojit
         * spec:
         *
         *
<pre>
"specs": {
    "parent": {
        "type": "MyCompositeMojit",
         "config": {
             "children": {
                 "foo": {
                     "type": "FooMojit"
                 },
                 "bar": {
                     "type": "BarMojit"
                 }
             }
         }
    }
        }
</pre>
         * And given the view template:
<pre>
&lt;div id=&quot;{{mojit_view_id}}&quot;&gt;
&lt;h1&gt;{{title}}&lt;/h1&gt;
&lt;div class=&quot;fooslot&quot;&gt;
    {{{foo}}}
&lt;/div&gt;
&lt;div class=&quot;barslot&quot;&gt;
    {{{bar}}}
&lt;/div&gt;
&lt;/div&gt;
</pre>
         * And the controller:
<pre>
Y.mojito.controller = {
    index: function(ac) {
        ac.composite.done({
            title: 'Hello there'
        });
    }
};
</pre>
         * This will execute the child intances of the "FooMojit" and
         * "BarMojit", returning their rendered values into the parent's view
         * template, thus rendering the full parent view including the children.
         * The API of this method is equivalent to ac.done().
         * @method done
         * @param {object} templateData The data you want return by the request.
         * @param {object} parentMeta Any meta-data required to service the request.
         */
        done: function (templateData, parentMeta) {
            var ac = this.ac,
                cfg = this.command.instance.config,
                children = cfg.children;

            templateData = templateData || {};
            parentMeta   = parentMeta || {};

            // Backward Compatibility block
            if (templateData.template) {
                Y.log('ac.composite.done({template:{title: "..."}}) is a legacy API, ' +
                        'use ac.composite.done({title: "..."}) instead.', 'warn', NAME);
                templateData = templateData.template;
            }

            if (!children || Y.Object.size(children) === 0) {
                throw new Error('Cannot run composite mojit children because' +
                                ' there are no children defined in the' +
                                ' composite mojit spec.');
            }

            this.execute(cfg, function (data, meta) {

                parentMeta.assets = Y.mojito.util.metaMerge(
                    parentMeta.assets || {},
                    meta.assets || {}
                );
                // 1. templateData and data are normally exclusive, in which case
                // the prority is not relevent. In which case we set D
                // 2. parentMeta and meta should be merged to preserve the children
                // binders map, assets, etc. but giving parentMeta the priority in case
                // a custom configuration in the parent should overrule something coming
                // from the chindren merged meta
                ac.done(Y.merge(data, templateData), Y.mojito.util.metaMerge(parentMeta, meta));

            }, this);
        },


        /**
         * This method requires an explicit config object and returns
         * a RMP compliant object via a callback.
         *
<pre>
cfg = {
    children: {
        slot-1: {
            type: "default",
            action: "index"
        },
        slot-2: {
            type: "default",
            action: "index",
            params: {
                route: {},
                url: {},
                body: {},
                file: {}
            }
        }
    },
    assets: {}
        }
</pre>
         *
         * The "callback" is an object containg the child slots with its
         * rendered data.
         *
<pre>
callback({
    slot-1: <string>,
    slot-2: <string>
        },
        {
   http: {}
   assets: {}
        })
</pre>
         * @method execute
         * @param {object} cfg The configuration object to be used.
         * @param {function} cb The callback that will be called.
         */
        execute: function (cfg, cb) {

            var ac = this.ac,
                content = {},
                my = this,
                meta = {},
                children = cfg.children || {},
                child;

            // check to ensure children is an Object, not an array
            if (Y.Lang.isArray(cfg.children)) {
                throw new Error('Cannot process children in the format of an' +
                                ' array. \'children\' must be an object.');
            }

            // HookSystem::StartBlock
            Y.mojito.hooks.hook('addon', this.adapter.hook, 'start', my, cfg);
            // HookSystem::EndBlock

            meta.children = children;

            for (child in children) {
                if (children.hasOwnProperty(child)) {
                    children[child] = this.addChild(child, children[child]);
                }
            }

            this.queue.done(function (results) {
                var i;
                // HookSystem::StartBlock
                Y.mojito.hooks.hook('addon', my.adapter.hook, 'end', my);
                // HookSystem::EndBlock

                if (my.failed) {
                    // skiping due to an error during queue process
                    return;
                }

                // Reference the data we want from the "results" into our
                // "content" obj Also merge the meta we collected.
                for (i = 0; i < results.length; i += 1) {
                    content[results[i].name] = results[i].data;
                    if (results[i].meta) {
                        meta = Y.mojito.util.metaMerge(meta,
                                results[i].meta);
                    }
                }

                // Mix in the assets given via the config
                if (cfg.assets) {
                    if (!meta.assets) {
                        meta.assets = {};
                    }
                    ac.assets.mixAssets(meta.assets, cfg.assets);
                }

                this.queue = new Y.Parallel({
                    context: this
                });

                cb(content, meta);
            });
        },


        /**
         * This method allow you to add more childs into the queue of children
         * manually. By default, the `config.children` structure will be processed
         * when calling `ac.composite.done`, but you can add more childs programatically
         * before or after calling `ac.composite.done` if you need to.
         * This is useful when you want to have more control over the children collection.
         *
<pre>
ac.composite.addChild('slot-1', {
    type: "foo",
    action: "index"
})
</pre>
         * @method addChild
         * @param {string} childName The mojit instance name that will be used
         * in a template in a form of {{{<childName>}}}.
         * @param {object} child The configuration object for the child.
         * @return {object} The normalized configuration object for the
         * child, in case the child has to be proxied.
         */
        addChild: function (childName, child) {

            var originalChild = child,
                my = this,
                childAdapter,
                newCommand,
                id;

            // check to ensure children doesn't have a null child
            // in which case it will be automatically skipped to
            // facilitate disabling children based on the context.
            if (!child) {
                return;
            }

            // first off, check to see if this child's execution should be
            // deferred
            if (child.defer) {
                // it doesn't make sense to have a deferred child with a
                // proxy, because the defer means to proxy it
                // through the LazyLoad mojit
                if (Y.Lang.isObject(child.proxy)) {
                    throw new Error('Cannot specify a child mojit spec' +
                                    ' with both \'defer\' and \'proxy\'' +
                                    ' configurations, because \'defer\'' +
                                    ' assumes a \'proxy\' to the LazyLoad' +
                                    ' mojit.');
                }
                // aha! that means we will give it a proxy to the LazyLoad
                // mojit, which will handle lazy execution on the client.
                child.proxy = {
                    type: 'LazyLoad'
                };
            }
            if (Y.Lang.isObject(child.proxy)) {
                // found a proxy, replace the child with the proxy and shove
                // the child to proxy into it
                child = child.proxy;
                child.proxied = originalChild;
                // remove any defer or proxy flags so it doesn't reload
                // infinitely
                originalChild.proxy = undefined;
                originalChild.defer = false;
            }

            // Make a new "command" that works in the context of this
            // composite
            newCommand = {
                instance: child,
                // use action in child spec or default to index
                action: child.action || 'index',
                context: this.command.context,
                params: child.params || this.command.params
            };

            // identifier for the child (only used in the logs)
            id = NAME + '::' + (newCommand.base ? '' : '@' + newCommand.type) + ':' + newCommand.action;

            childAdapter = new Y.mojito.OutputBuffer(id, this.queue.add(function (err, data, meta) {

                // HookSystem::StartBlock
                Y.mojito.hooks.hook('adapterBuffer', this.hook, 'end', this);
                // HookSystem::EndBlock

                if (err && originalChild.propagateFailure) {
                    my._onChildFailure(childName, err);
                    return;
                }

                // This ends up in my.queue.results array.
                return {
                    name: childName,
                    data: (data || ''),
                    meta: meta
                };

            }));

            // HookSystem::StartBlock
            Y.mojito.hooks.hook('adapterBuffer', this.adapter.hook, 'start', childAdapter);
            // HookSystem::EndBlock

            childAdapter = Y.mix(childAdapter, this.adapter);

            this.dispatch(newCommand, childAdapter);

            return child;
        },


        _onChildFailure: function (childName, err) {
            // error already reported by AdapterBuffer
            this.adapter.error('Failed composite because of first child failure of "' + childName + '"');
            // cleaning up the house
            this.queue.results = [];
            // cancel any pending job by invalidating the `execute` callback
            this.failed = true;
        }

    };

    Y.namespace('mojito.addons.ac').composite = Addon;

}, '0.1.0', {requires: [
    'parallel',
    'mojito',
    'mojito-util',
    'mojito-hooks',
    'mojito-output-buffer',
    'mojito-assets-addon'
]});
