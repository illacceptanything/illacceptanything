'use strict';
var gutil = require('gulp-util');
var through = require('through');
var Stream = require('stream');
var utils = require('util');
var Readable = Stream.Readable;



module.exports = function (func) {
  
  if (!func || typeof func != 'function') {
    throw new gutil.PluginError('gulp-forEach', '`forEach` must be called with one parameter, a function');
  }
  
  var openStreams = [];
  var ended = false;
  
    
  function closeStreamIfNoMoreOpenStreams(stream){
    if(openStreams.length == 0){
      if(ended){
        stream.queue(null);
      }
    }
  }
  
  return through(function(data){
    
    if (data.isStream()) {
      this.emit('error', new gutil.PluginError('gulp-forEach', 'Streaming not supported'));
      return;
    }    
    
    var self = this;    
    var notYetRead = true;
    
    
    var readStream = new Readable({objectMode: true});
    readStream._read = function(){
      
      if(notYetRead){
        notYetRead = false;
        readStream.push(data);
      }else{
        readStream.push(null);
      }
    };
        
    var resultStream = func(readStream, data);
    
    if(resultStream){
    
      openStreams.push(resultStream);

      resultStream.on('end', function(){
        openStreams.splice(openStreams.indexOf(resultStream), 1);
        closeStreamIfNoMoreOpenStreams(self);
      });

      resultStream.on('data', function(result){
        self.queue(result);
      });
      
    }else{
      closeStreamIfNoMoreOpenStreams(self);
    }      
        
  }, function(){
    ended = true;
    closeStreamIfNoMoreOpenStreams(this);
  });
};
