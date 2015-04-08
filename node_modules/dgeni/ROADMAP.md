# Roadmap

This document gives you a high level overview of where we are going with dgeni and what to
expect in future releases.  If you would like to get involved, take a look at the
[contributing][contributing] document.

Each release has an associated milestone that contains all the issues and PRs that must be
closed before we can publish that version.

## dgeni

The bare documentation generator tool.

### Releases
* [v0.3.1][dgeni-v0.3.1] - Bug fixes
* [v0.4.0][dgeni-v0.4.0] - no-config (should we push on and get di.js into this release?)
* [v0.5.0][dgeni-v0.5.0] - di.js (es6?) - this is up for discussion

### Additional Work
* Write guides for dgeni
* Generate API documentation for dgeni the tool

## dgeni-packages

The main store of dgeni packages containing all the processors and services that actually give
dgeni its document processing super-powers.

### Releases
* v0.9.4 - bug fixes
* v0.10.0 - no-config (to match the dgeni 0.4.0 release)
* v0.11.0 - packages to support easy documenting of angular apps
* v0.12.0
    - better jsdoc processing
    - coffeescript & es6 file support
    - better guide-style docs
    - i18n support
* v0.13.0 - di.js? to match (dgeni v0.5.0)

### Additional Work
* Write API docs for each package and processor/service therein
* Generate API documentation for each package

## dgeni-example

A very simple example of how to get dgeni up and running.

## Branches
* dgeni-0.3.0 - A version of the example app that works with dgeni 0.3.0
* master - works with dgeni 0.4.0

## dgeni-docs

A new project, which doesn't yet exist.  It will be a documentation container website app.
The idea is that you can generate partials and metadata using dgeni and upload it to an instance of
this web app. The web app will host the documentation that you have uploaded providing navigation,
searching and display of the hosted docs.

* It should be easily themeable/skinnable using templates and stylesheets.
* It should be able to host multiple versions of a project
* It should allow linking between components in different projects/versions

## angular.js docs

The angular.js docs website and the original consumer of dgeni.  We need to keep this updated with
the latest changes to dgeni.  Moving forward AngularJS V2 consists of multiple interdependent projects,
written in ES6, which will all need a website (see dgeni-docs) to display their documentation.


[contributing]: https://github.com/angular/dgeni/blob/master/CONTRIBUTING.md
[dgeni-v0.3.1]: https://github.com/angular/dgeni/issues?milestone=1&state=open
[dgeni-v0.4.0]: https://github.com/angular/dgeni/issues?milestone=2&state=open
[dgeni-v0.5.0]: https://github.com/angular/dgeni/issues?milestone=3&state=open
