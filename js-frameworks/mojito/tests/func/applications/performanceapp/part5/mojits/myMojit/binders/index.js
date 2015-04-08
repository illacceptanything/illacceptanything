/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('myMojitBinder', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        handleClick: function(event){
            
            alert(event.currentTarget.getContent());
        }

    };

});