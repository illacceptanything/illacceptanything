## Running Arrow Functional Tests 

1. From the application directory, install Arrow and PhantomJS:

        $ npm install
1. Start PhantomJS in the background:

        $ node_modules/phantomjs/bin/phantomjs --webdriver=4445 &

1. Start the application in the background.

        $ mojito start &

1. Run the Arrow test.

        $ arrow --browser=phantomjs arrow_tests/test_tribapp_descriptor.json 

1. You should see that one test has passed.




