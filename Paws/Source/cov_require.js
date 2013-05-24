module.exports = function(local_require){
   return function(){
      if (process.env.NODE_ENV === 'coverage') {
         arguments[0] = 
         arguments[0].replace(/.coffee$/, '.js') }
      return local_require.apply(new Object, arguments) } }
