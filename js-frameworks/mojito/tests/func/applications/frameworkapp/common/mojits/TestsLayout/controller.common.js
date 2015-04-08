/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('TestsLayout', function(Y, NAME) {

/**
 * The MyLayout module.
 *
 * @module MyLayout
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
	     index: function(ac) {
            Y.log('index()', 'debug', NAME);
            var parentConfig = ac.config.get();
                myId = parentConfig.id;
            var childrenConfig = parentConfig.children;
            var childInfo = [];

            Y.Object.each(childrenConfig, function(childSpec, childName) {
            	var serverUrl, urlHTML;
            		
            	if (childSpec.config)
                {
            		serverUrl = childSpec.config.myUrls;
            		Y.Array.each(serverUrl, function(eachUrl){
            			if (urlHTML)
            			{
            				urlHTML += "<a href='"+ eachUrl +"'>" + eachUrl + "</a><br/>";
            			}
            			else
            			{
            				urlHTML = "<a href='"+ eachUrl +"'>" + eachUrl + "</a><br/>";
            			}
            		});
                }
            	//console.log("childName******* "+ childName );
            	//console.log(childSpec);
            	
                var info = {
                    name: childName,
                    urls: urlHTML
                };
                
                childInfo.push(info);
            });

			ac.composite.execute(parentConfig, function(childContent, meta){
				Y.Array.each(childInfo, function(info){
					info.html = childContent[info.name];
				});
				ac.done({
					title: myId,
					childData: childInfo
				}, meta);
			});
		}
    };

}, '0.0.1', {requires: [
    'mojito', 
    'mojito-composite-addon',
    'mojito-config-addon']});
