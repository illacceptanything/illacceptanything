/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('SearchMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        index: function(ac)
        {
            var query = ac.params.getFromMerged('query') || 'food',
                zip = ac.params.getFromMerged('zip') || '94089',
                result = { query: query, zip: zip };
            
            ac.models.get('localsearch').getData(query, zip, function(r) {
                
                // demo: return a random number of results
                var count = Math.floor(Math.random()*11),
                    results = r.slice(0,count),
                    children = {};
                
                result.results = [];
                Y.Array.each(results, function (result, i) {
                    children[i] = { type: 'SearchResult', config: result };
                });
                
                if (results.length)
                {
                    ac.composite.execute({ children: children}, function (data, meta)
                    {
                        // format the results so the template can iterate through them
                        Y.Object.each(data, function (item, i) {
                            result.results.push({ output: item });
                        });
                        ac.done(result, meta);
                    });
                }
                else
                {
                    ac.done(result, {});
                }
            });
        }
    };
}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-composite-addon',
    'mojito-params-addon',
    'SearchMojitModel']});
