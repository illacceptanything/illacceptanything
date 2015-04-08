
require("../lib/jasmine-promise");
var Q = require("q");
var Http = require("../../http");
var Negotiate = require("../../http-apps/negotiate");
var Content = require("../../http-apps/content");

describe("http host negotiation", function () {
    it("should work", function () {
        return Http.Server(Negotiate.Host({
            "localhost:999": function (request) {
                throw new Error("Should not get here");
            },
            "127.0.0.1:999": function (request) {
                throw new Error("Should not get here");
            },
            "localhost": function (request) {
                return Content.ok("Hello, Localhost");
            },
            "127.0.0.1": function (request) {
                return Content.ok("Hello, 127.0.0.1");
            }
        }))
        .listen(0)
        .then(function (server) {
            var port = server.address().port;
            return Q.fcall(function () {
                return Http.read({
                    "url": "http://localhost:" + port,
                    "charset": "utf-8"
                })
                .then(function (result) {
                    expect(result).toEqual("Hello, Localhost");
                });
            })
            .then(function () {
                return Http.read({
                    "url": "http://127.0.0.1:" + port,
                    "charset": "utf-8"
                })
                .then(function (result) {
                    expect(result).toEqual("Hello, 127.0.0.1");
                });
            })
            .finally(server.stop);
        });
    });
});

