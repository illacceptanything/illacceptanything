/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('AssetsMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac) {
            ac.done();
        },
        
        assetsWithLocation: function(ac){
        	
        	var myAssets = ac.config.getDefinition('assets');
        	
        	if (myAssets)
        	{
        		Y.Object.each(myAssets, function(assetSpec, assetLoc){
        			Y.Object.each(assetSpec, function(assetArray, assetName){
        				Y.Array.each(assetArray, function(eachAsset){
        					if (assetName === "css")
        					{
        						ac.assets.addCss(eachAsset, assetLoc);
        					}
        					else if (assetName === "js")
        					{
        						ac.assets.addJs(eachAsset, assetLoc);
        					}
        					else if (assetName === "blob")
        					{
        						ac.assets.addBlob(eachAsset, assetLoc);
        					}
        					else
        					{
        						Y.log("I don't know how to add the " + assetName);
        					}
        				});
        			});
        		});
        	}

        	//ac.assets.addBlob('<style type="text/css">body{background-color:pink}</style>', "top");

        	ac.done();
        },
        
        assetsByDefault: function(ac){
        	var myAssets = ac.config.getDefinition('assets');
        	if (myAssets)
        	{
        		Y.Object.each(myAssets, function(assetSpec, assetLoc){
        			Y.Object.each(assetSpec, function(assetArray, assetName){
        				Y.Array.each(assetArray, function(eachAsset){
        					if (assetName === "css")
        					{
        						ac.assets.addCss(eachAsset);
        					}
        					else if (assetName === "js")
        					{
        						ac.assets.addJs(eachAsset);
        					}
        					else if (assetName === "blob")
        					{
        						ac.assets.addBlob(eachAsset);
        					}
        					else
        					{
        						Y.log("I don't know how to add the " + assetName);
        					}
        				});
        			});
        		});
        	}
        	
        	ac.done();
        },
        
        assetsImg: function(ac){
            var chickenurl = ac.assets.getUrl("chicken.jpg");
                starurl = ac.assets.getUrl("star.png");
            ac.assets.preLoadImages([chickenurl, starurl]);
            ac.done({url1: chickenurl, url2: starurl});
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-assets-addon',
    'mojito-config-addon']});
