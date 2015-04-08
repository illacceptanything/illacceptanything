# node-html-encoder

NodeJS wrapper for JavaScript HTML Encoder library http://www.strictly-software.com/htmlencode

## Install
    npm install node-html-encoder

## Examples

    var Encoder = require('node-html-encoder').Encoder;

    // entity type encoder
    var encoder = new Encoder('entity');

    console.log(encoder.htmlEncode('<foo /> "bar"'))
    // prints &lt;foo /&gt; &quot;bar&quot;

    console.log(encoder.htmlDecode('&lt;foo /&gt; &quot;bar&quot;'))
    // prints <foo /> "bar"

    // numerical type encoder
    encoder = new Encoder('numerical');

    console.log(encoder.htmlEncode('<foo /> "bar"'))
    // prints &#60;foo /&#62; &#34;bar&#34;

    console.log(encoder.htmlDecode('&#60;foo /&#62; &#34;bar&#34;'))
    // prints <foo /> "bar"
