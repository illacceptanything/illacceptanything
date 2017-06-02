/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('myMojitBinder', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        handleClick: function(event){
            
            alert(event.currentTarget.getContent());
        }

    };

});
