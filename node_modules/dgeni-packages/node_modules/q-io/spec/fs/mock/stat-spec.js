"use strict";

require("../../lib/jasmine-promise");
var Q = require("q");
var FS = require("../../../fs");
/*global describe,it,expect */

describe("stat", function () {
    it("should return a Stats object", function () {
        return FS.mock(FS.join(__dirname, "fixture"))
        .then(function (mock) {
            return mock.stat("hello.txt");
        })
        .then(function (stat) {
            expect(stat.node).toBeDefined();
            expect(stat.size).toBeDefined();
            expect(stat.size).toBeGreaterThan(0);
            expect(stat.isDirectory()).toBe(false);
            expect(stat.isFile()).toBe(true);
        });
    });



});

