var gulp = require('gulp'),
    minimist = require('minimist'),
    summary = require('jshint-summary'),
    del = require('del'),
    plugins = require('gulp-load-plugins')();

var paths = {
    js: [
        'core/js/**/*.js',
        'modules/**/*.js'
    ],
    scss: [
        'core/scss/**/*.scss',
        'modules/**/*.scss'
    ],
    templates: [
        'build/js/templates/dropdown_template.js',
        'build/js/templates/file-input_template.js',
        'build/js/templates/text-field_template.js',
        'build/js/templates/search-filter_template.js',
        'build/js/templates/select_template.js',
        'build/js/templates/tabs_template.js',
        'build/js/templates/date-picker_template.js',
        'build/js/templates/progress_template.js'
    ],
    demo: [
        'demo/**/*',
        '!demo/scss/**/*',
        '!demo/scss'
    ],
    examples: [
        'modules/**/demo/**/*.html'
    ],
    libs: [
        'libs/**/*'
    ]
};

function watcherWithCache(name, src, tasks)
{
    var watcher = gulp.watch(src, tasks);

    watcher.on('change', function(event)
    {
        if (event.type === 'deleted')
        {
            delete plugins.cached.caches.scripts[event.path];
            plugins.remember.forget(name, event.path);
        }
    });
}

var knownOptions =
{
    string: 'version',
    default: { version: '' }
};

var options = minimist(process.argv.slice(2), knownOptions);

// Clean
gulp.task('clean:build', function(cb)
{
    del(['build/*'], cb);
});

gulp.task('clean:dist', function(cb)
{
    del(['dist/*'], cb);
});


// Develop
gulp.task('lint', function()
{
    return gulp.src(paths.js)
        .pipe(plugins.plumber())
        .pipe(plugins.cached('lint'))
        .pipe(plugins.jshint())
        .pipe(plugins.jshint.reporter('jshint-summary'))
        .pipe(plugins.jshint.reporter('fail'))
        .pipe(plugins.remember('lint'))
        .pipe(plugins.rename(function(path)
        {
            path.dirname = path.dirname.replace('/js', '');
        }))
        .pipe(gulp.dest('build/js'));
});

gulp.task('scss', function()
{
    return gulp.src('demo/scss/lumx.scss')
        .pipe(plugins.plumber())
        .pipe(plugins.rubySass())
        .pipe(gulp.dest('build'));
});

gulp.task('demo', function()
{
    return gulp.src(paths.demo)
        .pipe(plugins.plumber())
        .pipe(gulp.dest('build'));
});

gulp.task('examples', function()
{
    return gulp.src(paths.examples)
        .pipe(plugins.plumber())
        .pipe(plugins.rename(function(path)
        {
            path.dirname = path.dirname.replace('/demo', '');
        }))
        .pipe(gulp.dest('build/includes/modules'));
});

gulp.task('libs', function()
{
    return gulp.src(paths.libs)
        .pipe(plugins.plumber())
        .pipe(gulp.dest('build/libs'));
});


// Dist
gulp.task('scss:move-core', function()
{
    return gulp.src(paths.scss[0])
        .pipe(gulp.dest('dist/scss'));
});

gulp.task('scss:move-modules', function()
{
    return gulp.src(paths.scss[1])
        .pipe(plugins.rename(function(path)
        {
            path.dirname = '/';
        }))
        .pipe(gulp.dest('dist/scss/modules'));
});

gulp.task('scss:paths', ['scss:move-core', 'scss:move-modules'], function()
{
    return gulp.src(['dist/scss/_lumx.scss'])
        .pipe(plugins.plumber())
        .pipe(plugins.replace(/..\/..\/libs/g, '../../..'))
        .pipe(plugins.replace(/..\/..\/modules\/[^\/]*\/scss/g, 'modules'))
        .pipe(gulp.dest('dist/scss'));
});

gulp.task('dist:css', ['scss:paths'], function()
{
    return gulp.src(['core/scss/_lumx.scss'])
        .pipe(plugins.plumber())
        .pipe(plugins.rename('lumx.scss'))
        .pipe(plugins.rubySass())
        .pipe(plugins.minifyCss({ keepSpecialComments: 0 }))
        .pipe(plugins.insert.prepend('/*\n LumX ' + options.version + '\n (c) 2014-' + new Date().getFullYear() + ' LumApps http://ui.lumapps.com\n License: MIT\n*/\n'))
        .pipe(gulp.dest('dist'));
});

gulp.task('tpl:dropdown', function()
{
    return gulp.src('modules/dropdown/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'dropdown_template.js',
            moduleName: 'lumx.dropdown',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('tpl:file-input', function()
{
    return gulp.src('modules/file-input/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'file-input_template.js',
            moduleName: 'lumx.file-input',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('tpl:text-field', function()
{
    return gulp.src('modules/text-field/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'text-field_template.js',
            moduleName: 'lumx.text-field',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('tpl:search-filter', function()
{
    return gulp.src('modules/search-filter/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'search-filter_template.js',
            moduleName: 'lumx.search-filter',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('tpl:select', function()
{
    return gulp.src('modules/select/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'select_template.js',
            moduleName: 'lumx.select',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('tpl:tabs', function()
{
    return gulp.src('modules/tabs/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'tabs_template.js',
            moduleName: 'lumx.tabs',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('tpl:date-picker', function()
{
    return gulp.src('modules/date-picker/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'date-picker_template.js',
            moduleName: 'lumx.date-picker',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('tpl:progress', function()
{
    return gulp.src('modules/progress/views/*.html')
        .pipe(plugins.plumber())
        .pipe(plugins.templatecache({
            output: 'progress_template.js',
            moduleName: 'lumx.progress',
            strip: 'views/'
        }))
        .pipe(gulp.dest('build/js/templates'));
});

gulp.task('dist:scripts', ['tpl:dropdown', 'tpl:file-input', 'tpl:text-field', 'tpl:search-filter', 'tpl:select', 'tpl:tabs', 'tpl:date-picker', 'tpl:progress'], function()
{
    return gulp.src(paths.js.concat(paths.templates))
        .pipe(plugins.plumber())
        .pipe(plugins.concat('lumx.js'))
        .pipe(plugins.insert.prepend('/*\n LumX ' + options.version + '\n (c) 2014-' + new Date().getFullYear() + ' LumApps http://ui.lumapps.com\n License: MIT\n*/\n'))
        .pipe(gulp.dest('dist'))
        .pipe(plugins.uglify())
        .pipe(plugins.insert.prepend('/*\n LumX ' + options.version + '\n (c) 2014-' + new Date().getFullYear() + ' LumApps http://ui.lumapps.com\n License: MIT\n*/\n'))
        .pipe(plugins.rename('lumx.min.js'))
        .pipe(gulp.dest('dist'));
});

gulp.task('serve', ['watch'], function() {
    return plugins.connect.server({
        root: 'build'
    });
});

gulp.task('watch', ['build'], function()
{
    watcherWithCache('lint', paths.js, ['lint']);
    watcherWithCache('scss', [paths.scss, 'demo/scss/**/*.scss'], ['scss']);
    watcherWithCache('demo', paths.demo, ['demo']);
    watcherWithCache('examples', paths.examples, ['examples']);
    watcherWithCache('libs', paths.libs, ['libs']);
    watcherWithCache('tpl:dropdown', 'modules/dropdown/views/*.html', ['tpl:dropdown']);
    watcherWithCache('tpl:file-input', 'modules/file-input/views/*.html', ['tpl:file-input']);
    watcherWithCache('tpl:text-field', 'modules/text-field/views/*.html', ['tpl:text-field']);
    watcherWithCache('tpl:search-filter', 'modules/search-filter/views/*.html', ['tpl:search-filter']);
    watcherWithCache('tpl:select', 'modules/select/views/*.html', ['tpl:select']);
    watcherWithCache('tpl:tabs', 'modules/tabs/views/*.html', ['tpl:tabs']);
    watcherWithCache('tpl:date-picker', 'modules/date-picker/views/*.html', ['tpl:date-picker']);
    watcherWithCache('tpl:progress', 'modules/progress/views/*.html', ['tpl:progress']);
});

gulp.task('clean', ['clean:build', 'clean:dist']);

gulp.task('build', ['lint', 'scss', 'demo', 'examples', 'libs', 'tpl:dropdown', 'tpl:file-input', 'tpl:text-field', 'tpl:search-filter', 'tpl:select', 'tpl:tabs', 'tpl:date-picker', 'tpl:progress']);
gulp.task('dist', ['clean:dist'], function()
{
   gulp.start('dist:css');
   gulp.start('dist:scripts');
});

gulp.task('default', ['watch']);
