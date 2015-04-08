module.exports = function() {
  return {
    name: 'area',
    defaultFn: function(doc) {
      // Code files are put in the 'api' area
      // Other files compute their area from the first path segment
      return (doc.fileInfo.extension === 'js') ? 'api' : doc.fileInfo.relativePath.split('/')[0];
    }
  };
};