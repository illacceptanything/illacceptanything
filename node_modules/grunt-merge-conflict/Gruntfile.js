'use strict';
module.exports = function (grunt) {
  grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadTasks('tasks');

  grunt.initConfig({
    jshint: {
      options: {
        jshintrc: '.jshintrc'
      },
      all: [
        'Gruntfile.js',
        'lib/*.js',
        'tasks/*.js',
        'test/**/*.js'
      ]
    }
  });

  grunt.registerTask('default', ['jshint']);
};