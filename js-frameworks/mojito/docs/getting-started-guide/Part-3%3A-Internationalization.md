
# Getting Started with Mojito, Part 3: Internationalization

<div id="top"></div>

($Revision$ $Date$)

<div style="margin:1em; padding:0.4em; border:1px solid #F86; color:#844; background-color:#FEE;">
    <strong>NOTE:</strong>
    If you haven't installed Mojito yet, you should start with <em>part one</em> of the Getting Started Guide.
</div>

## Internationalization and Localized Templates

This tutorial will continue to build upon the previous example to display a list of Flickr images from YQL. In order to follow along, you must have the working example from Part 2 of this guide. The difference will be we are going to use YUI3 Internationalization features to support localizing your templates. The YUI Internationalization utility supports externalization, that is, separating data that needs to change for different languages or markets from the code of a software product, so that the same code can be used worldwide.

#### Add Language Modules

Using localized languages requires a little more work. In our mojit directory, we first create our language modules, and to specify which BCP 47 language tags we support.

    $ pushd mojits/Flickr
    $ mkdir lang
    $ cd lang

In the `mojits/Flickr/lang` directory, you will create a *Resource Bundle* specifying the keys and values for the strings that need localized. The "key" is the identifier within the language file used to access the resource (ie `actionContext.lang("TITLE")`, and the value is what will be provided in return for the given locale. The locale is pulled from the file based on the file name. So, we need to create and name a file for each locale we plan to support. Let's support German and English, which will be our default.

We create two files:  `Flickr.js` and `Flickr_de.js`.

The file names and contents are as follows, use your favorite editor to create both:

`mojits/Flickr/lang/Flickr.js`:

    YUI.add("lang/Flickr", function(Y) {

        Y.Intl.add(

            "Flickr",   // associated module name
            "en",    // BCP 47 language tag

            // key-value pairs for this module and language
            {
                TITLE: "Enjoy your Flickr Images!"
            }
        );

    }, "3.1.0", {requires: ['intl']});

`mojits/Flickr/lang/Flickr_de.js`:

    YUI.add("lang/Flickr_de", function(Y) {

        Y.Intl.add(

            "Flickr",   // associated module
            "de",       // BCP 47 language tag

            // key-value pairs for this module and language
            {
                TITLE: "Hallo! genießen Sie Ihre Bilder"
            }
        );

    }, "3.1.0", {requires: ['intl']});

By creating the above files you have told Mojito that the Flickr mojit supports both English and German translations.

#### Implement our Localized Controller

YUI3 provides support for Internationalization. Our actionContext provides access to our language modules and the current language. We will use that language to correctly format our date, and access the localized templates to feed our views with the properly localized strings from our resource bundles. What's new here is the inclusion of the YUI3 intl and datatype-date-format libraries, as well as, accessing the actionContext to get our lang setting (to use for the date) and our resource bundle strings.

Here is our updated controller:

`mojits/Flickr/controller.server.js`:

    YUI.add('Flickr', function(Y, NAME) {

        Y.mojito.controllers[NAME] = {

            index: function(ac) {
                ac.models.get('ModelFlickr').getFlickrImages('mojito', function(images) {
                    var dateString = ac.intl.formatDate(new Date());
                    var data = {
                        images: images,
                        date: dateString,
                        greeting: ac.intl.lang("TITLE"),
                        url: ac.url.make('flickr','index')
                    };
                    ac.done(data);
                });
            }

        };

    }, '0.0.1',  {requires: [
        'mojito-intl-addon',
        'mojito-models-addon',
        'ModelFlickr'
    ]});

#### Using Different Views

We can now use our internationalized strings and dates in our template. Open up the `index.mu.html` file, which is a [Mustache](http://mustache.github.com/) template, and create the following:

`mojits/Flickr/views/index.mu.html`:

    <div id="{{mojit_guid}}">
    <h2>{{ greeting }} - {{ date }} </h2>
    <h3>Requested URL: {{ url}} </h3>
    <ul>
        {{#images}}
            <li><a href="{{url}}">{{title}}</a></li>
        {{/images}}
    <ul>
    </div>


#### Start the Server

    $ mojito start

Now point your browser to [http://localhost:8666/flickr](http://localhost:8666/flickr). By default the language is English. To see the German version of this page, point your browser to [http://localhost:8666/flickr?lang=de](http://localhost:8666/flickr?lang=de).

## In Summary

This example shows how to localize your strings using YRB, and the YUI3 intl library.

[YUI3 intl](https://developer.yahoo.com/yui/3/intl/)

<hr/>
<div class="paginate right"><a href="/tutorials.GettingStarted-Part4">Part 4</a></div>
<div class="paginate"><a href="/tutorials.GettingStarted-Part2">Part 2</a></div>
<div align="center"><a href="#top">top</a></div>
