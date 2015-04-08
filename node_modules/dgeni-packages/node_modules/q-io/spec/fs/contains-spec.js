
var FS = require("../../fs");

require("../lib/jasmine-promise");

describe("contains", function () {
    it("reflexive case", function () {
        expect(FS.contains("/a/b", "/a/b")).toBe(true);
    });
});

