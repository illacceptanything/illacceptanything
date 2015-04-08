require("../lib/jasmine-promise");
var Q = require("q");
var HTTPS = require("https");
var http = require('../../http');

describe("https agent", function () {

    it("should allow use self-signed certificate", function () {
        var options = {
            passphrase: 'q-io-https',
            key: '-----BEGIN RSA PRIVATE KEY-----\n' +
'Proc-Type: 4,ENCRYPTED\n' +
'DEK-Info: DES-EDE3-CBC,6ACDE0E22CA24B54\n' +
'\n' +
'MHs2noP3Vm8SyHkZ/KaPv2tkwEQx3CZVFmaoTUakJsj/QT2itDWoEzRZaffmqJ5z\n' +
'tCYyXFhwfJinJVwkkOGU5Hv2n8hYNcsRucFC5Y+BUQKEuAtalq3YGoIWWggDPzqb\n' +
'xGEI46PZS+kt8wGcMau1ArSwv2GuY3P8yjV97uus8Mc+U7XazkAgtlLX95wu9cGL\n' +
'HrMUCll0SdCwwiQ+yOh/KPvLTOImq8l4h1EUi1EbLREvobpVvwWgwKPixKFSLg/3\n' +
'KM9WK8P7nuLoJlTTgwXjwOZRWGCaq/m8UkmeMGN3+OBjRnVOdwksvFJSRuHyy24F\n' +
'9DQHo3Lh+MRrlTq5byYFAYuom4yszxeTm6LS9iXlJNB2mrf1LHPvPlR86oXz8EoI\n' +
'/sgfYLOUUMtF5779dmeyUFeekW8w09uFt3BfcxWbh+AVraIHkH3iXHwvD5lrx2re\n' +
'MXP1PHZnzbhEkV01bRL0+keQuS3gd+q723iQnY6NqXPtnaQxBLt6Z0NLBAh6NbR5\n' +
'CsPRyDnNCYz2/i/3lxe2vGeVAGLgu5U2r8MvKXGMn6off+94jmLGWgWcnqlt/bfg\n' +
'Ob9hiZEjSAX+NOvnvhcT/J4NYnVoZPqodM+zvgSDMP1CGQ84k4Ubsnvv70wYvFbY\n' +
'CNqS/TpnK5uDE9vbwFyJhgOTXF+8PZJE3cRfOo9INIo39b9TTE7jPbgSF+k5V3Dd\n' +
'4t55Xt9GaJAHr3xVViNtOKssbXtnQ73P3dg4pTZkA4E0i0kYZhmR45qXL9989Pbm\n' +
'lNlirMIj8P4ysX2/MxT2KYcG6IBSHnAzx5raY0xSFUOeAB5q9GheqA==\n' +
'-----END RSA PRIVATE KEY-----\n',
            cert: '-----BEGIN CERTIFICATE-----\n' +
'MIICsDCCAhmgAwIBAgIJANjH3x23QvNtMA0GCSqGSIb3DQEBBQUAMEUxCzAJBgNV\n' +
'BAYTAkFVMRMwEQYDVQQIEwpTb21lLVN0YXRlMSEwHwYDVQQKExhJbnRlcm5ldCBX\n' +
'aWRnaXRzIFB0eSBMdGQwHhcNMTMwOTEyMDEyMjUzWhcNMjMwOTEwMDEyMjUzWjBF\n' +
'MQswCQYDVQQGEwJBVTETMBEGA1UECBMKU29tZS1TdGF0ZTEhMB8GA1UEChMYSW50\n' +
'ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKB\n' +
'gQDDPt+Ouup0iECslVxoA9mspIDVtgjph9bAuYmB84K3nz/CTA2E8mJhfDCFUk+v\n' +
'ipKvD+bnVgR8asql8AutHzaiCr6yOxUfs9gRgJ37TQGmTFpKx5/OegV2t7khq8Uc\n' +
'Wp7SChZuQ2CZrDjNeZl6K7CFtcbU6yV4tYKsNyNQslSyvwIDAQABo4GnMIGkMB0G\n' +
'A1UdDgQWBBTn1FqVoBznKu+Vtl5Bspq4PGwxEjB1BgNVHSMEbjBsgBTn1FqVoBzn\n' +
'Ku+Vtl5Bspq4PGwxEqFJpEcwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgTClNvbWUt\n' +
'U3RhdGUxITAfBgNVBAoTGEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZIIJANjH3x23\n' +
'QvNtMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQADgYEAMw8bTEPx+rlIxTMR\n' +
'rvZriPeMzlh45vhRyy7JO1C+RnzSVBWUyL/Ca7QbrFcq4nGai2xFnqEEiPKWrP8b\n' +
've1oBz9V18gv6T4oiqOpFmoh2c/IVNr2qE01GdGIos/wb1cYEFRsMLPplbv4PROR\n' +
'89Yb8/2e/5CzxW4LAMNRqD5GwKE=\n' +
'-----END CERTIFICATE-----\n'
        };

        var server = HTTPS.createServer(options, function (req, res) {
            res.writeHead(200, {
                'Content-Type': 'text/plain',
                'Content-Length': 2
            });
            res.end('ok');
        });

        server.listen(0);

        var port = server.address().port;

        var allow = http.request({
            "host": "localhost",
            "port": port,
            "ssl": true,
            "agent": new HTTPS.Agent({
                rejectUnauthorized: false
            })
        })
        .then(function (response) {
            expect(Q.isPromise(response.body)).toBe(false);
            return response.body.read()
            .then(function (body) {
                expect(body.toString("utf-8")).toBe("ok");
            });
        });

        var reject = http.request({
            host: "localhost",
            port: port,
            ssl: true,
            agent: new HTTPS.Agent({
                rejectUnauthorized: true
            })
        }).then(function (response) {
            // should not be here
            expect(response).toBeUndefined();
        }).fail(function(err) {
            expect(Q.isPromise(err)).toBe(false);
            expect(err).toEqual(jasmine.any(Error));
            expect(err.message).toBe('DEPTH_ZERO_SELF_SIGNED_CERT');
        });

        return Q.all([allow, reject]).finally(function () {
            server.close();
        });
    });
});