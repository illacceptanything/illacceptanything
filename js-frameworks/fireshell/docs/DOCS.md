# FireShell Docs

## Project setup and Grunt installation
FireShell utilises open source components running on the Terminal/command-line for it's workflow, you'll need to install Node and Grunt. Here's a walkthrough of how to get a project up and running in minutes. Once Node and Grunt are installed all future projects running Grunt are instant.

1. Install [Node.js](http://nodejs.org/download), [Sass](http://sass-lang.com/tutorial.html) and [Git](http://git-scm.com) on your machine. If you're a Windows user you'll also need to install [Ruby](http://rubyinstaller.org/downloads).
2. [Install Grunt](http://gruntjs.com/getting-started) using `npm install -g grunt-cli`. You may need to use `sudo` in front of the Grunt install command to give it permissions. For Windows tips with Grunt checkout their [FAQs](http://gruntjs.com/frequently-asked-questions).
3. Fork/Clone/Download the FireShell repository into your machine, you should hopefully see all the files and folders.
4. Navigate to the `grunt-dev.command` file and double-click it. This will open the Terminal and install the necessary `node_modules` folder, which are FireShell's dependencies. The `grunt-dev.command` file includes a `sudo` prefix so you'll need to enter your password to install.
5. The `grunt-dev.command` should install all the dependencies, which you can check back to see in your folder, and then run the commands associated with FireShell, and automatically open a new FireShell project running on `localhost:9000`.
6. From now on, just double-click the `grunt-dev.command` file to automatically run FireShell's Grunt tasks, it's setup using the following script to automatically `cd` you into the correct directory and run the necessary commands:

````sh
cd "$(dirname "$0")"
if [ ! -d node_modules ];then
    sudo npm install
fi
grunt
````

## How to use FireShell
Using FireShell is very easy, it's based on an easy philosphy of keeping things simple so that anybody can use it, even with zero experience on the command-line. FireShell uses Grunt to manage all the essential tasks for building with the web.

### Scaffolding
FireShell's scaffolding is lightweight and super easy. It takes into account a build directory of which you'll compile all your necessary code into. It keeps precious development files (raw `.scss` and `.js`) out of deployment, with a view that you'll be deploying just the contents of the `app` folder onto the server.

Once running, FireShell does the following:

1. Mounts the `app` folder onto a local server
2. Listens for changes inside the `src` directory, and compiles the necessary files into the `app` directory, which will then automaticaly livereload or inject changes. CSS changes are injected, all other changes force a page reload.

### Developing
Double-click the `grunt-dev.command` file and get developing, Grunt will report any errors with your code back to you on the command-line, even the line number. All CSS and JavaScript is uncompressed in development.

### Deploying
FireShell ships with a preconfigured build task for Grunt, just fire up the `grunt-build.command` file and your `src` directory files will be compiled into the `app` folder, but this time they're minified and ready to push onto a server environment.

### Gruntfile.js
One of the main features of FireShell, setup with dynamic variable names to make it even easier to use, here's where you'll need to edit to add more scripts to be run through Grunt:

````js
/**
 * Set project info
 */
project: {
  src: 'src',
  app: 'app',
  assets: '<%= project.app %>/assets',
  css: [
    '<%= project.src %>/scss/style.scss'
  ],
  js: [
    '<%= project.src %>/js/*.js'
  ]
}
````

For instance, if you're using a bunch of jQuery plugins, we can import a `/plugins/` folder into FireShell. It shouldn't matter the order you import them in the folder so we can use * to import all, though the order you specify the array will be the order of scripts import, you'll likely want to include your custom `scripts.js` last if you're instantiating plugins as they'll need to be loaded first:

````js
/**
 * Set project info
 */
project: {
  src: 'src',
  app: 'app',
  assets: '<%= project.app %>/assets',
  css: [
    '<%= project.src %>/scss/style.scss'
  ],
  js: [
    '<%= project.src %>/js/plugins/*.js',
    '<%= project.src %>/js/polyfills.js',
    '<%= project.src %>/js/scripts.js'
  ]
}
````

These scripts will be dynamically assigned throughout the rest of the Grunt configuration leaving it really easy and modular, just change file names once and it applies throughout. CSS files are best left as they are, and using `@import` inside your master `style.scss` file to import the correct files.

All Grunt dependencies inside FireShell's `Gruntfile.js` come with the URI to the repository where it's possible to customise the project further. Inline comments are also available for brief insights as to what each section does for beginners wanting to learn Grunt.

### Dynamic copyright/project banners
The package.json includes the dependencies for the project as well as information about the project. Entries here will be dynamically appended to the top of generated `.css` and `.js` files, by default it ships with FireShell's banner:

````js
/*!
 * FireShell
 * Fiercely quick and opinionated front-ends
 * http://getfireshell.com
 * @author Todd Motto
 * @version 1.0.0
 * Copyright 2013. MIT licensed.
 */
````

### Livereloading
Grunt's Livereload will inject the following script into your HTML for you (not included when you deploy):

````html
<!-- livereload script -->
<script type="text/javascript">document.write('<script src="http://'
 + (location.host || 'localhost').split(':')[0]
 + ':35729/livereload.js?snipver=1" type="text/javascript"><\/script>')
</script>
````

You can navigate to the `watch` portion of the Grunt configuration and specify which files you'd like to livereload once they're changed.

### Extending Grunt tasks
If you're including more Grunt tasks in your project, remember to use the `npm install <grunt package> --save-dev` inside your Terminal so that it gets added to your `package.json` file for future dependencies.

Add new tasks to either the default `grunt` task or `grunt build` task at the end of the `Gruntfile.js`:

````js
/**
 * Default task
 * Run `grunt` on the command line
 */
grunt.registerTask('default', [
  'sass:dev',
  'cssmin:dev',
  'bower:dev',
  'autoprefixer:dev',
  'jshint',
  'concat:dev',
  'connect:livereload',
  'open',
  'watch'
]);
````

## JavaScript
FireShell comes with a single `scripts.js` to get you started, of course if you're building an AngularJS project or other type you're going to need to customise the structure, but this gets you started. The generic scripts file ships with an immediately-invoked function expression (IIFE):

````js
(function ($, window, document, undefined) {
  'use strict';
  // FireShell
})(jQuery, window, document);
````

This helps with all your minification and not polluting with global variables, for instance before minification you've got very readable code and variable names (including the `document` and `window` objects):

````js
(function ($, window, document, undefined) {
  'use strict';
  var test = document.createElement('script');
})(jQuery, window, document);
````

When minified will be as follows, reducing many instances of the :

````js
(function (a,b,c,d) {
  'use strict';
  // Also not global
  var test = a.createElement('script');
})(jQuery,window,document);
````

Thus saving many bytes and reducing file size and performance, as well as keeping the global namespace clean. Passing in the `jQuery` object and giving it the dollar alias also makes it play nicely if you're including other frameworks that use the `$` namespace.

## Why just style.min.css and scripts.min.js?
Including only two of your custom CSS and JavaScript files in your HTML aligns with best practices in modern web development, minifying your code and limiting HTTP requests is a huge performance enhancer.

## Sass/SCSS setup
FireShell comes with a `.scss` file setup and existing `@import` declarations to the very common web components. FireShell hopes to help those out who aren't sure about structuring a CSS project confidently as well as getting them setup with using a CSS PreProcessor. The basic idea:

* `mixins` holds all Sass/SCSS mixins, FireShell ships with a few helpers
* `module` holds modules, more Object-Orientated components and a generic `app.scss` for everything else, all file names should be modular/OO.
* `partials` holds the blueprints for the project, the header, footer, sidebar and so on.
* `vendor` holds any files that are third party, such as the font awesome icons CSS
* `style.scss` imports all the necessary files from the above folders, when adding new files be sure to add it inside this file.

## Hidden files explained

It's a good idea to expose hidden files so you can configure your `.editorconfig`, `.jshintrc`, `.gitignore` files. On the command line, enter:

````
defaults write com.apple.Finder AppleShowAllFiles YES
````

To hide hidden files enter:

````
defaults write com.apple.Finder AppleShowAllFiles NO
````

### .editorconfig
EditorConfig helps developers define and maintain consistent coding styles between different editors and IDEs. The `.editorconfig` file consists of a format for defining coding styles and a collection of text editor plugins that enable editors to read the file format and adhere to defined styles.

### .gitignore
Ignores minified and generated files, this is best for working in teams to avoid constant conflict, only the source files are needed.

### .travis.yml
This is used on [travis-ci.org](http://travis-ci.org) for continuous integration tests, which monitor the FireShell build.

## Platform support

FireShell runs on both Mac OS X, Linux and Windows. Automated command-line scripts are only supported on Mac OS X and Windows.
