/*jslint anon:true, sloppy:true, nomen:true, indent: 4, white: false*/
/*global YUI*/
YUI.add('github', function (Y, NAME) {

/**
 * The github module.
 *
 * @module github
 */

        /**
         * Constructor for the Controller class.
         *
         * @class Controller
         * @constructor
         */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function (ac) {
            var view_type,
                yqlTable,
                yui,
                mojito,
                title,
                model = ac.models.get('yql'),
                id,
                repo,
                self = this;
            view_type = ac.params.getFromRoute('view_type') || "yui";

            if (view_type === "yui") {
                yqlTable = ac.config.getDefinition('yqlTable', '');
                yui = ac.config.getDefinition('yui', 'notitle');
                title = yui.title;
                id = yui.id;
                repo = yui.repo;
            } else if (view_type === "mojito") {
                yqlTable = ac.config.getDefinition('yqlTable', '');
                mojito = ac.config.getDefinition('mojito', 'notitle');
                title = mojito.title;
                id = mojito.id;
                repo = mojito.repo;
            }
            Y.log("github: model: ", "info", NAME);
            Y.log(model, "info", NAME);


            model.getData({}, yqlTable, id, repo, function (err, data) {
                //Y.log("github -index - model.getData:", "info", NAME);
                //Y.log(data, "info", NAME);
                var res = [];

                //Y.log("calling githubmap", "info", NAME);
                Y.log(data, "info", NAME);    

                // add mojit specific css
                ac.assets.addCss('./index.css');
                if(err) {
                    Y.log(err, "info", NAME);
                    ac.error(err);
                } else {
                    // Construct special data
                    res = self.githubMap(data);
                    // Populate and render template.
                    ac.done({
                        title: title,
                        results: res 
                    });
                }
            });
        },
        /**
         * Method for reading in the GitHub json data and returning
         *    the right link and title.
         *
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         * @param data {Array} The array of GitHub actions
         *
         */
        githubMap: function(data) {
            Y.log("githubmap called", "info", NAME);
            var res = [];
            Y.Array.each(data, function (itm, idx, arr) {
                var
                    type = itm.json.type,
                    username = itm.json.actor.login,
                    msg = "msg",
                    link = "http://www.yahoo.com";

                Y.log("github controller server type:" + type, "info", NAME);
                if (type === "IssueCommentEvent") {
                    Y.log("issuecommentevent!", "info", NAME);
                }
                switch (type) {
                case "CommitCommentEvent":
                    msg = "Made a Comment";
                    link = itm.json.payload.comment.html_url;
                    break;
                case "CreateEvent":
                    msg = "Created Something";
                    link = itm.json.payload.ref;
                    break;
                case "DeleteEvent":
                    msg = "Deleted Something";
                    link = itm.json.payload.ref;
                    break;
                case "DownloadEvent":
                    msg = "Downloaded Something";
                    link = itm.json.payload.download.html_url;
                    break;
                case "FollowEvent":
                    msg = "Followed Someone";
                    link = itm.json.payload.target.url;
                    break;
                case "ForkEvent":
                    msg = "Forked Something";
                    link = itm.json.payload.forkee.html_url;
                    break;
                case "GistEvent":
                    msg = "Acted on a Gist";
                    link = itm.json.payload.gist.html_url;
                    break;
                case "GollumEvent":
                    msg = "Acted on a Page";
                    if (itm.json.payload.pages instanceof Array) {
                        link = itm.json.payload.pages[0].html_url;
                    } else {
                        link = itm.json.payload.pages.html_url;
                    }
                    break;
                case "IssueCommentEvent":
                    Y.log(" inside case IssueCommentEvent!", "info", NAME);
                    msg = "Commented on an Issue";
                    link = itm.json.payload.comment.html_url;
                    break;
                case "IssuesEvent":
                    msg = "Acted on an Issue";
                    link = itm.json.payload.issue.html_url;
                    break;
                case "MemberEvent":
                    msg = "A member was added.";
                    link = itm.json.payload.member.html_url;
                    break;
                case "PublicEvent":
                    msg = "A Repo was made Public!";
                    link = "#";
                    break;
                case "PullRequestEvent":
                    msg = "Acted on a Pull Request";
                    link = itm.json.payload.pull_request.html_url;
                    break;
                case "PullRequestReviewCommentEvent":
                    msg = "Commented on a Pull Request";
                    link = itm.json.payload.comment.html_url;
                    break;
                case "PushEvent":
                    msg = "Pushed some code";
                    if (itm.json.payload.commits instanceof Array) {
                        link = "http://www.github.com/yui/yui3/commit/" + itm.json.payload.commits[0].sha;
                    } else {
                        link = "http://www.github.com/yui/yui3/commit/" + itm.json.payload.commits.sha;
                    }
                    break;
                case "TeamAddEvent":
                    msg = "Added someone to a team.";
                    link = itm.json.payload.user.url;
                    break;
                case "WatchEvent":
                    msg = "Had a Watch Event";
                    link = "#";
                    break;
                default:
                    msg = "Did Something? Don't know.";
                    link = "#";
                    break;
                }
                res[idx] = {
                    type: type,
                    username: username,
                    payload: itm.json.payload,
                    message: msg,
                    link: link
                };
            });
            // send the array back
            return res;
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon', 'mojito-params-addon', 'mojito-config-addon']});
