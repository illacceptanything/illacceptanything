#!/usr/bin/env node

/*jslint anon:true, sloppy:true, nomen:true, stupid:true, node:true*/

var fs = require('fs'),
    path = require('path'),
    wrench = require('wrench'),
    libpath = require('path'),
    glob = require("glob"),
    program = require('commander'),
    async = require('async'),
    child = require('child_process'),
    portfinder = require('portfinder'),
    hostname = require('os').hostname(),
    dns = require('dns'),
    cwd = __dirname,
    pids = [],
    thePid = null,
    pidNames = {},
    thePidName = null,
    returnVal = 0,
    hostip,
    phantomjsport = null,
    arrowReportDir,
    remoteselenium,
    MOJITOLIB = libpath.resolve(cwd, '..'),
    MOJITO_STARTED_REGEX;
/*jslint regexp: true*/
// MOJITO_STARTED_REGEX = /Mojito v\S{4,} started.+/; //todo: use callback
MOJITO_STARTED_REGEX = /✔\tMojito ready to serve\./;

program.parse(process.argv);

function test(cmd) {
    var series = [];
    cmd.logLevel = cmd.logLevel || 'WARN';
    // Default to all tests

    cmd.browser = cmd.browser || 'firefox';
    cmd.unitPath = path.resolve(cwd, cmd.unitPath || cmd.path || './unit');
    cmd.funcPath = path.resolve(cwd, cmd.funcPath || cmd.path || './func');
    if (cmd.reportFolder) {
        cmd.reportFolder = path.resolve(cwd, cmd.reportFolder);
    }

    if (cmd.unit) {
        arrowReportDir = cmd.reportFolder || cmd.unitPath;
    } else {
        arrowReportDir = cmd.reportFolder || cmd.funcPath + '/../..';
    }
    arrowReportDir = arrowReportDir + '/artifacts/arrowreport/';

    try {
        wrench.rmdirSyncRecursive(arrowReportDir);
    } catch (e) {}
    wrench.mkdirSyncRecursive(arrowReportDir);


    if (process.env.SELENIUM_HUB_URL) {
        remoteselenium = process.env.SELENIUM_HUB_URL;
        console.log('selenium host.....' + remoteselenium);
        series.push(gethostip);
    }

    if (cmd.arrow) {
        series.push(startArrowServer);
    }

    if (cmd.browser === 'phantomjs') {
        series.push(function (callback) {
            startPhantomjs(cmd, callback);
        });
    } else if (cmd.reuseSession) {
        if (cmd.selenium) {
            series.push(function (callback) {
                startArrowSelenium(cmd, callback);
            });
        }
    }

    if (!cmd.unit && !cmd.func) {
        console.log("---Run both unit and functinal tests with phantomjs---");
        cmd.unit = true;
        cmd.func = true;
    }

    if (cmd.unit) {
        series.push(function (callback) {
            runUnitTests(cmd, callback);
        });
    }
    if (cmd.func) {
        series.push(function (callback) {
            runFuncAppTests(cmd, callback);
        });
    }
    async.series(series, finalize);
}

function gethostip(callback) {
    dns.lookup(hostname, function (err, addr, fam) {
        if (err) {
            callback(err);
            return;
        }
        hostip = addr;
        console.log('App running at.....' + hostip);
        callback(null);
    });
}

function runCommand(path, command, argv, callback) {
    callback = callback || function () {};
    process.chdir(path);
    console.log(command + ' ' + argv.join(' '));
    var cmd = child.spawn(command, argv, {
        cwd: path,
        env: process.env
    });

    cmd.stdout.on('data', function (data) {
        // Don't care generally. But, specific commands may want a listener for this.
    });

    cmd.stderr.on('data', function (data) {
        process.stdout.write(data);
    });

    cmd.on('exit', function (code) {
        cmd.stdin.end();
        if (0 !== code) {
            callback('exit: child process exited with code ' + code);
            return;
        }
        callback(code);
    });

    cmd.on('error', function (err) {
        process.stderr.write('error: ' + err + '\n');
        callback(err.code);
    });

    cmd.on('uncaughtException', function (err) {
        process.stderr.write('uncaught exception: ' + err + '\n');
        callback(1);
    });

    return cmd;
}

function startArrowServer(callback) {
    var timeout,
        p,
        listener = function (data) {
            process.stdout.write(data);
        };
    console.log("---Starting Arrow Server---");
    p = runCommand(cwd, "node", [cwd + "/../node_modules/yahoo-arrow/arrow_server/server.js"], function () {
        // If this command returns called, then it failed to launch
        if (timeout) {
            clearTimeout(timeout);
        }
        console.log('arrow_server failed to start. If it is already running' +
            ' use \'-a\' to skip startup of arrow_server.');
        pids.pop();
        callback(1); // Trigger failure
    });
    p.stdout.on('data', listener);
    pids.push(p.pid);
    pidNames[p.pid] = 'arrow_server';
    timeout = setTimeout(function () {
        p.stdout.removeListener('data', listener); // Stop printing output from arrow_server
        callback(null);
    }, 5000);
}

function startPhantomjs(cmd, callback) {
    console.log("---Starting Phantomjs---");
    var timeout,
        listener,
        done,
        p,
        command,
        commandArgs;

    portfinder.basePort = 4445;
    //find available port for phantomjs
    portfinder.getPort(function (err, port) {
        if (err) {
            console.log(err);
            console.log('Failed to find port to start phantomjs');
            process.exit(1);
            return;
        }
        phantomjsport = port;
        done = function () {
            clearTimeout(timeout);
            p.stdout.removeListener('data', listener);
            callback(null);
        };
        listener = function (data) {
            process.stdout.write(data);
            if (data.toString().match(/GhostDriver - Main - running on port/)) {
                done();
            }
        };
        if (fs.existsSync(cwd + "/../node_modules/phantomjs")) {
            command = "node";
            commandArgs = [cwd + "/../node_modules/phantomjs/bin/phantomjs"];
            commandArgs.push("--webdriver=" + port);
        } else {
            command = "phantomjs";
            commandArgs = ["--webdriver=" + port];
        }
        p = runCommand(cwd, command, commandArgs, function() {
            // If this command returns called, then it failed to launch
            if (timeout) {
                clearTimeout(timeout);
            }
            console.log('phantomjs failed to start. Phantomjs needs to be installed either locally or globally');
            pids.pop();
            callback(1); // Trigger failure
        });
        p.stdout.on('data', listener);
        pids.push(p.pid);
        pidNames[p.pid] = 'phantomjs driver';
        timeout = setTimeout(function () {
            done();
        }, 5000);
    });
}

function startArrowSelenium(cmd, callback) {
    console.log("---Starting Arrow Selenium---");
    var commandArgs = [cwd + "/../node_modules/yahoo-arrow/arrow_selenium/selenium.js"];
    if (remoteselenium) {
        commandArgs.push('--seleniumHost=' + remoteselenium);
    }
    commandArgs.push("--open=" + cmd.browser);
    runCommand(cwd, "node", commandArgs, function () {
        callback(null);
    });
}

function runUnitTests(cmd, callback) {
    console.log('---Running Unit Tests---');

    var descriptor = cmd.descriptor || '**/*_descriptor.json',
        p,
        commandArgs = [
            cwd + "/../node_modules/yahoo-arrow/index.js",
            "--descriptor=" + cmd.unitPath + '/' + descriptor,
            "--exitCode=true",
            "--keepTestReport=true",
            "--report=true",
            "--reportFolder=" + arrowReportDir
        ],
        filestoexclude = 'tests/base/mojito-test.js,' +
            'lib/app/autoload/mojito-client.client.js,' +
            'lib/app/autoload/perf.client.js,lib/app/autoload/perf.server.js';
    if ('phantomjs' !== cmd.browser && cmd.reuseSession) {
        commandArgs.push('--reuseSession');
    }
    if (phantomjsport) {
        commandArgs.push('--phantomHost=http://localhost:' + phantomjsport + '/wd/hub');
    }
    if (cmd.logLevel) {
        commandArgs.push('--logLevel=' + cmd.logLevel);
    }
    if (cmd.browser) {
        commandArgs.push('--browser=' + cmd.browser);
    }
    if (cmd.driver) {
        commandArgs.push('--driver=' + cmd.driver);
    }
    if (cmd.testName) {
        commandArgs.push('--testName=' + cmd.testName);
    }
    if (cmd.group) {
        commandArgs.push('--group=' + cmd.group);
    }
    if (cmd.coverage) {
        commandArgs.push('--coverage=' + cmd.coverage);
    }
    if (cmd.coverage) {
        commandArgs.push('--coverageExclude=' + filestoexclude);
    }
    if (cmd.seleniumHost) {
        commandArgs.push('--seleniumHost=' + cmd.seleniumHost);
    }

    p = runCommand(
        cmd.unitPath,
        "node",
        commandArgs,
        function (code) {
            callback(code);
        }
    );
    p.stdout.on('data', function (data) {
        process.stdout.write(data);
    });
}

function build(cmd, callback) {
    var cmdArgs = [
            'build', 'html5app',
            libpath.resolve(cmd.funcPath, 'applications/frameworkapp/flatfile')
        ];

    console.log('---Building Apps---');
    runCommand(
        cmd.funcPath + '/applications/frameworkapp/common',
        cwd + "/../bin/mojito",
        cmdArgs,
        callback
    );
}

function runStaticApp(basePath, path, port, callback) {
    console.log('---Starting static server for ' + path + ' at port ' + port);
    var listener,
        p = runCommand(
            basePath + '/' + path,
            cwd + "/../node_modules/.bin/static",
            ['-p', port, '-c', '1'],
            function () {}
        );
    thePid = p.pid;
    thePidName = 'static ' + libpath.basename(path) + ':' + port;
    pids.push(p.pid);
    pidNames[p.pid] = 'static ' + libpath.basename(path) + ':' + port;

    listener = function(data) {
        /*jslint regexp: true*/
        if (data.toString().match(/serving \".\" at http:\/\//)) {
            p.stdout.removeListener('data', listener);
            callback(thePid);
        }
    };
    p.stdout.on('data', listener);
}

function runNpm(path, args, callback) {
    runCommand(path, "npm", args, function (code) {
        if (code !== 0) {
            // Try to install with ynpm if npm is not available
            args.push("--registry=http://ynpm-registry.corp.yahoo.com:4080");
            runCommand(path, "ynpm", args, function (code) {
                if (code !== 0) {
                    // Neither npm nor ynpm is available
                    process.stderr.write("please install npm.\n");
                }
                callback(code);
            });
        } else {
            callback(code);
        }
    });
}

function runFuncAppTests(cmd, callback) {
    var descriptor = cmd.descriptor || '**/*_descriptor.json',
        exeSeries = [],
        descriptors = glob.sync(cmd.funcPath + '/' + descriptor),
        // skip some tests (for now) until they are fixed
        skips,
        descriptors2 = [];

    //
    // NOTE: remove those descriptors to prevent those tests to run for now
    //
    // TODO: Fix those tests
    skips = [
    ];
    descriptors.forEach(function (descriptor) {
        var regex,
            found = false,
            i;
        for (i = 0; i < skips.length; i += 1) {
            regex = new RegExp(skips[i]);
            if (regex.test(descriptor)) {
                found = true;
                console.log('Skipping test descriptor : ' + descriptor);
                break;
            }
        }
        if (!found) {
            descriptors2.push(descriptor);
        }
    });
    descriptors = descriptors2;
    //

    async.forEachSeries(descriptors, function(des, callback) {

        var appConfig = JSON.parse(fs.readFileSync(des, 'utf8')),
            app = appConfig[0].config.application,
            port = cmd.port || 8666,
            param = app.param || null,
            type = app.type || 'mojito';
        if (type === "static") {
            exeSeries.push(build(cmd, function() {
                runStaticApp(cmd.funcPath + '/applications', app.path, port, function(thispid) {
                    runFuncTests(cmd, des, port, thispid, callback);
                });
            }));
        } else {
            // Install dependecies for specific projects
            // Change here if you want your app to do npm install prior to start mojito server for test
            //
            // NOTE: run.js needs to install additional dependencies for 
            // quickstartguide. To avoid mojito installation collision,
            // only install required dependencies.
            //
            // TODO: find a more generic solution
            if (app.path === "../../../examples/quickstartguide") {
                exeSeries.push(installDependencies(app, ["node-markdown"], cmd.funcPath + '/applications', function() {
                    runMojitoApp(app, cmd, cmd.funcPath + '/applications', port, app.param, function(thispid) {
                        runFuncTests(cmd, des, port, thispid, callback);
                    });
                }));
            } else {
                exeSeries.push(runMojitoApp(app, cmd, cmd.funcPath + '/applications', port, app.param, function(thispid) {
                    runFuncTests(cmd, des, port, thispid, callback);
                }));
            }
        }
    }, function(err) {
        callback(err);
    });
    async.series(exeSeries, callback);
}

function installMojito(app, basePath, callback) {
    console.log("---Linking mojito dependency---");
    // Install local mojito
    runNpm(basePath + '/' + app.path, ["i", MOJITOLIB], callback);
}

function installDependencies(app, args, basePath, callback) {
    console.log("---Starting installing dependencies---");
    // Install with npm
    runNpm(basePath + '/' + app.path, ["i"].concat(args), function (code) {
        if (code !== 0) {
            callback(code);
        } else {
            // Don't install Mojito dependency for examples/quickstartguide
            // TODO: find a more generic solution
            if (app.path !== "../../../examples/quickstartguide") {
                installMojito(app, basePath, callback);
            } else {
                callback(code);
            }
        }
    });
}

function runFuncTests(cmd, desc, port, thispid, callback) {
    console.log('---Running Functional Tests---');

    var group = cmd.group || null,
        p,
        baseUrl,
        commandArgs;

    if (cmd.baseUrl) {
        baseUrl = cmd.baseUrl;
    } else if (hostip) {
        baseUrl = 'http:\/\/' + hostip + ':' + port;
    } else {
        baseUrl = 'http:\/\/localhost' + ':' + port;
    }
    commandArgs = [
        cwd + "/../node_modules/yahoo-arrow/index.js",
        "--descriptor=" + desc,
        "--baseUrl=" + baseUrl,
        "--exitCode=true",
        "--keepTestReport=true",
        "--report=true",
        "--reportFolder=" + arrowReportDir,
        "--config=" + cwd + "/config/config.js"
    ];
    if ('phantomjs' !== cmd.browser && cmd.reuseSession) {
        commandArgs.push('--reuseSession');
    }
    if (phantomjsport) {
        commandArgs.push('--phantomHost=http://localhost:' + phantomjsport + '/wd/hub');
    }
    if (remoteselenium) {
        commandArgs.push('--seleniumHost=' + remoteselenium);
    }
    if (cmd.logLevel) {
        commandArgs.push('--logLevel=' + cmd.logLevel);
    }
    if (cmd.browser) {
        commandArgs.push('--browser=' + cmd.browser);
    }
    if (cmd.driver) {
        commandArgs.push('--driver=' + cmd.driver);
    }
    if (cmd.testName) {
        commandArgs.push('--testName=' + cmd.testName);
    }
    if (cmd.group) {
        commandArgs.push('--group=' + cmd.group);
    }
    if (cmd.coverage) {
        commandArgs.push('--coverage=' + cmd.coverage);
    }
    if (cmd.seleniumHost) {
        commandArgs.push('--seleniumHost=' + cmd.seleniumHost);
    }

    p = runCommand(
        cmd.funcPath,
        "node",
        commandArgs,
        function (code) {
            try {
                console.log('Shutting down pid ' + thePid + ' -- ' + thePidName);
                process.kill(thePid);
                pids.pop(thePid);
            } catch (e) {
                console.log('FAILED to shut down pid:' + thePid);
            }
            callback(code);
        }
    );
    p.stdout.on('data', function (data) {
        process.stdout.write(data);
    });
}

function runMojitoApp(app, cliOptions, basePath, port, params, callback) {
    var cmdArgs = ['app.js'],
        cmd = "node", // actual cli command to startup the app
        thispid;

    console.log("---Starting application---");
    if (port) {
        process.env.PORT = port;
    }
    if (params) {
        cmdArgs.push('--context');
        cmdArgs.push(params);
    }
    var p = runCommand(basePath + '/' + app.path, cmd, cmdArgs, function () {});
    thispid = p.pid;
    thePid = p.pid;
    thePidName = app.name + ':' + port + (params ? '?' + params : '');
    pids.push(thePid);
    pidNames[p.pid] = thePidName;
    if (cliOptions.debugApps) {
        p.stdout.on('data', function(data) {
            console.error('---DEBUG ' + port + ' STDOUT--- ' + data.toString());
        });
        p.stderr.on('data', function(data) {
            console.error('---DEBUG ' + port + ' STDERR--- ' + data.toString());
        });
    }

    function listener(data) {
        var match = data.toString().match(MOJITO_STARTED_REGEX);
        if (match) {
            p.stdout.removeListener('data', listener);
            console.error('---' + match[0] + '---');
            callback(thePid);
        }
    }

    p.stdout.on('data', listener);
}

function finalize(err, results) {
    var i;
    console.log("---in finalize---");
    for (i = 0; i < pids.length; i += 1) {
        console.log('Shutting down pid ' + pids[i] + ' -- ' + pidNames[pids[i]]);
        try {
            process.kill(pids[i]);
        } catch (e) {
            console.log('FAILED to shut down pid ' + pids[i] + ' -- ' + pidNames[pids[i]]);
        }
    }
    if (err) {
        console.log(err);
        console.log('FAILED');
        process.exit(1);
        return;
    }
    console.log('Completed');
    process.exit(0);

}

function test(cmd) {
    var series = [];
    cmd.logLevel = cmd.logLevel || 'WARN';
    // Default to all tests

    cmd.browser = cmd.browser || 'firefox';
    cmd.unitPath = path.resolve(cwd, cmd.unitPath || cmd.path || './unit');
    cmd.funcPath = path.resolve(cwd, cmd.funcPath || cmd.path || './func');
    if (cmd.reportFolder) {
        cmd.reportFolder = path.resolve(cwd, cmd.reportFolder);
    }

    if (cmd.unit) {
        arrowReportDir = cmd.reportFolder || cmd.unitPath;
    } else {
        arrowReportDir = cmd.reportFolder || cmd.funcPath + '/../..';
    }
    arrowReportDir = arrowReportDir + '/artifacts/arrowreport/';

    try {
        wrench.rmdirSyncRecursive(arrowReportDir);
    } catch (e) {}
    wrench.mkdirSyncRecursive(arrowReportDir);


    if (process.env.SELENIUM_HUB_URL) {
        remoteselenium = process.env.SELENIUM_HUB_URL;
        console.log('selenium host.....' + remoteselenium);
        series.push(gethostip);
    }

    if (cmd.arrow) {
        series.push(startArrowServer);
    }

    if (cmd.browser === 'phantomjs') {
        series.push(function (callback) {
            startPhantomjs(cmd, callback);
        });
    } else if (cmd.reuseSession) {
        if (cmd.selenium) {
            series.push(function (callback) {
                startArrowSelenium(cmd, callback);
            });
        }
    }

    if (!cmd.unit && !cmd.func) {
        console.log("---Run both unit and functinal tests with phantomjs---");
        cmd.unit = true;
        cmd.func = true;
    }

    if (cmd.unit) {
        series.push(function (callback) {
            runUnitTests(cmd, callback);
        });
    }
    if (cmd.func) {
        series.push(function (callback) {
            runFuncAppTests(cmd, callback);
        });
    }
    async.series(series, finalize);
}

program.command('test')
    .description('Run unit and functional tests')
    .option('-u, --unit', 'Run unit tests')
    .option('-f, --func', 'Run functional tests')
    .option('-s, --no-selenium', 'Don\'t run arrow_selenium')
    .option('-a, --no-arrow', 'Don\'t run arrow_server')
    .option('--debugApps', 'show STDOUT and STDERR from apps')
    .option('--logLevel <value>', 'Arrow logLevel')
    .option('--testName <value>', 'Arrow testName')
    .option('--descriptor <value>', 'which descriptor to run. filename (or glob) relative to --path')
    .option('--port <value>', 'port number to run app')
    .option('--coverage', 'Arrow code coverage')
    .option('--reuseSession', 'Arrow reuseSession')
    .option('--baseUrl <value>', 'Full app path including port if there is one to run arrow tests')
    .option('--group <value>', 'Arrow group')
    .option('--driver <value>', 'Arrow driver')
    .option('--seleniumHost <value>', 'Selenium host')
    .option('--browser <value>', 'Arrow browser')
    .option('--path <value>', 'Path to find the tests. defaults to ./func or ./unit')
    .option('--reportFolder <value>', 'Result dir. defaults to ./unit/artifact/ for unit, ./func/../../artifact/ for functional or all tests')
    .action(test);

// report how we're called, mainly to help debug CI environments
console.log(process.argv.join(' '));
console.log();

program.parse(process.argv);
