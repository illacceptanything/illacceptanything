
# Getting Started with Mojito, Part 2: YQL Data Source

<div id="top"></div>
($Revision$ $Date$)

<div style="margin:1em; padding:0.4em; border:1px solid #F86; color:#844; background-color:#FEE;">
    <strong>NOTE:</strong>
    If you haven't installed Mojito yet, you should start with <em>part one</em> of the Getting Started Guide.
</div>

## YQL

The following tutorial will guide you through the process of creating a mojit that displays Flickr images with YQL.

#### Create a new Mojito application

    $ mojito create app simple basic-yql

You should see some output on the screen that is similar to:

    creating app called 'basic_yql'
    (using "simple" archetype)
    ✔ app: basic_yql created!

Now you have a directory in front of you called `basic_yql`, which is a runnable Mojito application. Sure, it's bare-bones, but we're going to change all that by creating a new mojit to run within it. We don't want to create anything complicated, so we'll tell Mojito to build us out a mojit called "Flickr".

Any invalid characters (like `-`) will be converted to an underscore character.

### Create a new Flickr mojit

    $ cd basic_yql
    $ mojito create mojit simple Flickr

The screen should output something like this:

    creating mojit called 'Flickr'
    (using "simple" archetype)
    ✔ mojit: Flickr created!
    ✔ mojito done.
    
Now if you look into the `mojits` directory of your new `basic_yql` application, you'll see that there is a directory there called `Flickr`. Inside it, there are some files that are used to define this mojit. Let's look into the `models` directory.

#### Model

It looks like there some canned function called `getData`, so let's just remove that generic function and create a new one.

`mojits/Flickr/models/model.server.js`:

    YUI.add('FlickrModel', function(Y, NAME) {

        Y.mojito.models[NAME] = {

            init: function(config) {
                this.config = config;
            },

            getFlickrImages: function(queryString, callback) {
                var q = 'select * from flickr.photos.search where text="%' + queryString + '%"';
                Y.YQL(q, function(rawYqlData) {
                    Y.log(rawYqlData);
                    var rawPhotos = rawYqlData.query.results.photo,
                        rawPhoto = null
                        photos = [],
                        photo = null,
                        i = 0;

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
                    callback(photos);

                });
            }

        };

        function buildFlickrUrlFromRecord(record) {
            return 'http://farm' + record.farm
                + '.static.flickr.com/' + record.server
                + '/' + record.id + '_' + record.secret + '.jpg';
        }

    }, '0.0.1', {requires: ['yql']});

Notice that I've added `'yql'` to the `requires` array at the bottom of the YUI module declaration.

We are doing some processing and creating our own array of photo objects for the controller before calling the callback with the data. I've added a utility function to build up an image URL from the raw record, and then once all the photos are processed, the new photos array is sent to the callback.

#### Controller

Let's take a quick look at the controller at `mojits/Flickr/controller.server.js`:

    YUI.add('Flickr', function(Y, NAME) {

        Y.mojito.controllers[NAME] = {

            init: function(config) {
                this.config = config;
            },

            index: function(ac) {
                ac.models.get('ModelFlickr').getFlickrImages('mojito', function(images) {
                    ac.done({images: images});
                });
            }

        };

    }, '0.0.1', {requires: [
        'mojito-models-addon',
        'ModelFlickr'
    ]});

Our controller is getting data from its model and passing it back into the Mojito framework via the **Action Context**.

#### The Action Context

The Action Context is the API for Mojito that is provided to a controller action. It is the main point of integration between your mojits and the Mojito framework. This is how you pass data from mojits into the framework and specify how that data should be handled.

The Action Context is an abstraction that allows you to write code that can run either on the server or within the browser. If you would like to access server specific features (like the HTTP Request object itself), you can specify this using _Addons_. **Action Context Addons** will be described elsewhere.

#### Using views

The controller we were working with has a function called `index`, this is called an **action** in Mojito-speak. Each action might have a view template to match the data it outputs in the `views` directory. When we created our mojit using the Mojito command line tool, an `index.mu.html` file was created in the `views` directory. When the controller passed the flickr images into the action context (`ac.done(images)`) above, Mojito took that as a request to pass this data through a template and render it into HTML. Mojito's convention is to look for a file matching the action being executed, so because we were executing the `index` action, Mojito will attempt to pass that data through a template called `index.mu.html` in the views directory.

Open up the `index.mu.html` file, which is a [Mustache](http://mustache.github.com/) template, and let's fill it in so it generates markup.

    <div id="{{mojit_guid}}">
    <h2>Hello, world!</h2>
    <ul>
        {{#images}}
            <li><a href="{{url}}">{{title}}</a></li>
        {{/images}}
    </ul>
    </div>

#### Configure Mojito Startup

There is a mojit type set up to be used, but the Mojito application has not been configured to use any mojits within the application. Open the application.json file and make it look like this:

    [
        {

            "settings" : ["master"],

            "specs": {
                "flickr": {
                    "type": "Flickr"
                }
            }

        }
    ]

We have defined one mojit to be available of the type `Flickr`. We can access this mojit through the default routing at `/flickr/index`, which executes the mojit defined as "flickr" in the application.json's `specs` configuration.

Now point your browser to [http://localhost:8666/flickr/index](http://localhost:8666/flickr/index).

#### Configure a route

Within this state, the only URL that applies to this application is [http://localhost:8666/flickr/index](http://localhost:8666/flickr/index). You can provide further configuration to open up further URLs to execute this mojit. To do so, create a `routes.json` file alongside the `application.json` file.

    [{
        "settings": [ "master" ],

        "route one": {
            "verbs": [ "get" ],
            "path": "/flickr",
            "call": "flickr.index"
        },

        "route two": {
            "verbs": [ "get" ],
            "path": "/booyah",
            "call": "flickr.index"
        },

        "route three": {
            "verbs": [ "get" ],
            "path": "/pineapples",
            "call": "flickr.index"
        }

    }]

Each route will be checked in the order specified for a match against the URL being evaluated. This new routing table provides us with two further URLs that will execute the `flickr` mojit's `index` action. If you start up your application, and you can see your mojit running on the following URLs:

*   [http://localhost:8666/flickr](http://localhost:8666/flickr)
*   [http://localhost:8666/booyah](http://localhost:8666/booyah)
*   [http://localhost:8666/pineapples](http://localhost:8666/pineapples)

More details on URL routing are available later in the Getting Started Guide.

> ###A WORD ON BLOCKING
> If you are new to the NodeJS world, you should take these words to heart: <strong>DON'T WRITE BLOCKING CODE</strong>. Our model needs to be asynchronous in order to allow the rest of Mojito execution to continue, so we cannot call any model functions synchronously. We must call them with a callback function to be executed whenever the model gets around to receiving its data. We have no idea where the model is getting its data, but we have to assume that it may block.

## In Summary

This has been a very simple use-case of Mojito. There are many ways you can configure your application without ever touching the `server.js` file, and we'll go into those more advanced cases in further editions of the Mojito Getting Started Guide.

<hr/>
<div class="paginate right"><a href="/tutorials.GettingStarted-Part3">Part 3</a></div>
<div class="paginate"><a href="/tutorials.GettingStarted-Part1">Part 1</a></div>
<div align="center"><a href="#top">top</a></div>
