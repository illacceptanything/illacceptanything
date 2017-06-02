import { set } from "ember-metal/property_set";
import Stream from "ember-metal/streams/stream";
import KeyStream from "ember-views/streams/key_stream";

var source, object, count;

function incrementCount() {
  count++;
}

QUnit.module('KeyStream', {
  setup: function() {
    count = 0;
    object = { name: "mmun" };

    source = new Stream(function() {
      return object;
    });

    source.setValue = function(value) {
      object = value;
      this.notify();
    };
  },
  teardown: function() {
    count = undefined;
    object = undefined;
    source = undefined;
  }
});

QUnit.test("can be instantiated manually", function() {
  var nameStream = new KeyStream(source, 'name');
  equal(nameStream.value(), "mmun", "Stream value is correct");
});

QUnit.test("can be instantiated via `Stream.prototype.get`", function() {
  var nameStream = source.get('name');
  equal(nameStream.value(), "mmun", "Stream value is correct");
});

QUnit.test("is notified when the observed object's property is mutated", function() {
  var nameStream = source.get('name');
  nameStream.subscribe(incrementCount);

  equal(count, 0, "Subscribers called correct number of times");
  equal(nameStream.value(), "mmun", "Stream value is correct");

  set(object, 'name', "wycats");

  equal(count, 1, "Subscribers called correct number of times");
  equal(nameStream.value(), "wycats", "Stream value is correct");
});

QUnit.test("is notified when the source stream's value changes to a new object", function() {
  var nameStream = source.get('name');
  nameStream.subscribe(incrementCount);

  equal(count, 0, "Subscribers called correct number of times");
  equal(nameStream.value(), "mmun", "Stream value is correct");

  object = { name: "wycats" };
  source.setValue(object);

  equal(count, 1, "Subscribers called correct number of times");
  equal(nameStream.value(), "wycats", "Stream value is correct");

  set(object, 'name', "kris");

  equal(count, 2, "Subscribers called correct number of times");
  equal(nameStream.value(), "kris", "Stream value is correct");
});

QUnit.test("is notified when the source stream's value changes to the same object", function() {
  var nameStream = source.get('name');
  nameStream.subscribe(incrementCount);

  equal(count, 0, "Subscribers called correct number of times");
  equal(nameStream.value(), "mmun", "Stream value is correct");

  source.setValue(object);

  equal(count, 1, "Subscribers called correct number of times");
  equal(nameStream.value(), "mmun", "Stream value is correct");

  set(object, 'name', "kris");

  equal(count, 2, "Subscribers called correct number of times");
  equal(nameStream.value(), "kris", "Stream value is correct");
});

QUnit.test("is notified when setSource is called with a new stream whose value is a new object", function() {
  var nameStream = source.get('name');
  nameStream.subscribe(incrementCount);

  equal(count, 0, "Subscribers called correct number of times");
  equal(nameStream.value(), "mmun", "Stream value is correct");

  object = { name: "wycats" };
  nameStream.setSource(new Stream(function() {
    return object;
  }));

  equal(count, 1, "Subscribers called correct number of times");
  equal(nameStream.value(), "wycats", "Stream value is correct");

  set(object, 'name', "kris");

  equal(count, 2, "Subscribers called correct number of times");
  equal(nameStream.value(), "kris", "Stream value is correct");
});

QUnit.test("is notified when setSource is called with a new stream whose value is the same object", function() {
  var nameStream = source.get('name');
  nameStream.subscribe(incrementCount);

  equal(count, 0, "Subscribers called correct number of times");
  equal(nameStream.value(), "mmun", "Stream value is correct");

  nameStream.setSource(new Stream(function() {
    return object;
  }));

  equal(count, 1, "Subscribers called correct number of times");
  equal(nameStream.value(), "mmun", "Stream value is correct");

  set(object, 'name', "kris");

  equal(count, 2, "Subscribers called correct number of times");
  equal(nameStream.value(), "kris", "Stream value is correct");
});
