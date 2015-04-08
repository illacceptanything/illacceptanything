YUI.add('YckMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            var allcookies =ac.yck.get();
            //ycookie = new ac.yck.get("ycookie");
            var data = {
                allcookies:allcookies
            };
            ac.done(data);
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-yck-addon']});
