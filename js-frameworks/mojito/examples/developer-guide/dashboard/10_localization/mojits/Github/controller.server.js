/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('github', function(Y, NAME) {

/**
 * The github module.
 *
 * @module github
 */
    var githubMap = function (ac, data) {
        Y.log("githubmap called");
        var res = [];

        Y.Array.each(data, function (itm, idx, arr) {
            Y.log(itm);
            var
                type = itm.json.type,
                username = itm.json.actor.login,
                msg = "msg",
                link = "http://www.yahoo.com";

            Y.log("github controller server type:" + type);
            if (type === "IssueCommentEvent") {
                Y.log("issuecommentevent!");
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
                if (typeof itm.json.payload.pages === 'array') {
                    link = itm.json.payload.pages[0].html_url;
                } else {
                    link = itm.json.payload.pages.html_url;
                }
                break;
            case "IssueCommentEvent":
                Y.log(" inside case IssueCommentEvent!");
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
                if (typeof itm.json.payload.commits === 'array') {
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
    };
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
            var view_type, yqlTable, yui, mojito, title, id, repo, model = ac.models.get('yql');
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
            Y.log(model);
            model.getData({}, yqlTable, id, repo, function (data) {
                Y.log("github -index - model.getData:");
                Y.log(data);

                //construct special data

                var res = [];
                Y.log("calling githubmap");
                res = githubMap(ac, data);

                // add mojit specific css
                ac.assets.addCss('./index.css');
                ac.done({
                    title: title,
                    results: res
                });
            });
        }
    };
}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon', 'mojito-models-addon', 'mojito-config-addon', 'mojito-params-addon', 'mojito-helpers-addon']});
