var util = require('util'),
	mocks = require('mocks'),
	EventEmitter = require('events').EventEmitter;

var childProcess = {
	cleanUp: function() {
	  process.removeAllListeners('mock:child_process:stdout:data');
	  process.removeAllListeners('mock:child_process:stderr:data');
	  process.removeAllListeners('mock:child_process:error');
	  process.removeAllListeners('mock:child_process:exit');	
	},

	spawn: function (cmd, args) {
		function std() {}

		function mockProcess(cmd, args) {
			var self = this;

			this.stdout = new std();
			this.stderr = new std();
			this.kill = function () {
				self.emit('exit');	
			};

			childProcess.cleanUp();

			process.on('mock:child_process:stdout:data', function(data) {
				self.stdout.emit('data', data);
			});

			process.on('mock:child_process:stderr:data', function(data) {
				self.stderr.emit('data', data);
			});

			process.on('mock:child_process:error', function(data) {
				self.emit('error', data);
			});

			process.on('mock:child_process:exit', function(data) {
				self.emit('exit', data);
			});
		}

		util.inherits(std, EventEmitter);
		util.inherits(mockProcess, EventEmitter);
		return new mockProcess(cmd, args);
	}		
};

var ServerResponse = function () {
	var response = new mocks.http.ServerResponse();
	response.pipe = function (stream) {
		process.nextTick(function () {
			stream.emit('finish');
		});
		return stream;
	}

	return response;
};

var ServerRequest = {
	get: function(url, callback) {
		ServerRequest.url = url;
		callback(ServerResponse());
	}
};

var fileSystem = mocks.fs.create({
  bin: {
  	darwin: {
      'BrowserStackLocal': 1
  	},
  	linux32: {
      'BrowserStackLocal': 1
  	},
  	linux64: {
      'BrowserStackLocal': 1
  	},
  	win32: {
      'BrowserStackLocal': 1
  	}
  }
});

function File() {
	this.close = function () {};
}
util.inherits(File, EventEmitter);

fileSystem.createWriteStream = function (fileName) {
	fileSystem.fileName = fileName;
	return new File();
};

fileSystem.createFileSync = function (fileName) {
	fileSystem.fileNameCreated = fileName;
};

fileSystem.chmod = function (fileName, mode, callback) {
	fileSystem.fileNameModded = fileName;
	fileSystem.mode = mode;
	callback();
};

function Extract() {
	this.on('finish', function() {
		this.emit('close');
	})
}
util.inherits(Extract, EventEmitter);

var unzip = {
	Extract: function (options) {
		unzip.dirName = options.path;
		return new Extract();
	}
};

var os = {
	_platform: 'unknown',
	_arch: 'unknown',
	platform: function () {
		return this._platform;
	},
	arch: function () {
		return this._arch;
	}
}

exports.childProcessMock = childProcess;
exports.httpMock = ServerRequest;
exports.fsMock = fileSystem;
exports.unzipMock = unzip;
exports.osMock = os;
