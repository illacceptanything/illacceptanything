/// Dart has support for both asynchronous and synchronous generators.
import "dart:async";

Stream<int> read() async* {
  var i = 0;
  while (i < 5) {
    i++;
    yield i; // Pauses execution here, waits for receiver to iterate, asynchronously.
  }
}

Iterable<int> readSync() sync* {
  var i = 0;
  while (i < 5) {
    i++;
    yield i; // Pauses execution here, waits for receiver to iterate.
  }
}

Iterable<int> readMoreSync() sync* {
  yield* readSync(); // You can also use yield* to yield another iterator or stream.
  yield 10;
}

main() async {
  await for (var i in read()) {
    print("Asynchronous Generator: ${i}");
  }

  for (var i in readSync()) {
    print("Synchronous Generator: ${i}");
  }

  for (var i in readMoreSync()) {
    print("More Synchronous Generator: ${i}");
  }
}
