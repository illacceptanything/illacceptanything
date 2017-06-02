/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true*/
/*global YUI*/


YUI.add('read-binder-index', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mp = mojitProxy;
            this.spaceid = this.mp.config.spaceId;
        },

        /**
         * Setup client-side event handlers.
         * @param {Node} mojitNode The node for the mojit's outer div.
         */
        bind: function(mojitNode) {
            var dotnode = mojitNode.one('div.nav'),
                dotnodes = dotnode.all('span'),
                viewnode = mojitNode.one('.main-sv'),
                viewnodes = viewnode.all('li.item-sv'),
                iframe = mojitNode.one('div.iframe'),
                // TODO: JSLint won't let this pass.
                //start = ~~this.mp.getFromUrl('start'),
                start = parseInt(this.mp.getFromUrl('start'), 10) || 0,
                self = this;

            // Remove the nav links, since presumably we have js at this point.
            mojitNode.all('a.link-prev, a.link-next').addClass('hidden');

            // Setup scrollview.
            // @see http://yuilibrary.com/yui/docs/api/classes/ScrollView.html
            Y.use('scrollview', 'scrollview-paginator', function(YY) {
                var scrollview = new YY.ScrollView({
                        srcNode: viewnode, // <ul> container for all stories.
                        height: 748,
                        width: 1023,
                        flick: {
                            preventDefault: function(e) {
                                return YY.UA.gecko; // Prevent gecko drag.
                            }
                        },
                        plugins: {
                            fn: YY.Plugin.ScrollViewPaginator, // Snap to.
                            cfg: {selector: 'li.item-sv'} // Story container.
                        },
                        render: true
                    });

                // When scrollview pages change, we update the dots.
                scrollview.pages.on('indexChange', function updateDots(ev) {
                    dotnodes.item(ev.prevVal).removeClass('selected');
                    dotnodes.item(ev.newVal).addClass('selected');
                });

                // Pick a page.
                scrollview.pages.set('index', start);
            });

            // Show all dots.
            dotnode.removeClass('hidden');

            // Remove spinner, all content is loaded.
            mojitNode.one('#spinner').remove();

            this.down(viewnode.all('div.url-nav a'), iframe);
            this.up(mojitNode.one('div.iframe-nav'), iframe);
        },

        /**
         * Animate the source webpage iframe sliding down.
         * @param {NodeList} nodes List of nodes to watch for click.
         * @param {Node} iframe The iframe to animate.
         */
        down: function(nodes, iframe) {
            nodes.on('click', function(ev) {
                var src = ev.currentTarget.get('href'),
                    anim = new Y.Anim({
                        node: iframe,
                        from: {top: 748},
                        to: {top: -2}
                    });

                ev.preventDefault();
                iframe.removeClass('hidden');

                anim.set('easing', Y.Easing.easeOut);
                anim.on('end', function() {
                    iframe.appendChild('<iframe src="' + src + '"></iframe>');
                });
                anim.run();
            });
        },

        /**
         * Animate the source webpage iframe sliding up.
         * @param {Node} node The node to watch for click.
         * @param {Node} iframe The iframe to animate up.
         */
        up: function(node, iframe) {
            node.on('click', function(ev) {
                var anim = new Y.Anim({
                        node: iframe,
                        from: {top: -2},
                        to: {top: 748}
                    });

                ev.preventDefault();
                iframe.one('iframe').remove();

                anim.set('easing', Y.Easing.easeOut);
                anim.on('end', function() {
                    iframe.addClass('hidden');
                });
                anim.run();
            });
        }
    };

}, '0.0.1', {requires: [
    'anim',
    'node-event-delegate',
    'scrollview',
    'scrollview-paginator'
]});
