![status](https://secure.travis-ci.org/wearefractal/gulp-jshint.png?branch=master)

## Information

<table>
<tr>
<td>Package</td><td>gulp-jshint</td>
</tr>
<tr>
<td>Description</td>
<td>JSHint plugin for gulp</td>
</tr>
<tr>
<td>Node Version</td>
<td>>= 0.4</td>
</tr>
</table>

## Usage

```javascript
var jshint = require('gulp-jshint');

gulp.task('lint', function() {
  gulp.src('./lib/*.js')
    .pipe(jshint())
    .pipe(jshint.reporter('YOUR_REPORTER_HERE'));
});
```

## Options

Plugin options:

- `fail`
  - Default is `false`
  - When `true` this will cause jshint to emit an error event on warnings which will exit the process with an error unless handled


You can pass in any other options and it passes them straight to JSHint. Look at their README for more info. You can also pass in the location of your jshintrc file as a string and it will load options from it.

## Results

Adds the following properties to the file object:

```javascript
  file.jshint.success = true; // or false
  file.jshint.errorCount = 0; // number of errors returned by JSHint
  file.jshint.results = []; // JSHint errors, see [http://jshint.com/docs/reporters/](http://jshint.com/docs/reporters/)
  file.jshint.data = []; // JSHint returns details about implied globals, cyclomatic complexity, etc
  file.jshint.opt = {}; // The options you passed to JSHint
```

## Reporters

### JSHint reporters

#### Built-in

You can choose any [JSHint reporter](https://github.com/jshint/jshint/tree/master/src/reporters)
when you call

```javascript
stuff
  .pipe(jshint())
  .pipe(jshint.reporter('default'))
```

#### External

Let's use [jshint-stylish](https://github.com/sindresorhus/jshint-stylish) as an example

```javascript
var stylish = require('jshint-stylish');

stuff
  .pipe(jshint())
  .pipe(jshint.reporter(stylish))
```

- OR -

```javascript
stuff
  .pipe(jshint())
  .pipe(jshint.reporter('jshint-stylish'))
```

JSHint plugins have no good module format so I tried to support all of them I saw in the wild. Hopefully it worked, but if a JSHint plugin isn't working with this library feel free to open an issue.

### Fail Reporter

Do you want the task to fail when a JSHint error happens? gulp-jshint includes a simple utility for this.

This example will log the errors using the stylish reporter, then fail if JSHint was not a success.

```js
stuff
  .pipe(jshint())
  .pipe(jshint.reporter('jshint-stylish'))
  .pipe(jshint.reporter('fail'))
```

### Custom Reporters

Custom reporters don't interact with this module at all. jshint will add some attributes to the file object and you can add a custom reporter downstream.

```javascript
var jshint = require('gulp-jshint');
var map = require('map-stream');

var myReporter = map(function (file, cb) {
  if (!file.jshint.success) {
    console.log('JSHINT fail in '+file.path);
    file.jshint.results.forEach(function (err) {
      if (err) {
        console.log(' '+file.path + ': line ' + err.line + ', col ' + err.character + ', code ' + err.code + ', ' + err.reason);
      }
    });
  }
  cb(null, file);
});

gulp.task('lint', function() {
  gulp.files('./lib/*.js')
    .pipe(jshint())
    .pipe(myReporter);
});
```

## LICENSE

(MIT License)

Copyright (c) 2013 Fractal <contact@wearefractal.com>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
