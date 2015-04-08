// https://twitter.com/karangoel/status/585447100504150016

(function foo() {
  return function() {
    console.log.apply(console, arguments);
  };
})()('hello world')
