"use strict";

require("../../lib/jasmine-promise");
var Q = require("q");
var FS = require("../../../fs");
/*global describe,it,expect */

describe("write", function () {
    it("should write a file to a mock filesystem", function () {
        return FS.mock(FS.join(__dirname, "fixture"))
        .then(function (mock) {

            return Q.fcall(function () {
                return mock.write("hello.txt", "Goodbye!\n");
            })
            .then(function () {
                return mock.isFile("hello.txt");
            })
            .then(function (isFile) {
                expect(isFile).toBe(true);
                return mock.read("hello.txt");
            })
            .then(function (content) {
                expect(content.toString("utf-8")).toBe("Goodbye!\n");
            })
        });
    });

    it("takes a flags argument", function () {
        return FS.mock(FS.join(__dirname, "fixture"))
        .then(function (mock) {
            return Q.fcall(function () {
                return mock.write("hello.txt", "Goodbye!\n", "a");
            })
            .then(function () {
                return mock.read("hello.txt");
            })
            .then(function (contents) {
                expect(contents).toBe("Hello, World!\nGoodbye!\n");
            });
        });
    });

    it("takes an options object with flags", function () {
        return FS.mock(FS.join(__dirname, "fixture"))
        .then(function (mock) {
            return Q.fcall(function () {
                return mock.write("hello.txt", "Goodbye!\n", { flags: "a" });
            })
            .then(function () {
                return mock.read("hello.txt");
            })
            .then(function (contents) {
                expect(contents).toBe("Hello, World!\nGoodbye!\n");
            });
        });
    });

    it("calls open correctly", function () {
        return FS.mock(FS.join(__dirname, "fixture"))
        .then(function (mock) {
            mock.open = function (path, options) {
                expect(path).toBe("hello.txt");
                expect(options.flags).toBe("a");
                expect(options.charset).toBe("utf8");
                return Q.resolve({write: function () {}, close: function () {}});
            };

            return mock.write("hello.txt", "Goodbye!\n", "a", "utf8");
        });
    });
});

