# jshint-stylish [![Build Status](https://travis-ci.org/sindresorhus/jshint-stylish.svg?branch=master)](https://travis-ci.org/sindresorhus/jshint-stylish)

> Stylish reporter for [JSHint](https://github.com/jshint/jshint)

![screenshot](screenshot.png)

Compared to the default reporter:

![default reporter](screenshot-default-formatter.png)


## Install

```sh
$ npm install --save-dev jshint-stylish
```


## Usage

### JSHint CLI

```sh
$ jshint --reporter node_modules/jshint-stylish/stylish.js file.js
```

### [gulp-jshint](https://github.com/wearefractal/gulp-jshint)

```js
gulp.task('default', function () {
	gulp.src(['file.js'])
		.pipe(jshint('.jshintrc'))
		.pipe(jshint.reporter('jshint-stylish'));
});
```

### [grunt-contrib-jshint](https://github.com/gruntjs/grunt-contrib-jshint)

```js
grunt.initConfig({
	jshint: {
		options: {
			reporter: require('jshint-stylish')
		},
		target: ['file.js']
	}
});

grunt.loadNpmTasks('grunt-contrib-jshint');
grunt.registerTask('default', ['jshint']);
```


## License

MIT © [Sindre Sorhus](http://sindresorhus.com)
