Notes
-----

Create project and mojit

    mojito create app simple yui_module
    mojito create mojit simple Notepad
    
Put a YUI Gallery Module in ./yui_modules/ AND rename it to include an affinity:

    mkdir ./yui_modules/
    wget -O ./yui_modules/storage-lite.client.js \
        https://raw.github.com/rgrove/storage-lite/master/src/storage-lite.js

Edit application.json "specs":

    "notepad": {
        "type": "HTMLFrameMojit",
        "config": {
            "deploy": true,
            "title": "Notepad example",
            "child": {
                "type": "Notepad"
            }
        }
    }

Add to routes.json:

    "/": {
        "call": "notepad.index",
        "path": "/",
        "verbs": ["get"]
    }

Edit controller index action:

    ac.done();

Add view and binder.
