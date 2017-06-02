define(["dojo", "dojo/require!dojo/hash,dojo/testsDOH/_base/loader/require/m2"], function(dojo){
  console.log("m1, a plain-old-style synch module wrapped in dojo/require!, evaluate start");
  dojo.provide("dojo.testsDOH._base.loader.require.m1");
  dojo.require("dojo.testsDOH._base.loader.require.m2");
  console.log("the value of m2 in m1 is: " + dojo.testsDOH._base.loader.require.m2);
  dojo.testsDOH._base.loader.require.m1 = "this is the value of m1";

  dojo.ready(function(){
    console.log("ready in m1 called");
  });
  console.log("m1 evaluate end");
});

