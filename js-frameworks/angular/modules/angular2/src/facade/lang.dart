library angular.core.facade.lang;

export 'dart:core' show Type, RegExp, print, DateTime;
import 'dart:math' as math;
import 'dart:convert' as convert;

class Math {
  static final _random = new math.Random();
  static int floor(num n) => n.floor();
  static double random() => _random.nextDouble();
}

class CONST {
  const CONST();
}
class ABSTRACT {
  const ABSTRACT();
}
class IMPLEMENTS {
  final interfaceClass;
  const IMPLEMENTS(this.interfaceClass);
}

bool isPresent(obj) => obj != null;
bool isBlank(obj) => obj == null;
bool isString(obj) => obj is String;
bool isFunction(obj) => obj is Function;

String stringify(obj) => obj.toString();

class StringWrapper {
  static String fromCharCode(int code) {
    return new String.fromCharCode(code);
  }

  static int charCodeAt(String s, int index) {
    return s.codeUnitAt(index);
  }

  static List<String> split(String s, RegExp regExp) {
    var parts = <String>[];
    var lastEnd = 0;
    regExp.allMatches(s).forEach((match) {
      parts.add(s.substring(lastEnd, match.start));
      lastEnd = match.end;
      for (var i = 0; i < match.groupCount; i++) {
        parts.add(match.group(i + 1));
      }
    });
    parts.add(s.substring(lastEnd));
    return parts;
  }

  static bool equals(String s, String s2) {
    return s == s2;
  }

  static String replace(String s, Pattern from, String replace) {
    return s.replaceFirst(from, replace);
  }

  static String replaceAll(String s, RegExp from, String replace) {
    return s.replaceAll(from, replace);
  }

  static startsWith(String s, String start) {
    return s.startsWith(start);
  }

  static String substring(String s, int start, [int end]) {
    return s.substring(start, end);
  }

  static String replaceAllMapped(String s, RegExp from, Function cb) {
    return s.replaceAllMapped(from, cb);
  }

  static bool contains(String s, String substr) {
    return s.contains(substr);
  }
}

class StringJoiner {
  final List<String> _parts = <String>[];

  void add(String part) {
    _parts.add(part);
  }

  String toString() => _parts.join("");
}

class NumberWrapper {
  static String toFixed(num n, int fractionDigits) {
    return n.toStringAsFixed(fractionDigits);
  }

  static bool equal(num a, num b) {
    return a == b;
  }

  static int parseIntAutoRadix(String text) {
    return int.parse(text);
  }

  static int parseInt(String text, int radix) {
    return int.parse(text, radix: radix);
  }

  static double parseFloat(String text) {
    return double.parse(text);
  }

  static double get NaN => double.NAN;

  static bool isNaN(num value) => value.isNaN;

  static bool isInteger(value) => value is int;
}

class RegExpWrapper {
  static RegExp create(regExpStr, [String flags = '']) {
    bool multiLine = flags.contains('m');
    bool caseSensitive = !flags.contains('i');
    return new RegExp(regExpStr, multiLine: multiLine, caseSensitive: caseSensitive);
  }
  static Match firstMatch(RegExp regExp, String input) {
    return regExp.firstMatch(input);
  }
  static Iterator<Match> matcher(RegExp regExp, String input) {
    return regExp.allMatches(input).iterator;
  }
}

class RegExpMatcherWrapper {
  static _JSLikeMatch next(Iterator<Match> matcher) {
    if (matcher.moveNext()) {
      return new _JSLikeMatch(matcher.current);
    }
    return null;
  }
}

class _JSLikeMatch {
  Match _m;

  _JSLikeMatch(this._m);

  String operator[](index) => _m[index];
  int get index => _m.start;
  int get length => _m.groupCount + 1;
}

class FunctionWrapper {
  static apply(Function fn, posArgs) {
    return Function.apply(fn, posArgs);
  }
}

class BaseException extends Error {
  final String message;

  BaseException(this.message);

  String toString() {
    return this.message;
  }
}

const _NAN_KEY = const Object();

// Dart can have identical(str1, str2) == false while str1 == str2
bool looseIdentical(a, b) =>
    a is String && b is String ? a == b : identical(a, b);

// Dart compare map keys by equality and we can have NaN != NaN
dynamic getMapKey(value) {
  if (value is! num) return value;
  return value.isNaN ? _NAN_KEY : value;
}

dynamic normalizeBlank(obj) {
  return isBlank(obj) ? null : obj;
}

bool isJsObject(o) {
  return false;
}

bool assertionsEnabled() {
  try {
    assert(false);
    return false;
  } catch (e) {
    return true;
  }
}

// Can't be all uppercase as our transpiler would think it is a special directive...
class Json {
  static parse(String s) => convert.JSON.decode(s);
  static String stringify(data) => convert.JSON.encode(data);
}

class DateWrapper {
  static DateTime fromMillis(int ms) {
    return new DateTime.fromMillisecondsSinceEpoch(ms);
  }
  static int toMillis(DateTime date) {
    return date.millisecondsSinceEpoch;
  }
  static DateTime now() {
    return new DateTime.now();
  }
  static toJson(DateTime date) {
    return date.toUtc().toIso8601String();
  }
}
