var FS = require("../../fs");

require("../lib/jasmine-promise");

describe("reroot", function () {

    it("should still have makeDirectory()", function() {
        return FS.reroot("/")
        .then(function(fs) {
            expect(fs.makeTree instanceof Function).toBe(true);
            expect(fs.makeDirectory instanceof Function).toBe(true);
        });
    });

    it("should have a makeDirectory() that creates within the attenuated root", function() {
        var tmpdir = FS.join(__dirname, 'tmp');
        return FS.removeTree(tmpdir)
        .then(null, function () {
            // ignore fail
        }).then(function() {
            return FS.makeTree(tmpdir)
        }).then(function () {
            return FS.isDirectory(tmpdir);
        }).then(function (isDirectory) {
            if (!isDirectory)
                throw new Error("Failed to create tmpdir");
        }).then(function() {
            return FS.reroot(tmpdir);
        }).then(function(fs) {
            return fs.makeDirectory('/foo');
        }).then(function() {
            var outerFooDir = FS.join(tmpdir, 'foo');
            return FS.isDirectory(outerFooDir);
        }).then(function(isDirectory) {
            if (!isDirectory)
                throw new Error("Directory not created");
        })
        .finally(function () {
            return FS.removeTree(tmpdir)
            .then(null, function () {
                // ignore fail
            })
        })
    });
});
