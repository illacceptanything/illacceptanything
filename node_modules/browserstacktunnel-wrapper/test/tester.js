(function () {
  'use strict';

  var http = require('http'),
      BrowserStackTunnel = require('../src/BrowserStackTunnel'),
      config = {
        hostname: 'localhost',
        port: 9876
      };

  var server = http.createServer(function (request, response) {
    response.end('This is a test');
  });
  server.listen(config.port);

  var tunnel = new BrowserStackTunnel({
    key: process.env.BROWSER_STACK_ACCESS_KEY || 'KvLmNJcAdydcipLNZRxg',
    hosts: [{
      name: config.hostname,
      port: config.port,
      sslFlag: 0
    }]
  });

  tunnel.start(function (error) {
    if (error) {
      console.log(error);
    } else {
      console.log('Tunnel established.');
    }
    tunnel.killTunnel();
    process.exit();
  });
}());