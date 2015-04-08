module.exports = function(grunt) {
  // Project configuration.
  grunt.initConfig({
    jshint: {
      files: ['src/**/*.js', 'test/src/**/*.js'],
      options: grunt.file.readJSON('./.jshintrc')
    },
    mochaTest: {
      test: {
      	options: {
          reporter: 'spec'        
        },
        src: ['test/src/**/*.js']
    	}
    },
    watch: {
      files: ['grunt.js', 'src/**/*.js', 'test/src/**/*.js'],
      tasks: ['default']
    }
	});

	grunt.loadNpmTasks('grunt-mocha-test');
	grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadNpmTasks('grunt-contrib-watch');

  grunt.registerTask('default', ['jshint', 'mochaTest']);
};
