# lumX

The first responsive front-end framework based on [AngularJS][angular] & [Google Material Design specifications][material]. lumX will help you to design your applications faster and easier. We followed Google Material Design recommendations to bring the best experience to your users.

## Quick start

Three quick start options are available:

- [Download the latest release][release].
- Clone the repo: git clone https://github.com/lumapps/lumx.git.
- Install with Bower: `bower install lumx`.

## Documentation

lumX's documentation, included in this repo in the demo directory, is built with [Gulp][gulp] and hosted on [Google App Engine][gae] at http://ui.lumapps.com. The docs may also be run locally.

### Running documentation locally

1. If necessary, install the [Python SDK of Google App Engine][gaepython].
2. In your environment, declare the variable `APPENGINE` with the path to your Google App Engine binaries.
For example, using bash in a Unix system with the default path will need to edit the `.bashrc` in your home to add:
    ```bash
    export APPENGINE=/home/USER/google-cloud-sdk/bin/
    ```
3. Run `npm install & bower install`.
4. Run ``./launch.sh`.
5. Visit http://localhost:8888 in your browser, and voilà.

#### or

1. Run `npm install & bower install`.
2. Run `gulp serve`.
3. Visit http://localhost:8888 in your browser, and voilà.

## How to get help, contribute, or provide feedback

Please refer to our [contributing guidelines](CONTRIBUTING.md). The roadmap is available in [this repo](ROADMAP.md).

## Copyright and license

Code and documentation copyright 2014 LumApps. Code released under the [MIT license](LICENSE.md).


[angular]: https://angularjs.org/
[gae]: https://cloud.google.com/appengine/
[gaepython]: https://cloud.google.com/appengine/downloads
[gulp]: http://gulpjs.com/
[material]: http://www.google.com/design/spec/material-design/introduction.html
[release]: https://github.com/lumapps/lumX/tags
