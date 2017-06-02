var
	fs = require( "fs" ),
	shell = require( "shelljs" ),
	path = require( "path" ),

	cdnFolder = "dist/cdn",

	devFile = "dist/jquery.js",
	minFile = "dist/jquery.min.js",
	mapFile = "dist/jquery.min.map",

	releaseFiles = {
		"jquery-VER.js": devFile,
		"jquery-VER.min.js": minFile,
		"jquery-VER.min.map": mapFile
	},

	googleFilesCDN = [
		"jquery.js", "jquery.min.js", "jquery.min.map"
	],

	msFilesCDN = [
		"jquery-VER.js", "jquery-VER.min.js", "jquery-VER.min.map"
	];

/**
 * Generates copies for the CDNs
 */
function makeReleaseCopies( Release ) {
	shell.mkdir( "-p", cdnFolder );

	Object.keys( releaseFiles ).forEach(function( key ) {
		var text,
			builtFile = releaseFiles[ key ],
			unpathedFile = key.replace( /VER/g, Release.newVersion ),
			releaseFile = cdnFolder + "/" + unpathedFile;

		if ( /\.map$/.test( releaseFile ) ) {
			// Map files need to reference the new uncompressed name;
			// assume that all files reside in the same directory.
			// "file":"jquery.min.js","sources":["jquery.js"]
			text = fs.readFileSync( builtFile, "utf8" )
				.replace( /"file":"([^"]+)","sources":\["([^"]+)"\]/,
					"\"file\":\"" + unpathedFile.replace( /\.min\.map/, ".min.js" ) +
					"\",\"sources\":[\"" + unpathedFile.replace( /\.min\.map/, ".js" ) + "\"]" );
			fs.writeFileSync( releaseFile, text );
		} else if ( builtFile !== releaseFile ) {
			shell.cp( "-f", builtFile, releaseFile );
		}
	});
}

function makeArchives( Release, callback ) {

	Release.chdir( Release.dir.repo );

	function makeArchive( cdn, files, callback ) {
		if ( Release.preRelease ) {
			console.log( "Skipping archive creation for " + cdn + "; this is a beta release." );
			callback();
			return;
		}

		console.log( "Creating production archive for " + cdn );

		var sum,
			archiver = require( "archiver" )( "zip" ),
			md5file = cdnFolder + "/" + cdn + "-md5.txt",
			output = fs.createWriteStream(
				cdnFolder + "/" + cdn + "-jquery-" + Release.newVersion + ".zip"
			),
			rver = /VER/;

		output.on( "close", callback );

		output.on( "error", function( err ) {
			throw err;
		});

		archiver.pipe( output );

		files = files.map(function( item ) {
			return "dist" + ( rver.test( item ) ? "/cdn" : "" ) + "/" +
				item.replace( rver, Release.newVersion );
		});

		sum = Release.exec( "md5sum " + files.join( " " ), "Error retrieving md5sum" );
		fs.writeFileSync( md5file, sum );
		files.push( md5file );

		files.forEach(function( file ) {
			archiver.append( fs.createReadStream( file ),
				{ name: path.basename( file ) } );
		});

		archiver.finalize();
	}

	function buildGoogleCDN( callback ) {
		makeArchive( "googlecdn", googleFilesCDN, callback );
	}

	function buildMicrosoftCDN( callback ) {
		makeArchive( "mscdn", msFilesCDN, callback );
	}

	buildGoogleCDN(function() {
		buildMicrosoftCDN( callback );
	});
}

module.exports = {
	makeReleaseCopies: makeReleaseCopies,
	makeArchives: makeArchives
};
