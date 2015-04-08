
# Getting Started with Mojito, Part 5: Composite Mojits

<div id="top"></div>

($Revision$ $Date$)

<div style="margin:1em; padding:0.4em; border:1px solid #F86; color:#844; background-color:#FEE;">
    <strong>NOTE:</strong>
    If you haven't installed Mojito yet, you should go to <a href="https://developer.yahoo.com/cocktails/mojito/docs/quickstart/">part one of the Getting Started Guide</a> and do so!
</div>

In this Guide we'll create a page containing multiple mojits. One mojit will show a list of image thumbnails. The other will show details about the image chosen from the list.

Of course, if the user has javascript enabled we'll do a client-side refresh of the mojits. Otherwise, the user will fallback to a round trip to the server.


## Create a Mojito Application

We'll start by making a Mojito application. This should be familiar to you from the previous Getting Started Guides.

    $ mojito create app flickr-list
    $ cd flickr-list


## Create an Image List Mojit

Next we'll create a mojit which lists flickr images.
We'll re-use the Flickr mojit from the previous Getting Started Guide.

    $ mojito create mojit simple PagedFlickr
    $ mkdir mojits/PagedFlickr/lang
    $ mkdir mojits/PagedFlickr/assets

Edit `mojits/PagedFlickr/controller.server.js` to contain the following:

    YUI.add('PagedFlickr', function(Y, NAME) {

        var PAGESIZE = 6;

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
                        date: dateString,
                        greeting: ac.intl.lang("TITLE"),
                        prev: {
                            url: selfUrl(ac, 'flickr', { page: page-1 } ),
                            title: ac.intl.lang("PREV") || 'prev'
                        },
                        next: {
                            url: selfUrl(ac, 'flickr', { page: page+1 } ),
                            title: ac.intl.lang("NEXT") || 'next'
                        }
                    };

                    Y.Array.each(images, function(image) {
                        image.detail_url = '/';
                    }, this);
                    data.images = images;

                    if (page > 1) {
                        data.prev.url = selfUrl(ac, 'flickr', { page: page-1 });
                        data.has_prev = true;
                    }
                    ac.done(data);

                });
            }
        };

        function selfUrl(ac, mojitType, mods) {
            var params = Y.mojito.util.copy(ac.params.getFromMerged());
            for (var k in mods) {
                params[k] = mods[k];
            }
            return ac.url.make(mojitType, 'index', Y.QueryString.stringify(params));
        }

    }, '0.0.1', {requires: [
        'mojito-models-addon',
        'mojito-intl-addon',
        'mojito-util',
        'querystring-stringify',
        'ModelFlickr'
    ], lang: ['de', 'en-US']});


    

This new controller expects the model to provide the additional data of the flickr ID for each photo (see below).

And we are going to provide a shared model alongside the mojits directory at `models/flickr.server.js` This model also returns the Flickr IDs of each photo as a part of its data set:

    /*
     * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
     */
    YUI.add('ModelFlickr', function(Y, NAME) {

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
                        if (!rawYqlData || !rawYqlData.query || !rawYqlData.query.results) {
                            callback(rawYqlData);
                            return;
                        }
                        var rawPhotos = rawYqlData.query.results.photo,
                            rawPhoto = null,
                            photos = [],
                            photo = null,
                            i = 0;

                        for (; i < rawPhotos.length; i++) {
                            rawPhoto = rawPhotos[i];
                            photo = {
                                id: rawPhoto.id,
                                title: rawPhoto.title,
                                url: buildFlickrUrlFromRecord(rawPhoto)
                            };
                            // some flickr photos don't have titles, so force them
                            if (!photo.title) {
                                photo.title = "[" + queryString + "]";
                            }
                            photos.push(photo);
                        }
                        callback(null, photos);
                    });
            }

        };

        function buildFlickrUrlFromRecord(record) {
            return 'http://farm' + record.farm
                    + '.static.flickr.com/' + record.server
                    + '/' + record.id + '_' + record.secret + '.jpg';
        }

    }, '0.0.1', {
        requires: ['yql', 'jsonp-url']
    });

This shared model can be referenced from any mojit within this application.

> #### Internationalization
>You can use the same internationalization data as the previous Getting Started Guide, so we don't review that process here.

And here is the view, just like the last Getting Started Guide: `mojits/PagedFlickr/views/index.mu.html`:

    <div id="{{mojit_guid}}">
        <h2>{{ greeting }} - {{ date }}</h2>
        <ul class="pics">
            {{#images}}
                <li class="pic"><a href="{{detail_url}}"><img src="{{url}}" alt="{{title}}"/></a></li>
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

The only difference in the template above and the one for the previous guide is the `a href` attribute on the images, which now expects a `detail_url` string. We've created this in the controller above using the image ID given by the model.

Here's styling for our mojit, for `mojits/PagedFlickr/assets/index.css`:

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

If we name the css file `index.css` -- the same root name as the action it applies to, it will automatically be added to the page.

Here's our `routes.json`, to access our mojit:

    [
        {
            "settings": [ "master" ],

            "flickr_by_page": {
                "verb": ["get"],
                "path": "/flickr/page/:page",
                "call": "flickr.index"
            },

            "flickr_base": {
                "verb": ["get"],
                "path": "/flickr",
                "param": "page=1",
                "call": "flickr.index"
            }

        }
    ]

The last thing we'll need to do is update the `application.json` to register our mojit:

    [
        {
            "settings": [ "master" ],

            "specs": {
                "flickr": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "child": {
                            "type": "PagedFlickr"
                        }
                    }
                }
            }
        }
    ]

We are now roughly where we left off at the end of the Getting Started Guide part 4.

However, we'll want to change things around a little. We want a vertical list of thumbnails that doesn't take up too much space.

Here's an updated `mojits/PagedFlickr/assets/index.css`:

    .pics .pic img {
        height: 60px;
        width: 60px;
    }
    #paginate span { margin:1em; }
    
    ul.pics {
        list-style-type: none;
    }
    ul.pics .pic { 
        padding: 1px;
    }

We'll also want to update the links to show details about the clicked image. Our new model already provides the flickr image ID as a part of the dataset it returns, so all we need to do is update the controller to use the new image.id to create a URL that shows details of the chosen image.

The `detail_url` links don't do anything yet because Mojito doesn't yet know how to make "image detail" links.

Start the server and try it out at <http://localhost:8666/flickr>. We can now page through the thumbnails. The "image detail" URLs will all just be `/`. We'll implement that next.

## Make an Image Detail Mojit

Next we'll make an image detail mojit.

    $ mojito create mojit simple FlickrDetail
    $ mkdir mojits/FlickrDetail/lang
    $ mkdir mojits/FlickrDetail/assets

Before we get too far into implementing that mojit, lets stitch it into the application.
Here's an updated `application.json`:

    [
        {
            "settings": [ "master" ],
    
            "specs": {
                "flickr": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "child": {
                            "type": "PagedFlickr"
                        }
                    }
                },
    
                "detail": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "child": {
                            "type": "FlickrDetail"
                        }
                    }
                }
            }
        }
    ]

And here's an updated `routes.json`:

    [
        {
            "settings": [ "master" ],

            "flickr_by_page": {
                "verbs": ["get"],
                "path": "/flickr/page/:page",
                "call": "flickr.index"
            },

            "flickr_base": {
                "verbs": ["get"],
                "path": "/flickr",
                "param": "page=1",
                "call": "flickr.index"
            },

            "detail": {
                "verbs": ["get"],
                "path": "/detail/image/:image",
                "call": "detail.index"
            }

        }
    ]

Now that mojito knows about the FlickrDetail mojit and knows where to find it, it knows how to generate URLs for it. Start the server up and visit <http://localhost:8666/flickr>.

You should now be able to mouse-over the images and see URLs like `/detail/image/12345789`. Now we need to make those URLs work and do something.

Here's the `mojits/FlickrDetail/controller.js`:

    YUI.add('FlickrDetail', function(Y, NAME) {

        Y.mojito.controllers[NAME] = {

            index: function(ac) {

                var image = ac.params.getFromMerged('image') || '0';

                // a little paranoia about inputs
                if (!image.match(/^\d+$/)) {
                    ac.done({ type: 'error', message: ac.intl.lang('ERROR_BAD_IMAGE_ID') }, { name:'message' });
                    return;
                }

                if ('0' === image) {
                    ac.done({ type: 'info', message: ac.intl.lang('INFO_NO_IMAGE_CHOSEN') }, { name:'message' });
                    return;
                }

                ac.models.get('ModelFlickr').getFlickrDetail(image, function(err, details) {
                    if (err) {
                        ac.error(new Error("YQL Error"));
                        return;
                    }
                    //Y.log(details);
                    details.intl = {
                        DATE_POSTED:    ac.intl.lang('DATE_POSTED'),
                        TITLE:          ac.intl.lang('TITLE'),
                        TITLE_NONE:     ac.intl.lang('TITLE_NONE'),
                        DESCRIPTION:    ac.intl.lang('DESCRIPTION'),
                        DESCRIPTION_NONE: ac.intl.lang('DESCRIPTION_NONE'),
                        OWNER_USERNAME: ac.intl.lang('OWNER_USERNAME'),
                        TAGS:           ac.intl.lang('TAGS'),
                        TAGS_NONE:      ac.intl.lang('TAGS_NONE'),
                        URLS:           ac.intl.lang('URLS'),
                        URL_PHOTO_PAGE: ac.intl.lang('URL_PHOTO_PAGE'),
                        URL_IMAGE:      ac.intl.lang('URL_IMAGE')
                    };
                    details.intl.posted = ac.intl.formatDate(new Date(1000 * Number(details.dates.posted)));

                    // The mustache library we're using is a little finicky.
                    details.title = details.title || false;
                    if (details.title) {
                        details.have_title = true;
                    }
                    details.description = details.description || false;
                    if (details.description) {
                        details.have_description = true;
                    }
                    details.tags = details.tags || false;

                    ac.done(details);
                });
            }

        };

    }, '0.0.1', {requires: [
        'mojito-intl-addon',
        'mojito-models-addon',
        'ModelFlickr'
    ], lang: ['de', 'en-US']});

Because we are using a shared model, we can simply add a new function into our model called `getFlickrDetail`, which will query YQL for data. Edit `models/flickr.server.js` and add this function:

    getFlickrDetail: function(imageId, callback) {
        var q = 'select * from flickr.photos.info where photo_id="' + imageId + '"';
        Y.YQL(q, function(rawYqlData) {
            if (!rawYqlData || !rawYqlData.query || !rawYqlData.query.results) {
                callback("BAD YQL!");
                return;
            }
            var photo = rawYqlData.query.results.photo;
            photo.urls.image = {
                type: 'image',
                content: buildFlickrUrlFromRecord(photo)
            };
            callback(null, photo);
        });
    }

We'll also need our language files. Here's `mojits/FlickrDetail/lang/FlickrDetail_en-US.js`:

    YUI.add("lang/FlickrDetail_en-US", function(Y) {
    
        Y.Intl.add(
    
            "FlickrDetail", // associated module
            "en-US",        // BCP 47 language tag
    
            // key-value pairs for this module and language
            {
                INFO_NO_IMAGE_CHOSEN: "No image chosen.",
                ERROR_BAD_IMAGE_ID: "Error! Bad image ID.",
                ERROR_NO_DETAILS: "Failed to retrieve details for photo.",
                DATE_POSTED: "posted",
                TITLE: "title",
                TITLE_NONE: "none",
                DESCRIPTION: "description",
                DESCRIPTION_NONE: "none",
                OWNER_USERNAME: "username",
                TAGS: "tags",
                TAGS_NONE: "none",
                URLS: "urls",
                URL_PHOTO_PAGE: "page",
                URL_IMAGE: "image"
            }
        );
    }, "3.1.0", {requires: ['intl']});

And here's `mojits/FlickrDetail/lang/FlickrDetail_de.js`:

    YUI.add("lang/FlickrDetail_de", function(Y) {
    
        Y.Intl.add(
    
            "FlickrDetail", // associated module
            "de",           // BCP 47 language tag
    
            // key-value pairs for this module and language
            {
                INFO_NO_IMAGE_CHOSEN: "Bild nicht gewählt",
                ERROR_BAD_IMAGE_ID: "Fehler! schlechtes Image-Kennung.",
                ERROR_NO_DETAILS: "Wir konnten zu Informationen für Foto abzurufen.",
                DATE_POSTED: "Erstellungsdatum",
                TITLE: "Titel",
                TITLE_NONE: "kein",
                DESCRIPTION: "Beschreibung",
                DESCRIPTION_NONE: "keine",
                OWNER_USERNAME: "Benutzername",
                TAGS: "Begriffe",
                TAGS_NONE: "keine",
                URLS: "URLs",
                URL_PHOTO_PAGE: "Seite",
                URL_IMAGE: "Bild"
            }
        );
    }, "3.1.0", {requires: ['intl']});

Some styling would be nice. Here's `static/FlickrDetail/assets/index.css`:

    .FlickrDetail .img td {
        width: 400px;
        height: 400px;
        text-align: center;
        vertical-align: middle;
    }
    .FlickrDetail .img img {
        max-width: 400px;
        max-height: 400px;
    }
    .FlickrDetail th {
        padding: 0.2em 0.6em;
        text-align: right;
        vertical-align: top;
    }
    .FlickrDetail .title th {
        vertical-align: bottom;
    }
    .FlickrDetail .title td {
        font-size: 120%;
    }
    .FlickrDetail .title td ,
    .FlickrDetail .description td ,
    .FlickrDetail .tags td {
        max-width: 20em;
    }
    .FlickrDetail .none {
        font-style: italic;
        font-size: 80%;
        color: #888;
        padding-left: 0.6em;
    }
    .FlickrDetail.info {
        text-align: center;
        vertical-align: middle;
        padding: 1em;
        color: #666;
    }
    .FlickrDetail.error {
        text-align: center;
        vertical-align: middle;
        padding: 1em;
        color: #844;
    }

And finally we'll need our views.
Here's the standard one, for `mojits/FlickrDetail/views/index.mu.html`:

    <div id="{{mojit_guid}}" class="FlickrDetail">
    <table>
    <tr class="img">
        <td colspan="2">
            <img src="{{#urls}}{{#image}}{{{content}}}{{/image}}{{/urls}}" alt="{{title}}"/>
        </td>
    </tr>
    <tr class="title">
        <th>{{#intl}}{{TITLE}}{{/intl}}</th>
        <td>
            {{#have_title}}{{title}}{{/have_title}}
            {{^have_title}}{{#intl}}<span class="none">{{TITLE_NONE}}</span>{{/intl}}{{/have_title}}
        </td>
    </tr>
    <tr class="username">
        <th>{{#intl}}{{OWNER_USERNAME}}{{/intl}}</th>
        <td>{{#owner}}{{username}}{{/owner}}</td>
    </tr>
    <tr class="posted">
        <th>{{#intl}}{{DATE_POSTED}}{{/intl}}</th>
        <td>{{#intl}}{{posted}}{{/intl}}</td>
    </tr>
    <tr class="description">
        <th>{{#intl}}{{DESCRIPTION}}{{/intl}}</th>
        <td>
            {{#have_description}}{{description}}{{/have_description}}
            {{^have_description}}{{#intl}}<span class="none">{{DESCRIPTION_NONE}}</span>{{/intl}}{{/have_description}}
        </td>
    </tr>
    <tr class="tags">
        <th>{{#intl}}{{TAGS}}{{/intl}}</th>
        <td>
            {{#tags}}{{#tag}}{{content}} {{/tag}}{{/tags}}
            {{^tags}}{{#intl}}<span class="none">{{TAGS_NONE}}</span>{{/intl}}{{/tags}}
        </td>
    </tr>
    <tr class="urls">
        <th>{{#intl}}{{URLS}}{{/intl}}</th>
        <td>
            {{#urls}}{{#url}}<a href="{{{content}}}">{{#intl}}{{URL_PHOTO_PAGE}}{{/intl}}</a>{{/url}}{{/urls}}
            {{#urls}}{{#image}}<a href="{{{content}}}">{{#intl}}{{URL_IMAGE}}{{/intl}}</a>{{/image}}{{/urls}}
        </td>
    </tr>
    </table>
    </div>

We've also introduced a new template just for presenting simple messages. This goes in `mojits/FlickrDetail/views/message.mu.html`:

    <div id="{{mojit_guid}}" class="FlickrDetail {{type}}">
    {{message}}
    </div>

You can now start your server and visit <http://localhost:8666/flickr>. You can do the normal paging through images, and when you see an image you like you can click on it to see the new image detail page.

## Put the Two Mojits Together

Now that we have two separate mojits working, lets put them both together on the same page. In order to do this we'll need to create a mojit that contains sub-mojits.

    $ mojito create mojit simple FlickrBrowser
    $ mkdir mojits/FlickrBrowser/assets
    $ rm -rf mojits/FlickrBrowser/models/

We removed the models directory since we won't need it for this mojit.

Let's start with the mojit instance specification, since the hope is that it will drive most of what our "composing mojit" does.

Here is a new `application.json`:

    [
        {
            "settings": [ "master" ],

            "specs": {
                "flickr": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "child": {
                            "type": "FlickrBrowser",
                            "config": {
                                "children": {
                                    "thumbs": {
                                        "type": "PagedFlickr"
                                    },
                                    "detail": {
                                        "type": "FlickrDetail"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    ]

We did several things here. First, we have only one top-level mojit:  our HTMLFrameMojit to make the page. It has one child:  a composite mojit (which we're working on now). The composite mojit has two child mojits:  the paged thumbnail mojit and image detail mojit which we've already completed.

Lets take a look at a new `routes.json`:

    [
        {
            "settings": [ "master" ],

            "flickr_by_page": {
                "verbs": ["get"],
                "path": "/flickr/page/:page/image/:image",
                "call": "flickr.index"
            },

            "flickr_base": {
                "verbs": ["get"],
                "path": "/flickr",
                "param": "page=1&image=0",
                "call": "flickr.index"
            }

        }
    ]

Since we only have one page, it's gotten a little easier. We do still have two routes into the "flickr" mojit, one without arguments and another with. For the full path, we need to hold parameters for both the child mojits. The `page` parameter will be used by the PagedFlickr mojit, and the `image` parameter will be used by the FlickrDetail mojit.

So now we're ready to work on our FlickrBrowser mojit. Here's the controller at `mojits/FlickrBrowser/controller.server.js`:

    YUI.add('FlickrBrowser', function(Y) {

        Y.mojito.controller = {

            init: function(config) {
                this.config = config;
            },

            index: function(ac) {
                ac.composite.done();
            }

        };

    }, '0.0.1', {requires: []});


Is that really all?  Yup. In Mojito, we expect composite mojits to be written a lot, so the framework has provided a utility that helps manage children. The `ac.composite.run()` function looks for `children` and runs them. It calls `ac.done()` itself once all children are done.

Here's the normal view in `mojits/FlickrBrowser/views/index.mu.html`:

    <div id="{{mojit_guid}}" class="mojit FlickrBrowser">
    <table>
        <tr>
            <td>
                {{{thumbs}}}
            </td>
            <td>
                {{{detail}}}
            </td>
        </tr>
    </table>
    </div>

And now for some nice styling.
Since the child mojits have already styled themselves nicely, we just need to make some small adjustments.
Here's `mojits/FlickrBrowser/assets/index.css`:

    .FlickrBrowser td {
        vertical-align: top;
    }
    .FlickrBrowser .FlickrDetail {
        min-width: 200px;
    }

There's one more small change that we need to make. We need to update the PagedFlickr mojit to create thumbnail URLs to the "flickr" page instead of the "detail" page. Here is an updated `mojits/PagedFlickr/controller.server.js`:

    YUI.add('PagedFlickr', function(Y, NAME) {

        var PAGESIZE = 6;

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
                        date: dateString,
                        greeting: ac.intl.lang("TITLE"),
                        prev: {
                            url: selfUrl(ac, 'flickr', { page: page-1 } ),
                            title: ac.intl.lang("PREV") || 'prev'
                        },
                        next: {
                            url: selfUrl(ac, 'flickr', { page: page+1 } ),
                            title: ac.intl.lang("NEXT") || 'next'
                        }
                    };

                    Y.Array.each(images, function(image) {
                        image.detail_url = selfUrl(ac, 'flickr', { image: image.id });
                    }, this);
                    data.images = images;

                    if (page > 1) {
                        data.prev.url = selfUrl(ac, 'flickr', { page: page-1 });
                        data.has_prev = true;
                    }
                    ac.done(data);

                });
            }
        };

        function selfUrl(ac, mojitType, mods) {
            var params = Y.mojito.util.copy(ac.params.getFromMerged());
            console.log(params);
            for (var k in mods) {
                params[k] = mods[k];
            }
            return ac.url.make(mojitType, 'index', Y.QueryString.stringify(params));
        }

    }, '0.0.1', {requires: [
        'mojito-models-addon',
        'mojito-intl-addon',
        'mojito-util',
        'querystring-stringify',
        'ModelFlickr'
    ], lang: ['de', 'en-US']});

Start up the server again and visit <http://localhost:8666/flickr>. We should be able to page through the thumbs as before. Now, when we click on a thumb, it'll show details for that image. The choice is persisted as we continue paging.

## Making it Work on the Client

When the user clicks on a thumbnail, we don't really need to refresh the whole page. All we really need to refresh is the FlickrDetail mojit. Rebuilding only _part_ of the page is something that we can only do on the client.

The first thing that we need to do is embed Mojito into the generated page. This is easy:  just add `"deploy": true` to the specification for the HTMLFrameMojit. Here's an updated `application.json` that demonstrates that:

    [
        {
            "settings": [ "master" ],

            "specs": {
                "flickr": {
                    "type": "HTMLFrameMojit",
                    "config": {
                        "deploy": true,
                        "child": {
                            "type": "FlickrBrowser",
                            "config": {
                                "children": {
                                    "thumbs": {
                                        "type": "PagedFlickr"
                                    },
                                    "detail": {
                                        "type": "FlickrDetail"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    ]

We don't need to specify `deploy` for the composite or child mojits. If a mojit has a binder, it will automatically be sent to the client.

When we ran the `mojito create mojit` command to create our mojits, a `binders` directory and a default binder was create for each.

Here is `mojits/PagedFlickr/binders/binder.js`:

    YUI.add('PagedFlickrBinderIndex', function(Y, NAME) {

        Y.namespace('mojito.binders')[NAME] = {

            init: function(mojitProxy) {
                this.mojitProxy = mojitProxy;
            },

            bind: function(node) {
                node.all('.pic a').on('click', function(evt) {
                    var url = evt.currentTarget.get('href');
                    Y.log('on click ' + url, 'debug', NAME);
                    var matches = url.match(/image\/(\d+)/);
                    var imageID = matches[1];
                    if (imageID) {
                        evt.halt();

                        // Update our pagination links so when we round-trip back
                        // to the server, we persist the image choice.
                        node.all('#paginate a').each(function(pageLink) {
                            var pageUrl = pageLink.get('href');
                            pageLink.set('href', pageUrl.replace(/\/image\/\d+/, '/image/'+imageID));
                        });

                        Y.log('broadcast flickr-image-chosen ' + imageID, 'debug', NAME);
                        this.mojitProxy.broadcast('flickr-image-chosen', { id: imageID });
                    }
                }, this);
            }

        };

    }, '0.0.1', {requires: ['node', 'mojito-client']});


This binder waits for a user to click. It then generates a `flickr-image-chosen` event with the image ID. It also needs to update the `previous` and `next` links so that the image choice is maintained when the user clicks on those.

Here is `mojits/FlickrDetail/binders/binder.js`:

    YUI.add('FlickrDetailBinderIndex', function(Y, NAME) {

        Y.namespace('mojito.binders')[NAME] = {

            init: function(mojitProxy) {
                var self = this;
                this.mojitProxy = mojitProxy;
                this.mojitProxy.listen('flickr-image-detail', function(payload) {
                    Y.log('on flickr-image-detail ' + payload.data.id, 'debug', NAME);
                    var urlParams = Y.mojito.util.copy(mojitProxy.context);
                    var routeParams = {
                        image: payload.data.id
                    };
                    mojitProxy.invoke('index', {
                        params: {
                            url: urlParams,
                            route: routeParams
                        },
                        scope: this
                    }, function(err, markup) {
                        if (err) {
                            Y.log(err, 'error', NAME);
                        } else {
                            self.node.replace(markup);
                        }
                    });
                });
            },

            bind: function(node) {
                this.node = node;
            }

        };

    }, 'MOJIT_VERSION', {requires: ['node', 'mojito-client', 'mojito-util']});

This listens for the `flickr-image-detail` event. When it arrives, it asks mojito to run the `index` action on the controller. The results of that action are then substituted in for the old contents of the mojit.

Here is `mojits/FlickrBrowser/binders/binder.js`:

    YUI.add('FlickrBrowserBinderIndex', function(Y, NAME) {

        Y.namespace('mojito.binders')[NAME] = {

            init: function(mojitProxy) {
                this.mojitProxy = mojitProxy;
                this.mojitProxy.listen('flickr-image-chosen', function(event) {
                    Y.log('on flickr-image-chosen ' + event.data.id, 'debug', NAME);
                    // Turn the event generated by PagedFlickr into something
                    // understood by FlickrDetail.
                    mojitProxy.broadcast('flickr-image-detail', { id: event.data.id });
                });
            }

        };

    }, '0.0.1', {requires: ['node', 'mojito-client']});


Since each child mojit is dealing with a different type of event, the composite mojit needs to translate between the two.

We could have made a single event that both PagedFlickr and FlickrDetail understood. However, we want to write those two mojits in a way that is internally consistent for each. That way then can be re-used on different pages. Writing the event passing binder was hopefully simple enough.

There's one more thing that we need to do. Since the client-side part of Mojito will be calling back to the server to load just the FlickrDetail mojit, we need to provide a route for just that mojit.
Here's an updated `routes.json`:

    [
        {
            "settings": [ "master" ],
    
            "flickr_by_page": {
                "verbs": ["get"],
                "path": "/flickr/page/:page/image/:image",
                "call": "flickr.index"
            },

            "flickr_base": {
                "verbs": ["get"],
                "path": "/flickr",
                "param": "page=1&image=0",
                "call": "flickr.index"
            }
    
        }
    ]

### Running Completely Client-Side

Because all the controllers and models in this example don't require server-specific technologies, we can safely deploy them all to the client and run them within the browser. To do this, simple change all the controller and model filenames to `*.common.js`. For example, change `mojits/FlickrDetail/controller.server.js` to `mojits.FlickrDetail/controller.common.js`, specifying that this code can run either on the server or the client.

Now all YQL calls will be made from within the browser.

## In Summary

By creating a mojit made up of multiple child mojits much more interesting pages can be made.
This nesting can go arbitrarily deep.

Mojito ships with utilities that help you write your own composite mojits.
This gives you full control over how those child mojits are put together since you provide the templates, css, and javascript (binders) to manage their composition and interaction.



<hr/>
<div class="paginate"><a href="/tutorials.GettingStarted-Part4">Part 4</a></div>
<div align="center"><a href="#top">top</a></div>
