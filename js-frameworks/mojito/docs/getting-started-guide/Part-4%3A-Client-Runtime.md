
# Getting Started with Mojito, Part 4: Client Runtime
<div id="top"></div>

($Revision$ $Date$)

<div style="margin:1em; padding:0.4em; border:1px solid #F86; color:#844; background-color:#FEE;">
    <strong>NOTE:</strong>
    If you haven't installed Mojito yet, you should start with <em>part one</em> of the Getting Started Guide.
</div>

In this Guide we'll create a mojit that dynamically interacts with the user. We'll do this in a way that works client-side if the user has javascript enable, and falls back to server-side on less capable devices.

We will once again start with the end result of the previous guide as our starting point.

## Adding Pagination Links to the View

Let's start by adding room within our current view template for `prev` and `next` links that will drive our pagination.

    <div id="{{mojit_guid}}">
        <h2>{{ greeting }} - {{ date }}</h2>
        <ul class="pics">
            {{#images}}
                <li class="pic"><a href="{{url}}"><img src="{{url}}" alt="{{title}}"/></a></li>
            {{/images}}
        </ul>
        <div id="paginate">
            <span>
                {{#prev}}
                    <a href="{{{url}}}">{{title}}</a>
                {{/prev}}
            </span>
            <span>
                {{#next}}
                    <a href="{{{url}}}">{{title}}</a>
                {{/next}}
            </span>
        </div>
    </div>

We now expect two additional template values: `prev` and `next`, which should both have values for `url` and `title`.

We can also update our controller to provide some mock values now just to ensure this is working:

    YUI.add('Flickr', function(Y, NAME) {

        Y.mojito.controllers[NAME] = {

            index: function(ac) {
                ac.models.get('ModelFlickr').getFlickrImages('mojito', function(images) {
                    var dateString = ac.intl.formatDate(new Date());
                    data = {
                        images: images,
                        date: dateString,
                        greeting: ac.intl.lang("TITLE") || 'title',
                        prev: {
                            url: '#',
                            title: ac.intl.lang("PREV") || 'prev'
                        },
                        next: {
                            url: '#',
                            title: ac.intl.lang("NEXT") || 'next'
                        }
                    };
                    ac.done(data);
                });

            }

        };

    }, '0.0.1',  {requires: [
        'mojito-intl-addon',
        'mojito-models-addon',
        'ModelFlickr'
    ], lang: ['de', 'en-US']});

In order for the proper internationalization features to work, we also need to add the language files in a `lang` directory, just like we did in the previous guide:

`mojits/Flickr/lang/Flickr_en-US.js`

    YUI.add("lang/Flickr_en-US", function(Y) {

        Y.Intl.add(

            "Flickr",  // associated module
            "en-US",   // BCP 47 language tag

            // key-value pairs for this module and language
            {
                TITLE: "Enjoy your Flickr Images!",
                PREV: "previous",
                NEXT: "next"
            }
        );
    }, "3.1.0", {requires: ['intl']});

`mojits/Flickr/lang/Flickr_de.js`

    YUI.add("lang/Flickr_de", function(Y) {

        Y.Intl.add(

            "Flickr",  // associated module
            "de",      // BCP 47 language tag

            // key-value pairs for this module and language
            {
                TITLE: "Hallo! genießen Sie Ihre Bilder",
                PREV: "zurück",
                NEXT: "weiter"
            }
        );
    }, "3.1.0", {requires: ['intl']});

Notice that we've also added language translations for `PREV` and `NEXT`, which will be our pagination control labels.

Now if we run `mojito start`, we get all these mojito pics on displaying on the same page. If you point your browser to [http://localhost:8666/flickr?lang=de](http://localhost:8666/flickr?lang=de), you should see a German page, complete with "zurück" and "weiter" for the previous and next links. (These links don't do anything just yet.)

## Running Your Mojit on the Client

If we want to do client-side pagination, we need to enable Mojito to run within the browser, therefore letting your mojit code also run in the browser. Here are the steps you'll need to take to do so. By default, Mojito will not serve full HTML pages for each mojit it serves. You must decide which mojits represent an entire HTML document and wrap them within an `HTMLFrameMojit`.

### Assets and the HTMLFrameMojit

In order for Mojito to put itself on the page, it needs a way of adding its javascript to the generated output. We accomplish this with the HTMLFrameMojit which ships with Mojito.

The HTMLFrameMojit wraps another mojit and provides the HTML frame around it. The HTML includes places to put the assets (javascript and css). Since it's a generic mojit meant to be used with many other mojits, its controller is fixed. We modify its behavior by changing its configuration.

You can make the following change to `application.json` to wrap our Flickr mojit:

    [
        {
            "settings": [ "master" ],

            "specs": {
                "flickr": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "deploy": true,
                        "child": {
                            "type": "Flickr"
                        }
                    }
                }
            }
        }
    ]

It's as simple as that. The mojit we've been calling "flickr" is moved to a child of HTMLFrameMojit, which now takes the name of "flickr". If you restart the server, reload the page, and view the source you'll see a bunch of javascript added to the bottom of the page. This is the Mojito client-side deploying itself into the page.

Since we can now add assets to the page, lets add our own CSS to make things look a little nicer. Create a `mojits/Flickr/assets/css/style.css` file with the following contents:

    .pics .pic img {
        height: 200px;
        width: 200px;
    }
    #paginate span { margin:1em 3em; }

    ul.pics {
        width: 700px;
        height: 660px;
        list-style-type: none;
    }
    ul.pics .pic {
        float: left; padding: 5px;
    }

Now we need to tell Mojito to include this CSS in our page. There are two ways of doing this: `ac.addAsset()` can be called from the controller, or an "assets" section can be added to the HTMLFrameMojit's configuration.

Here's the second (simpler) approach:

    [
        {
            "settings": [ "master" ],

            "specs": {
                "flickr": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "deploy": true,
                        "child": {
                            "type": "Flickr"
                        },
                        "assets": {
                            "top": {
                                "css": [
                                    "/static/Flickr/assets/css/style.css"
                                ]
                            }
                        }
                    }
                }
            }
        }
    ]

The "top" property describes any assets that should be added to the top of the page, within the `HEAD` element (as opposed to "bottom", which is not shown here). You can also add "js" assets here for libraries you whould like to include with your mojit.

Restart your server and reload the page. You should now see more nicely styled output.

## Making "Previous" and "Next" Links Work

In the files above, we've already created the workings for "previous" and "next" links within our language files and the view. Now we need to create a model that is pageable.

Let's start by modifying the model to take "start" and "count" parameters:

`mojits/Flickr/models/model.server.js`:

    YUI.add('FlickrModel', function(Y, NAME) {

        Y.mojito.models[NAME] = {

            getFlickrImages: function(queryString, start, count, callback) {
                var q;
                start = parseInt(start) || 0;
                count = parseInt(count) || 10;
                // The YQL docs say that the second number is the end, but in practice
                // it appears to be the count.
                // https://developer.yahoo.com/yql/guide/paging.html#remote_limits
                q = 'select * from flickr.photos.search(' + start + ',' + count + ') where text="' + queryString + '"';
                Y.YQL(q, function(rawYqlData) {
                    Y.log(rawYqlData);
                    if (!rawYqlData.query.results) {
                        callback("No YQL results!");
                        return;
                    }
                    var rawPhotos = rawYqlData.query.results.photo,
                        rawPhoto = null,
                        photos = [],
                        photo = null,
                        i = 0;

                    // Sometimes YQL returns more than the requested amount.
                    rawPhotos.splice(count);

                    for (; i<rawPhotos.length; i++) {
                        rawPhoto = rawPhotos[i];
                        photo = {
                            title: rawPhoto.title,
                            url: buildFlickrUrlFromRecord(rawPhoto)
                        };
                        // some flickr photos don't have titles, so force them
                        if (!photo.title) {
                            photo.title = "[" + queryString + "]";
                        }
                        photos.push(photo);
                    }
                    Y.log('calling callback with photos');
                    Y.log(photos);
                    callback(null, photos);
                });
            }

        };

        function buildFlickrUrlFromRecord(record) {
            return 'http://farm' + record.farm
                + '.static.flickr.com/' + record.server
                + '/' + record.id + '_' + record.secret + '.jpg';
        }

    }, '0.0.1', {requires: ['yql', 'jsonp-url']});

For the controller, we'll make things a little simpler. We'll just expose a "page" parameter and use that to move to the "previous" and "next" pages. We'll fix the page size to 9 images. (The Flickr YQL query returns 10 by default, so we'll use a slightly different number to show that the "count" parameter is working.)

`mojits/Flickr/controller.server.js`:

    YUI.add('Flickr', function(Y, NAME) {

        var PAGESIZE = 9;

        Y.mojito.controllers[NAME] = {

            index: function(ac) {
                var page = ac.params.getFromMerged('page'),
                    start;

                // a little paranoia about inputs
                page = parseInt(page, 10);
                if ((!page) || (page < 1)) {
                    page = 1;
                }

                // The "page" parameter is base-1, but the model's "start"
                // parameter is base-0.
                start = (page-1) * PAGESIZE;

                ac.models.get('ModelFlickr').getFlickrImages('mojito', start, PAGESIZE, function(err, images) {

                    var dateString, data;

                    // on model error, fail fast
                    if (err) {
                        return ac.error(err);
                    }

                    dateString = ac.intl.formatDate(new Date());
                    data = {
                        images: images,
                        date: dateString,
                        greeting: ac.intl.lang("TITLE") || 'title',
                        prev: {
                            url: selfUrl(ac, { page: page-1 }),
                            title: ac.intl.lang("PREV") || 'prev'
                        },
                        next: {
                            url: selfUrl(ac, { page: page+1 }),
                            title: ac.intl.lang("NEXT") || 'next'
                        }
                    };

                    ac.done(data);
                
                });
            }
        };
    
       function selfUrl(ac, mods) {
           // No real link for pages before page 1
           if (mods.page < 1) { return '#'; }
           var params = ac.params.getFromMerged();
           for (var k in mods) {
               params[k] = mods[k];
           }
           return ac.url.make('flickr', 'index', Y.QueryString.stringify(params));
        }


    }, '0.0.1', {requires: [
        'mojito-intl-addon',
        'mojito-models-addon',
        'ModelFlickr'
    ], lang: ['de', 'en-US']});

The controller is now providing the proper url string for the `previous` and `next` links to the view template. Inside the `selfUrl()` function, you can see we're calling `ac.url.make()` to create each URL. This [make](https://developer.yahoo.com/cocktails/mojito/api/Url.common.html#method_make) function is provided by an [**Action Context Addon**](https://developer.yahoo.com/cocktails/mojito/api/module_actioncontextaddon.html) called [`url`](https://developer.yahoo.com/cocktails/mojito/api/Url.common.html). It accepts the 'base' and 'action' of the mojit to route to, as well as stringified parameters to use to generate the route. In this case, we're making sure the current URL parameters are retained, except for the `page` parameter, which is overridden with the page we want to navigate to on click.

Now restart your server and point your browser again to [http://localhost:8666/flickr](http://localhost:8666/flickr). Now the links are working.

## Enabling AJAX Pagination

So far, we've only provide pagination by navigating to a new URL, but we want to be able to paginate without changing pages, just by doing an AJAX call to re-populate the Flickr view.

### Create a "base" Flickr mojit spec

In order to make the Flickr mojit available to be execute via a Remote Procedure Call (RPC), we need to make sure that mojit is defined as a root-level mojit in the application configuration. This allows us to specify a configuration for it that will always apply to this mojit when it is executed remotely. Update the `application.json` file to pull the child mojit out into a "base" mojit:

    [
        {
            "settings": [ "master" ],

            "specs": {
                "flickr": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "deploy": true,
                        "child": {
                            "base": "paged-flickr"
                        },
                        "assets": {
                            "top": {
                                "css": [
                                    "/static/Flickr/assets/css/style.css"
                                ]
                            }
                        }
                    }
                },

                "paged-flickr": {
                    "type": "Flickr"
                }
            }
        }
    ]

Now that we've pulled this mojit out into a root-spec, we can invoke it from the client. But to do that, first we'll need to update the binder in `mojits/Flickr/binders.index.js`. Any binders found in a mojit binder directory are _always_ deployed to the client along with the Mojito client runtime. This is where you can write code against the DOM for each mojit. In our case, we want to write an event handler that intercepts clicks on the "previous" and "next" navigation links. Our binder below has `init` and `bind` functions. Mojito passes a "mojit proxy" into each binder, then if it finds a DOM node on the page that is associated with the binder, it also calls the `bind` function and passes in a [Y.Node](https://developer.yahoo.com/yui/3/api/Node.html) object.

`mojits/Flickr/binders/index.js`:

    YUI.add('FlickrBinderIndex', function(Y, NAME) {

        function getPage(href) {
            return href.split('/').pop().split('=').pop()
        }

        Y.namespace('mojito.binders')[NAME] = {

            init: function(mojitProxy) {
                this.mojitProxy = mojitProxy;
            },

            bind: function(node) {
                var self = this;
                this.node = node;
                var paginator = function(evt) {
                    var tgt = evt.target;
                    var page = getPage(tgt.get('href'));

                    var replaceDOM = function(err, markup) {
                        self.node.replace(markup);
                    };

                    evt.halt();

                    self.mojitProxy.invoke('index', {
                        params: {
                            route: {page: page}
                        }
                    }, replaceDOM);

                };
                this.node.all('#paginate a').on('click', paginator, this);
            }

        };

    }, '0.0.1', {requires: ['mojito-client']});

When we receive the Y.Node object in the `bind` function above, we attach an event handler whenever one of the pagination links is clicked that calls the `paginator` function above. We halt the event and instead invoke the `index` action through the mojit proxy to re-render our current DOM node. In order to specify what page we want to show, we inspect the `href` attribute of the clicked link to get the page we want to navigate towards, then pass it along with the action invocation as a route parameter.

Because both the controller and model of our Flickr mojit are restricted to the server at this point (due to their file names: "controller.**server**.js" and "model.**server**.js"), those YUI modules are not available within the client runtime, so Mojito makes an AJAX call to the server to execute the Flickr index action and re-render a new view for a different page.

Now restart your server and point your browser again to [http://localhost:8666/flickr](http://localhost:8666/flickr). Now the links are working entirely from the client runtime, and no page refreshes are needed for pagination.

## Running the Controller and Model on the Browser

The only reason Mojito is making an RPC call for the mojit's invocation is because the Flickr controller and model have a **server affinity**. This means they can only run on the server. Mojit components can have a **server**, **client**, or **common** affinity, which is expressed as a part of its filename. In order to allow our Flickr mojit to execute on the browser, we simply change the name two files:

    $ cd mojit/Flickr
    $ mv controller.server.js controller.common.js
    $ cd models
    $ mv model.server.js model.common.js

Now, restart your server, go to [http://localhost:8666/flickr](http://localhost:8666/flickr), then click through the "next" and "previous" links. You'll notice from the server logs that each pagination is now occurring entirely on the client.

## In Summary

Modifying the mojit to have pages was hopefully fairly straightforward. This server-side implementation becomes the fallback for less capable devices once we get things running client-side.

Enabling client-side execution was also hopefully straightforward. The only new code was related to handling dynamic user events -- something that doesn't exist server-side.

<hr/>
<div class="paginate right"><a href="/tutorials.GettingStarted-Part5">Part 5</a></div>
<div class="paginate"><a href="/tutorials.GettingStarted-Part3">Part 3</a></div>
<div align="center"><a href="#top">top</a></div>
