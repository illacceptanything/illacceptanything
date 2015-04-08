YUI.add('SimpleModelModel', function(Y, NAME) {
    
   Y.mojito.models[NAME] = {
        init: function(cfg) {
            this.cfg = cfg;
        },
          
        getTurkeyImages: function(callback) {
            var photos = [
                {
                    "title": "Wild turkey1",
                    "url": "/static/SimpleModel/assets/BanffPark.jpg"
                },
                {
                    "title": "Wild turkey2",
                    "url": "/static/SimpleModel/assets/Calgary.jpg"      
                },
                {
                    "title": "Wild turkey3",
                    "url": "/static/SimpleModel/assets/JasperPark.jpg"
                },
                {
                    "title": "Wild turkey4",
                    "url": "/static/SimpleModel/assets/RockMountain.jpg"           
                }
            ];
            //add delay to simulate the async nature of YQL
            setTimeout(function() {
                callback(photos);
            }, 10);
        },
        
        getConfigFromModel: function(callback){
            var data = {
                myconfig0: this.cfg.myconfig0,
                myconfig1: this.cfg.myconfig1,
                myconfig2: this.cfg.myconfig2,
                myconfig3: this.cfg.myconfig3,
            }
            
            var mydata = [];
            mydata.push(data);
            callback(mydata);
        }

    };

    function buildFlickrUrlFromRecord(record) {
        return 'http://farm' + record.farm 
            + '.static.flickr.com/' + record.server 
            + '/' + record.id + '_' + record.secret + '.jpg';
    }
    
}, '0.0.1', {requires: [
    'mojito',
    'yql',
    'jsonp-url']}); 
