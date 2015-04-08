var path = require('canonical-path');
var Q = require('q');
var qfs = require('q-io/fs');
var _ = require('lodash');
var glob = require('glob');
var Minimatch = require("minimatch").Minimatch;
var StringMap = require('stringmap');

/**
 * @dgPackage readFilesProcessor
 *
 * @description Read documents from files and add them to the docs collection
 *
 * @property {string} basePath The path against which all paths to files are resolved
 *
 * @property {Array.<String|Object>} sourceFiles A collection of info about what files to read.
 *      If the item is a string then it is treated as a glob pattern. If the item is an object then
 *      it can have the following properties:
 *
 *      * `basePath` {string} the `relativeFile` property of the generated docs will be relative
 *        to this path. This path is relative to `readFileProcessor.basePath`.  Defaults to `.`.
 *      * `include` {string|Array<string>} glob pattern(s) of files to include (relative to `readFileProcessor.basePath`)
 *      * `exclude` {string|Array<string>} glob pattern(s) of files to exclude (relative to `readFileProcessor.basePath`)
 *      * `fileReader` {string} name of a file reader to use for these files
 *
 * @property {Array.<Function>} fileReaders A collection of file readers. A file reader is an object
 *                                          that has the following properties/methods:
 *      * `name` - the name of the file reader so that sourceFiles can reference it
 *      * `getDocs(fileInfo)` - this method is called to read the content of the file specified
 *         by the `fileInfo` object and return an array of documents.
 *      * `defaultPattern {Regex}` - a regular expression used to match to source files if no fileReader
 *         is specified in the sourceInfo item.
 *
 */
module.exports = function readFilesProcessor(log) {
  return {
    $validate: {
      basePath: { presence: true },
      sourceFiles: { presence: true },
      fileReaders: { presence: true },
    },
    $runAfter: ['reading-files'],
    $runBefore: ['files-read'],
    $process: function() {
      var fileReaders = this.fileReaders;
      var fileReaderMap = getFileReaderMap(fileReaders);
      var basePath = this.basePath;

      var sourcePromises = this.sourceFiles.map(function(sourceInfo) {

        sourceInfo = normalizeSourceInfo(basePath, sourceInfo);

        log.debug('Source Info:\n', sourceInfo);

        return getSourceFiles(sourceInfo).then(function(files) {

          var docsPromises = [];

          log.debug('Found ' + files.length + ' files:\n', files);

          files.forEach(function(file) {

            // Load up each file and extract documents using the appropriate fileReader
            var docsPromise = qfs.read(file).then(function(content) {

              // Choose a file reader for this file
              var fileReader = sourceInfo.fileReader ? fileReaderMap.get(sourceInfo.fileReader) : matchFileReader(fileReaders, file);

              log.debug('Reading File Content\nFile Path:', file, '\nFile Reader:', fileReader.name);

              var fileInfo = createFileInfo(file, content, sourceInfo, fileReader, basePath);

              var docs = fileReader.getDocs(fileInfo);

              // Attach the fileInfo object to each doc
              docs.forEach(function(doc) {
                doc.fileInfo = fileInfo;
              });

              return docs;
            });

            docsPromises.push(docsPromise);

          });
          return Q.all(docsPromises).then(_.flatten);
        });

      });
      return Q.all(sourcePromises).then(_.flatten);
    }
  };
};

function createFileInfo(file, content, sourceInfo, fileReader, basePath) {
  return {
    fileReader: fileReader.name,
    filePath: file,
    baseName: path.basename(file, path.extname(file)),
    extension: path.extname(file).replace(/^\./, ''),
    basePath: sourceInfo.basePath,
    relativePath: path.relative(sourceInfo.basePath, file),
    projectRelativePath: path.relative(basePath, file),
    content: content
  };
}


function getFileReaderMap(fileReaders) {
  var fileReaderMap = new StringMap();
  fileReaders.forEach(function(fileReader) {

    if ( !fileReader.name ) {
      throw new Error('Invalid File Reader: It must have a name property');
    }
    if ( typeof fileReader.getDocs !== 'function' ) {
      throw new Error('Invalid File Reader: "' + fileReader.name + '": It must have a getDocs property');
    }

    fileReaderMap.set(fileReader.name, fileReader);
  });
  return fileReaderMap;
}


function matchFileReader(fileReaders, file) {
  // We can't use fileReaders.find here because q-io overrides the es6-shim find() function
  var found = _.find(fileReaders, function(fileReader) {
    // If no defaultPattern is defined then match everything
    return !fileReader.defaultPattern || fileReader.defaultPattern.test(file);
  });
  if ( !found ) { throw new Error('No file reader found for ' + file); }
  return found;
}

/**
 * Resolve the relative include/exclude paths in the sourceInfo object,
 * @private
 */
function normalizeSourceInfo(basePath, sourceInfo) {

  if ( _.isString(sourceInfo) ) {
    sourceInfo = { include: [sourceInfo] };
  } else if ( !_.isObject(sourceInfo) || !sourceInfo.include) {

    throw new Error('Invalid sourceFiles parameter. ' +
      'You must pass an array of items, each of which is either a string or an object of the form ' +
      '{ include: "...", basePath: "...", exclude: "...", fileReader: "..." }');
  }

  if ( !_.isArray(sourceInfo.include) ) {
    sourceInfo.include = [sourceInfo.include];
  }
  sourceInfo.exclude = sourceInfo.exclude || [];
  if ( !_.isArray(sourceInfo.exclude) ) {
    sourceInfo.exclude = [sourceInfo.exclude];
  }

  sourceInfo.basePath = path.resolve(basePath, sourceInfo.basePath || '.');
  sourceInfo.include = _.map(sourceInfo.include, function(include) {
    return path.resolve(basePath, include);
  });
  sourceInfo.exclude = _.map(sourceInfo.exclude, function(exclude) {
    return path.resolve(basePath, exclude);
  });

  return sourceInfo;
}


function getSourceFiles(sourceInfo) {

  // Compute matchers for each of the exclusion patterns
  var excludeMatchers = _.map(sourceInfo.exclude, function(exclude) {
    return new Minimatch(exclude);
  });

  // Get a list of files to include
  var filesPromises = _.map(sourceInfo.include, function(include) {
    // Each call to glob will produce a array of file paths
    return Q.nfcall(glob, include);
  });

  return Q.all(filesPromises).then(function(filesCollections) {

    // Once we have all the file path arrays, flatten them into a single array
    return _.flatten(filesCollections);

  }).then(function(files) {

    // Filter the files on whether they match the `exclude` property and whether they are files
    var filteredFilePromises = files.map(function(file) {

      if ( _.any(excludeMatchers, function(excludeMatcher) { return excludeMatcher.match(file); }) ) {
        // Return a promise for `null` if the path is excluded
        // Doing this first - it is synchronous - saves us even making the isFile call if not needed
        return Q(null);
      } else {
        // Return a promise for the file if path is a file, otherwise return a promise for `null`
        return qfs.isFile(file).then(function(isFile) { return isFile ? file : null; });
      }
    });

    // Return a promise to a filtered list of files, those that are files and not excluded
    // (i.e. those that are not `null` from the previous block of code)
    return Q.all(filteredFilePromises).then(function(filteredFiles) {
      return filteredFiles.filter(function(filteredFile) { return filteredFile; });
    });
  });
}
