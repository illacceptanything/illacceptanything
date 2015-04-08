# Contributing to Dgeni
While dgeni is amazing we need help keeping it going and make it better that it already is. Here's how you can help!

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Making Changes](#making-changes)
- [Coding Conventions](#coding-conventions)
- [Submission Guidelines](#submission-guidelines)
- [Additional Resources](#additional-resources)


## Code of Conduct
Help us keep dgeni open and inclusive. Please read and follow our [Code of Conduct][coc].


## Getting Started

1. Fork the repository
2. Create a topic branch off `master`
2. Run the tests
	1. `npm install`
	2. `npm test`
3. Good to go! Head over to the [Roadmap](roadmap) to see what is coming up.
   Or submit a [feature request or bug](issues).

[The dgeni project alone is not that useful - we probably need to provide information about getting
dgeni-packages and dgeni-example then linking them with npm for development.]

## Found a bug?

Please open an [issue in Github](issues) for the relevant project. Problems with specific processors are more
likely to be related to issues in the [dgeni-packages project](dgeni-packages).

If you are feeling confident then you could submit a Pull Request with a fix. See [Making Changes](#making-changes) below.

##Making Changes

Before you make a change to dgeni, check that there is not already a pull request in the pipeline.
If the change is reasonably large then it is best to discuss it in a [GitHub Issue](issue) to ensure
that you hard work will not be wasted.

Any changes to dgeni must follow our coding conventions, be accompanied by appropriate unit tests and documentation. You must also sign our Contributor License Agreement.


### Coding Conventions

To ensure consistency throughout the source code, keep these rules in mind as you are working:

* All features or bug fixes **must be tested** by one or more unit tests.
* All public API methods **must be documented**.
* We generally follow the rules contained in [Google's JavaScript Style Guide][js-style-guide]:
* Wrap all code at **100 characters**.


### Signing the CLA

Please sign our Contributor License Agreement (CLA) before sending pull requests. For any code
changes to be accepted, the CLA must be signed. It's a quick process, we promise!

* For individuals we have a [simple click-through form][individual-cla].
* For corporations we'll need you to
  [print, sign and one of scan+email, fax or mail the form][corporate-cla].

### Git Commit Guidelines

We have very precise rules over how our git commit messages can be formatted.  This leads to **more
readable messages** that are easy to follow when looking through the **project history**.  But also,
we use the git commit messages to generate the [CHANGELOG](changelog).

#### Commit Message Format
Each commit message consists of a **header**, a **body** and a **footer**.  The header has a special
format that includes a **type**, a **scope** and a **subject**:

```
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

Any line of the commit message cannot be longer 100 characters! This allows the message to be easier
to read on github as well as in various git tools.

#### Type
Must be one of the following:

* **feat**: A new feature
* **fix**: A bug fix
* **docs**: Documentation only changes
* **style**: Changes that do not affect the meaning of the code (white-space, formatting, missing
  semi-colons, etc)
* **refactor**: A code change that neither fixes a bug or adds a feature
* **perf**: A code change that improves performance
* **test**: Adding missing tests
* **chore**: Changes to the build process or auxiliary tools and libraries such as documentation
  generation

#### Scope
The scope could be anything specifying place of the commit change. For example `Dgeni`,
`log`, `Package`, `readFilesProcessor`, `templateFinder`, etc...

#### Subject
The subject contains succinct description of the change:

* use the imperative, present tense: "change" not "changed" nor "changes"
* don't capitalize first letter
* no dot (.) at the end

#### Body
Just as in the **subject**, use the imperative, present tense: "change" not "changed" nor "changes"
The body should include the motivation for the change and contrast this with previous behavior.

#### Footer
The footer should contain any information about **Breaking Changes** and is also the place to
reference GitHub issues that this commit **Closes**.


A detailed explanation can be found in this [document][commit-message-format].

### Signing the CLA

Please sign our Contributor License Agreement (CLA) before sending pull requests. For any code
changes to be accepted, the CLA must be signed. It's a quick process, we promise!

* For individuals we have a [simple click-through form][individual-cla].
* For corporations we'll need you to
  [print, sign and one of scan+email, fax or mail the form][corporate-cla].

# Additional Resources</a>

* [General GitHub documentation](http://help.github.com/)
* [GitHub pull request documentation](http://help.github.com/send-pull-requests/)
* [Angular.js Contributing Doc](https://github.com/angular/angular.js/blob/CONTRIBUTING.md)

[roadmap]: http://github.com/angular/dgeni/blob/master/ROADMAP.md
[issues]: https://github.com/angular/dgeni/issues?state=open
[dgeni-packages]: https://github.com/angular/dgeni-packages
[js-style-guide]: http://google-styleguide.googlecode.com/svn/trunk/javascriptguide.xml
[changelog]: https://github.com/angular/dgeni/blob/master/CHANGELOG.md
[commit-message-format]: https://docs.google.com/document/d/1QrDFcIiPjSLDn3EL15IJygNPiHORgU1_OOAqWjiDU5Y/edit#
[corporate-cla]: http://code.google.com/legal/corporate-cla-v1.0.html
[individual-cla]: http://code.google.com/legal/individual-cla-v1.0.html
[coc]: https://github.com/angular/code-of-conduct/blob/master/CODE_OF_CONDUCT.md
