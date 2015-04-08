
require("../lib/jasmine-promise");
var FS = require("../../fs");

describe("write", function () {
    it("should write in utf-8", function () {
        var path = FS.join(__dirname, "fixtures/so-it-is-written.txt");
        var content = "Hello, World!\n";
        return FS.write(path, content)
        .then(function (result) {
            expect(result).toBe(undefined);
            return FS.read(path)
        })
        .then(function (readContent) {
            expect(readContent).toEqual(content);
        })
        .finally(function () {
            return FS.remove(path);
        });
    });

    it("should write bytewise", function () {
        var path = FS.join(__dirname, "fixtures/so-it-is-written.txt");
        var content = "Good bye, cruel World!\n";
        return FS.write(path, new Buffer(content, "utf-8"))
        .then(function (result) {
            expect(result).toBe(undefined);
            return FS.read(path)
        })
        .then(function (readContent) {
            expect(readContent).toEqual(content);
        })
        .finally(function () {
            return FS.remove(path);
        });
    });
});

