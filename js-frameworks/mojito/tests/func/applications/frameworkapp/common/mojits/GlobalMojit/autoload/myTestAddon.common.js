/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
/**
 * @submodule ActionContextAddon
 */
YUI.add('mojito-mytest-addon', function(Y, NAME) {

    function Addon(command, adapter, ac) {
        this.ac = ac;
    }

    Addon.prototype = {

        namespace: 'mytest',

        myAddonFunction: function() {
            return "I am calling from the addon in the GlobalMojit";
        }
    };


    Y.mojito.addons.ac.mytest = Addon;

}, '0.1.0');
