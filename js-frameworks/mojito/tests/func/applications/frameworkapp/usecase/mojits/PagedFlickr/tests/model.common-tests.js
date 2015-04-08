YUI.add('PagedFlickrModel-tests', function(Y, NAME) {
    
    var suite = new YUITest.TestSuite(NAME),
        model = null,
	A = YUITest.Assert;
	
    suite.add(new YUITest.TestCase({

        name: 'PagedFlickr model user tests',

        setUp: function() {
            model = new Y.mojit.test.PagedFlickr.model();
        },
        tearDown: function() {
            model = null;
        },

        'test flickr results format': function() {
            var callbackCalledPass1 = false,
                callbackCalledPass2 = false;

            // We'll make two overlapping calls, and check that they
            // do indeed overlap.
            model.getFlickrImages('mojito', 0, 8, function(firstPhotos) {
                var image8pass1;
                Y.log(firstPhotos);
                A.isNotUndefined(firstPhotos, 'no photos object in result');
                A.isArray(firstPhotos);
                A.isTrue(firstPhotos.length > 0, 'empty list of photos');
                // test out last image
                image8pass1 = firstPhotos[7];
                A.isObject(image8pass1);
                A.isString(image8pass1.url, 'bad image url');
                A.isString(image8pass1.title, 'bad image title');
                callbackCalledPass1 = true;

                model.getFlickrImages('mojito', 7, 8, function(secondPhotos) {
                    var image8pass2;
                    Y.log(secondPhotos);
                    A.isNotUndefined(secondPhotos, 'no photos object in result');
                    A.isArray(secondPhotos);
                    A.isTrue(secondPhotos.length > 0, 'empty list of photos');
                    // test out first image
                    image8pass2 = secondPhotos[0];
                    A.isObject(image8pass2);
                    A.isString(image8pass2.url, 'bad image url');
                    A.isString(image8pass2.title, 'bad image title');
                    callbackCalledPass2 = true;
                    // make sure that last image from first pass is the same as
                    // first image of second pass
                    A.areEqual(image8pass1.url, image8pass2.url, "images didn't overlap");
                    A.areEqual(image8pass1.title, image8pass2.title, "images didn't overlap");
                });

            });

            this.wait(function() {
                A.isTrue(callbackCalledPass1, 'the model callback (pass 1) was never executed');
                A.isTrue(callbackCalledPass2, 'the model callback (pass 2) was never executed');
            }, 6000);
        }

    }));
	
    YUITest.TestRunner.add(suite);
    
}, '0.0.1', {requires: ['mojit-test', 'PagedFlickrModel']});
