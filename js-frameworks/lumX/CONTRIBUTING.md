# Contributing to lumX

We'd love for you to contribute to our source code and to make AngularJS even better than it is today! Here are the guidelines we'd like you to follow:

- [Code of Conduct](#code-of-conduct)
- [Got a question or a problem?](#got-a-question-or-a-problem-)
- [Found an issue?](#found-an-issue-)
- [Want a feature?](#want-a-feature-)
- [Submission guidelines](#submission-guidelines)
- [Coding rules](#coding-rules)
- [Git commit guidelines](#git-commit-guidelines)

## <a name="code-of-conduct"></a> Code of Conduct

As heavy users of [Angular](https://github.com/angular/angular.js), we encourage you to read and follow the [Angular Code of Conduct](https://github.com/angular/code-of-conduct/blob/master/CODE_OF_CONDUCT.md).

## <a name="got-a-question-or-a-problem-"></a> Got a question or a problem?

If you have questions about how to use AngularJS, please direct these to [StackOverflow](http://stackoverflow.com/questions/tagged/lumx).

## <a name="found-an-issue-"></a> Found an issue?

If you find a bug in the source code or a mistake in the documentation, you can help us by submitting an issue to our [GitHub Repository](https://github.com/lumapps/lumx/issues). Even better you can submit a Pull Request with a fix.

Please see the Submission Guidelines below.

## <a name="want-a-feature-"></a> Want a feature?

You can request a new feature by submitting an issue to our GitHub Repository. If you would like to implement a new feature then consider what kind of change it is, discuss it with us before hand in your issue, so that we can better coordinate our efforts, prevent duplication of work, and help you to craft the change so that it is successfully accepted into the project.

## <a name="submission-guidelines"></a> Submission guidelines

### Submitting an issue

Before you submit your issue search the archive, maybe your question was already answered.

If your issue appears to be a bug, and hasn't been reported, open a new issue. Help us to maximize the effort by not reporting duplicate issues. Providing the following information will increase the chances of your issue being dealt with quickly:

- **Motivation for or Use Case** - explain why this is a bug for you
- **lumX Version(s)** - is it a regression?
- **Browsers and Operating System** - is this a problem with all browsers or only IE8?
- **Reproduce the Error** - provide a live example (using [Plunker](http://plnkr.co/edit) or [JSFiddle](http://jsfiddle.net/)) or a unambiguous set of steps.

### Submitting a pull request
Before you submit your pull request consider the following guidelines:

* Search [GitHub](https://github.com/lumapps/lumx/pulls) for an open or closed Pull Request
that relates to your submission. You don't want to duplicate effort.
* Make your changes in a new git branch

```shell
git checkout -b my-fix-branch master
```

* Create your patch.
* Follow our [Coding Rules](#rules).
* Commit your changes using a descriptive commit message that follows our
[commit message conventions](#commit-message-format).
* Build your changes locally.

```shell
bower dist
```

* Push your branch to GitHub:

```shell
git push origin my-fix-branch
```

* In GitHub, send a pull request to `lumx:master`.
* If we suggest changes then
* Make the required updates.
* Rebase your branch and force push to your GitHub repository (this will update your Pull Request):

```shell
git rebase master -i
git push -f
```

That's it! Thank you for your contribution!

#### After your pull request is merged

After your pull request is merged, you can safely delete your branch and pull the changes from the main (upstream) repository:

* Delete the remote branch on GitHub either through the GitHub web UI or your local shell as follows:

```shell
git push origin --delete my-fix-branch
```

* Check out the master branch:

```shell
git checkout master -f
```

* Delete the local branch:

```shell
git branch -D my-fix-branch
```

* Update your master with the latest upstream version:

```shell
git pull --ff upstream master
```

## <a name="coding-rules"></a> Coding rules

We're using Javascript and [SCSS](http://sass-lang.com/) to build the framework. A [Gulpfile](http://gulpjs.com/) is in the project to help you contribute:

- `gulp` will build the project and watch for modifications.
- `gulp dist` will update the dist folder with the last contents.

The dist folder must be update inside your pull requests. If there is conflicts, just ignore them as the dist folder **must** be only the result of the generation.

Our coding convention is the following:

- 4 spaces for indentation, for Javascript and SCSS.
- Wrap all codes at 120 characters.
- For Javascript:
- Use camel-case.
- Use the [Allman style](http://en.wikipedia.org/wiki/Indent_style#Allman_style).
- For SCSS
- Except for the line wrap, please refer to [the Harry Roberts css guidelines](http://cssguidelin.es/).
- For the CSS properties, we follow the [concentric order](http://rhodesmill.org/brandon/2011/concentric-css/)

## <a name="git-commit-guidelines"></a> Git commit guidelines

We have very precise rules over how our git commit messages can be formatted.  This leads to **more readable messages** that are easy to follow when looking through the **project history**.

### Commit message format

Each commit message consists of a **header**, a **body** and a **footer**.  The header has a special format that includes a **type**, a **scope** and a **subject**:

```
<type> <scope>: <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

Any line of the commit message cannot be longer 100 characters! This allows the message to be easier to read on github as well as in various git tools.

### Type

Must be one of the following:

* **feat**: A new feature
* **fix**: A bug fix
* **docs**: Documentation only changes
* **style**: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc)
* **refactor**: A code change that neither fixes a bug or adds a feature
* **perf**: A code change that improves performance
* **chore**: Changes to the build process or auxiliary tools and libraries such as distribution generation

### Scope

The scope could be anything specifying place of the commit change. For example `notification', 'dropdown', etc.

### Body

The body should include the motivation for the change and contrast this with previous behavior.

### Footer

The footer should contain any information about **Breaking Changes** and is also the place to reference GitHub issues that this commit **Closes**.
The breaking changes must be at the end of the commit with only on "BROKEN:" before the list of breaking changes. They must be each on a new line.

### Commit example

```
feat TOTO: TOTO for all

Before we had to do XXX. There was this and this problem. Now, by using TOTO, it's simpler and the problems are managed.

Closes PR #25
Fix #15
BROKEN:
first thing broken
second thing broken
```
