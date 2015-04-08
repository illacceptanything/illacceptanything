
require("../lib/jasmine-promise");
var Http = require("../../http");
var Apps = require("../../http-apps");
var FS = require("../../fs");

// http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35.1
describe("HTTP Range", function () {
    var fixture, app, serveAndTest;
    describe("Byte Ranges" , function () {
        beforeEach(function () {
            fixture = FS.join(module.directory || __dirname, "fixtures", "01234.txt");
            app = new Apps.Chain()
                .use(Apps.Cap)
                .use(function () {
                    return Apps.File(fixture);
                })
                .end();
            serveAndTest = function serveAndTest(rangeExp, expectedStatus) {
                return Http.Server(app)
                .listen(0)
                .then(function (server) {
                    var port = server.node.address().port;
                    return Http.read({
                        "url": "http://127.0.0.1:" + port + "/",
                        "headers": {
                            "range": rangeExp
                        }
                    }, function (response) {
                        return response.status === expectedStatus;
                    }).finally(server.stop);
                })
            }
        });

        // A byte range operation MAY specify a single range of bytes, or a set of ranges within a single entity.
        //    ranges-specifier = byte-ranges-specifier
        //    byte-ranges-specifier = bytes-unit "=" byte-range-set
        //    byte-range-set  = 1#( byte-range-spec | suffix-byte-range-spec )
        //    byte-range-spec = first-byte-pos "-" [last-byte-pos]
        //    first-byte-pos  = 1*DIGIT
        //    last-byte-pos   = 1*DIGIT
        describe("byte range spec", function () {
            // The first-byte-pos value in a byte-range-spec gives the byte-offset of the first byte in a range. The
            // last-byte-pos value gives the byte-offset of the last byte in the range; that is, the byte positions
            // specified are inclusive. Byte offsets start at zero.
            it("positions are inclusive", function () {
               return serveAndTest("bytes=0-2", 206)
               .then(function (content) {
                   expect(content.toString('utf-8')).toEqual('012');
               });
            });
            it("last position is optional", function () {
               return serveAndTest("bytes=0-", 206)
               .then(function (content) {
                   expect(content.toString('utf-8')).toEqual('01234.txt\n');
               });
            });

            // TODO
            xit("non contiguous ranges", function () {
               return serveAndTest("bytes=0-2,4-5", 206)
               .then(function (content) {
                   expect(content.toString('utf-8')).toEqual('0124.');
               });
            });

            // If the last-byte-pos value is present, it MUST be greater than
            // or equal to the first-byte-pos in that byte-range-spec, or the
            // byte- range-spec is syntactically invalid. The recipient of a
            // byte-range- set that includes one or more syntactically invalid
            // byte-range-spec values MUST ignore the header field that
            // includes that byte-range- set.
            describe("invalid syntax should be ignored", function () {
                it("last positions must be greater than first", function () {
                    return serveAndTest("bytes=4-3", 216)
                    .then(function (content) {
                        expect(false).toBe(true);
                    }, function (error) {
                        expect(error.response.status).toBe(416);
                    })
                });
                it("if any part is invalid, all of it is invalid", function () {
                    return serveAndTest("bytes=0-2,4-3", 216)
                    .then(function (content) {
                        expect(false).toBe(true);
                    }, function (error) {
                        expect(error.response.status).toBe(416);
                    });
                });
            });

            // If the last-byte-pos value is absent, or if the value is greater than or equal to the current length of
            // the entity-body, last-byte-pos is taken to be equal to one less than the current length of the entity-
            // body in bytes.
            describe("last position should be truncated to the length of the content", function () {
                it("single range", function () {
                   return serveAndTest("bytes=0-10", 206)
                   .then(function (content) {
                       expect(content.toString('utf-8')).toEqual('01234.txt\n');
                   });
                });
                // TODO
                xit("multiple ranges", function () {
                   return serveAndTest("bytes=0-2,1-10", 206)
                   .then(function (content) {
                       expect(content.toString('utf-8')).toEqual('0121234.txt\n');
                   });
                });
            });
        });

        //By its choice of last-byte-pos, a client can limit the number of
        //bytes retrieved without knowing the size of
        // the entity.
        //    suffix-byte-range-spec = "-" suffix-length
        //    suffix-length = 1*DIGIT
        describe("suffix byte range spec", function () {
            it("can limit the number of bytes retrieved", function () {
               return serveAndTest("bytes=-3", 206)
               .then(function (content) {
                   expect(content.toString('utf-8')).toEqual('0123');
               });
            });

            // A suffix-byte-range-spec is used to specify the suffix of the
            // entity-body, of a length given by the suffix-length value. (That
            // is, this form specifies the last N bytes of an entity-body.) If
            // the entity is shorter than the specified suffix-length, the
            // entire entity-body is used.
            it("last position should be truncated to the length", function () {
               return serveAndTest("bytes=-20", 206)
               .then(function (content) {
                   expect(content.toString('utf-8')).toEqual('01234.txt\n');
               });
            });
        });

        // If a syntactically valid byte-range-set includes at least one byte-
        // range-spec whose first-byte-pos is less than the current length of
        // the entity-body, or at least one suffix-byte-range-spec with a non-
        // zero suffix-length, then the byte-range-set is satisfiable.
        // Otherwise, the byte-range-set is unsatisfiable.  If the
        // byte-range-set is unsatisfiable, the server SHOULD return a response
        // with a status of 416 (Requested range not satisfiable). Otherwise,
        // the server SHOULD return a response with a status of 206 (Partial
        // Content) containing the satisfiable ranges of the entity-body.
        describe("satisfiability", function () {
            it("should return 416 if a byte range spec is unsatisfiable", function () {
               return serveAndTest("bytes=10-11", 416)
               .then(function (content) {
                   expect(true).toBeTruthy();
               }).fail(function () {
                   expect(false).toBeTruthy();
               });
            });

            it("should return 416 if a suffix byte range spec is unsatisfiable", function () {
               return serveAndTest("bytes=1-0", 206)
               .then(function (content) {
                   expect(true).toBeFalsy();
               }).fail(function (error) {
                   expect(error.response.status).toBe(416);
               });
            });

            it("should return 416 if all byte range spec are unsatisfiable", function () {
               return serveAndTest("bytes=10-11,-0", 416)
               .then(function (content) {
                   expect(true).toBeTruthy();
               }).fail(function () {
                   expect(false).toBeTruthy();
               });
            });

            // TODO
            xit("should return 200 if at least one byte range spec is satisfiable", function () {
               return serveAndTest("bytes=10-11,0-2", 200)
               .then(function (content) {
                   expect(content.toString('utf-8')).toEqual('012');
               });
            });

        });
    });
});
