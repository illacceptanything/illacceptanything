module.exports = function(grunt) {
	var comment = '/*\n\tMatreshka v<%= pkg.version %> (<%= grunt.template.today("yyyy-mm-dd") %>)\n\tJavaScript Framework by Andrey Gubanov\n\tReleased under the MIT license\n\tMore info: http://matreshka.io\n*/\n'
	grunt.initConfig({
		pkg: grunt.file.readJSON('package.json'),
		requirejs: {
			compile: {
				options: {
					baseUrl: 'src',
					name: "matreshka",
					out: "matreshka.js",
					optimize: "none",
					preserveLicenseComments: false,
					paths: {
						matreshka_dir: ''
					},
					wrap: {
						start: comment,
						end: ';if(typeof define==="function"&&define.amd)define(["matreshka"],function(MK){return MK;});else if(typeof exports=="object")module.exports=Matreshka;'
					}
				}
			}
		},
		uglify: {
			options: {
				banner: comment,
				sourceMap: true,
				sourceMapName: 'matreshka.min.map',
			},
			build: {
				src: 'matreshka.js',
				dest: 'matreshka.min.js'
			}
		}
	});

	grunt.loadNpmTasks('grunt-contrib-uglify');
	grunt.loadNpmTasks('grunt-contrib-requirejs');
	grunt.registerTask('default', [ 'requirejs', 'uglify' ]);
};