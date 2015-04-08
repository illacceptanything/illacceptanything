# 8. Assets in Mojito #

## What Are Assets? ##

Assets are resources that are required on the clients. These resources are 
primarily CSS but can also be JavaScript that is ancillary to and not a 
core component of the Mojito application. 

## Location of Assets ##

Assets can be used at the application level and the mojit level.

### Application Assets ###

`{application_name}/assets/css`

### Mojit Assets ###

`{application_name}/mojits/{mojit_name}/assets`


### Static URL to Assets ##

To access assets from your templates, you need to use a static URL
to your assets.

For application-level assets, the static URL has the following syntax:

`/static/{application_name}/assets/{asset_file}`

For mojit-level assets, the static URL has the following syntax:

`/static/{mojit_name}/assets/{asset_file}`

The asset `index.css` is included in the template with the static URL.

    <link rel="stylesheet" type="text/css" href="/static/simple/assets/css/index.css"/>
      <div id="{{mojit_view_id}}" class="mojit">
      <h2 id="header">{{title}}</h2>
      <ul class="toolbar">
      {{#colors}}
        <li>{{id}}</li>
      {{/colors}}
      </ul>
    </div>


## Configuring Assets ##

In the application configuration files, you can specify assets with the 
`assets` object. Those assets in the ``top`` object are inserted
into the `<head>` element, and the assets in the `bottom` object
are inserted in the `<body>` element right before the closing
tag.

    [
      {
        "settings": [ "master" ],
        "specs": {
          "simple" : {
            "type": "myMojit"
          },
          "assets": {
            "top": {
              "css": [
                "/static/myApp/assets/css/index.css",
                "/static/myMojit/css/defaults.css"
              ],
            }
            "bottom": {
              "js": [
                "/mojits/myApp/assets/js/index.js"
              ]
            }
          }
        }
      }
    ]


## Dynamically Accessing Assets from Templates ##

From the controller, you can use the `ActionContext` object to use the 
`Assets` addon that provides methods for adding assets to your template:

  YUI.add('myMojit', function(Y, NAME){
    Y.namespace('mojito.controllers')[NAME] = {
      index: function(ac) {
        var css = "./simple.css";
        ac.assets.addCss(css, 'top');
        ac.done({ title: "Example of Adding Assets" });
      }
    };
  }, '0.0.1', {requires: ['mojito-assets-addon']});


## Learn More ##

* [Assets](http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_assets.html)
* [Code Examples: Assets](http://developer.yahoo.com/cocktails/mojito/docs/code_exs/assets.html)
