/*global __dirname*/
require("../lib/jasmine-promise");
var Http = require("../../http");
var Apps = require("../../http-apps");
var FS = require("../../fs");
var Mock = require("../../fs-mock");

describe("FileTree followInsecureSymbolicLinks", function () {
    var testApp;
    function makeApp(followInsecureSymbolicLinks) {
        var fixture = FS.join(module.directory || __dirname, "fixtures");
        return FS.mock(fixture)
        .then(function (mockFS) {
            return FS.merge([ mockFS, Mock({ "6789/0123.txt": "0123\n" })]);
        })
        .then(function (mockFS) {
            return mockFS.symbolicCopy("6789", FS.join("9012","linkedDir"), "directory").thenResolve(mockFS);
        })
        .then(function (mockFS) {
            return mockFS.symbolicCopy( FS.join("6789","0123.txt"), FS.join("9012","linkedFile.txt"), "file").thenResolve(mockFS);
        })
        .then(function (mockFS) {
            return new Apps.Chain()
            .use(Apps.ListDirectories)
            .use(function () {
                return Apps.FileTree(FS.join("/","9012"), {fs: mockFS, followInsecureSymbolicLinks: followInsecureSymbolicLinks});
            })
            .end()
        }).then(function (app) {
            return Http.Server(app).listen(0)
        });
    };

    describe("if false", function () {
        beforeEach(function () {
            testApp = makeApp(false)
        });
        it("should not follow symbolic links to directories", function () {
            return testApp.then(function (server) {
                var port = server.address().port;
                return Http.read({
                    url: "http://127.0.0.1:" + port + "/linkedDir/",
                    headers: {
                        accept: "text/plain"
                    },
                    charset: 'utf-8'
                })
                .then(null, function (error) {
                    expect(error.response.status).toEqual(404);
                })
                .finally(server.stop);
            });
        });
        it("should not follow symbolic links to files", function () {
            return testApp.then(function (server) {
                var port = server.address().port;
                return Http.read({
                    url: "http://127.0.0.1:" + port + "/linkedFile.txt",
                    headers: {
                        accept: "text/plain"
                    },
                    charset: 'utf-8'
                })
                .then(null, function (error) {
                    expect(error.response.status).toEqual(404);
                })
                .finally(server.stop);
            });
        });
    });

    describe("if true", function () {
        beforeEach(function () {
            testApp = makeApp(true)
        });
        it("should follow symbolic links to directories", function () {
            return testApp.then(function (server) {
                var port = server.address().port;
                return Http.read({
                    url: "http://127.0.0.1:" + port + "/linkedDir/",
                    headers: {
                        accept: "text/plain"
                    },
                    charset: 'utf-8'
                })
                .then(function (content) {
                    expect(content).toEqual("0123.txt\n");
                })
                .finally(server.stop);
            });
        });
        it("should follow symbolic links to files", function () {
            return testApp.then(function (server) {
                var port = server.address().port;
                return Http.read({
                    url: "http://127.0.0.1:" + port + "/linkedFile.txt",
                    headers: {
                        accept: "text/plain"
                    },
                    charset: 'utf-8'
                })
                .then(function (content) {
                    expect(content).toEqual("0123\n");
                })
                .finally(server.stop);
            });
        });
    });

});
