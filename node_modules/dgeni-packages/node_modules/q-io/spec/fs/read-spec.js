
require("../lib/jasmine-promise");
var FS = require("../../fs");

describe("read", function () {

    it("should read in utf-8", function () {
        return FS.read(FS.join(__dirname, "fixtures/hello.txt"))
        .then(function (text) {
            expect(text).toBe("Hello, World!\n");
        });
    });

    it("should read in bytewise", function () {
        return FS.read(FS.join(__dirname, "fixtures/hello.txt"), "b")
        .then(function (bytes) {
            expect(bytes instanceof Buffer).toBe(true);
        });
    });

});

