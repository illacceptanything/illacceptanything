
YUI.add('github-tests', function (Y, NAME) {

    var suite = new YUITest.TestSuite('github-tests'),
        controller = null,
        A = YUITest.Assert,
        model;

    suite.add(new YUITest.TestCase({

        name: 'github user tests',

        setUp: function () {
            controller = Y.mojito.controllers["github"];
            model = Y.mojito.models["stats-model-yql"];
        },
        tearDown: function () {
            controller = null;
        },
        'test mojit 001': function () {
            var ac,
                modelData,
                assetsResults,
                route_param,
                title,
                results,
                def_config =  {
                      "yui": {
                          "title" : "YUI GitHub Activity",
                          "id": "yui",
                          "repo": "yui3"
                      },
                      "mojito": {
                          "title" : "Mojito GitHub Activity",
                           "id": "yahoo",
                           "repo": "mojito"
                      }
                };
            ac = {
                assets: {
                    addCss: function (css) {
                        assetsResults = css;
                    }
                },
                config: {
                    getDefinition: function (key) {
                      return def_config[key];
                    }
                },
                params: {
                    getFromRoute: function (param) {
                        route_param = param;
                        return 'mojito';
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('yql', modelName, 'wrong model name');
                        return {
                            getData: function (params, feedURL, cb) {
                                return {
                                    onDataReturn: function (cb, result) {
                                       cb(result);
                                    }
                                }
                            }
                        };
                    }
                },
                done: function (data) {
                    title = data.title;
                    results = data.results;
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);
            A.isNotNull(route_param);



            // test githubMap

            A.isFunction(controller.githubMap);
            controller.githubMap(ac, [
                {
                    json: {
                        type: "CommitCommentEvent",
                        actor: {
                            login: "username"
                        },
                        payload:{
                            comment:{
                                html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "CreateEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            ref: "http://www.yahoo.com"
                        }
                    }
                },
                {
                    json: {
                        type: "DeleteEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            ref: "http://www.yahoo.com"
                        }
                    }
                },
                {
                    json: {
                        type: "DownloadEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            download: {
                                html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "FollowEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            target: {
                                url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "ForkEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            forkee: {
                                html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "GistEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            gist: {
                                html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "GollumEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            pages: [
                                {
                                    html_url: "http://www.yahoo.com"
                                }
                            ]
                        }
                    }
                },
                {
                    json: {
                        type: "GollumEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            pages: {
                                    html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "IssueCommentEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            comment: {
                                    html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "IssuesEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            issue: {
                                    html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "MemberEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            member: {
                                    html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "PublicEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            member: {
                                    html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "PullRequestEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            pull_request: {
                                    html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "PullRequestReviewCommentEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            comment: {
                                    html_url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "PushEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            commits: {
                                    sha: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "PushEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            commits: [{
                                    sha: "http://www.yahoo.com"
                            }]
                        }
                    }
                },
                {
                    json: {
                        type: "TeamAddEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            user: {
                                    url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "WatchEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            user: {
                                    url: "http://www.yahoo.com"
                            }
                        }
                    }
                },
                {
                    json: {
                        type: "someOtherEvent",
                        actor: {
                            login: "username"
                        },
                        payload: {
                            user: {
                                    url: "http://www.yahoo.com"
                            }
                        }
                    }
                }
            ]);


        },


       'test mojit 002': function () {
            var ac,
                modelData,
                assetsResults,
                route_param,
                title,
                results,
                def_config =  {
                      "yui": {
                          "title" : "YUI GitHub Activity",
                          "id": "yui",
                          "repo": "yui3"
                      },
                      "mojito": {
                          "title" : "Mojito GitHub Activity",
                           "id": "yahoo",
                           "repo": "mojito"
                      }
                };
            ac = {
                assets: {
                    addCss: function (css) {
                        assetsResults = css;
                    }
                },
                config: {
                    getDefinition: function (key) {
                      return def_config[key];
                    }
                },
                params: {
                    getFromRoute: function (param) {
                        route_param = param;
                        return 'yui';
                    }
                },
                models: {
                    get: function (modelName) {
                        A.areEqual('yql', modelName, 'wrong model name');
                        return {
                            getData: function (params, feedURL, cb) {
                                return {
                                    onDataReturn: function (cb, result) {
                                       cb(result);
                                    }
                                }
                            }
                        };
                    }
                },
                done: function (data) {
                    title = data.title;
                    results = data.results;
                }
            };

            A.isNotNull(controller);
            A.isFunction(controller.index);
            controller.index(ac);

            A.isNotNull(route_param);


        }

    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojito-test', 'github', 'stats-model-yql']});
