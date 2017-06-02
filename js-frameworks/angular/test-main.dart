import 'package:guinness/guinness.dart';
import 'package:unittest/unittest.dart' as unit;
import 'package:angular2/src/dom/browser_adapter.dart';

import 'package:angular2/src/test_lib/test_lib.dart' show testSetup;

main() {
  BrowserDomAdapter.makeCurrent();
  unit.filterStacks = true;
  unit.formatStacks = false;
  unit.unittestConfiguration.timeout = new Duration(milliseconds: 100);

  _printWarnings();

  guinness.autoInit = false;
  guinness.initSpecs();

  testSetup();
}

_printWarnings () {
  final info = guinness.suiteInfo();

  if (info.activeIts.any((it) => it.exclusive)) {
    print("WARN: iit caused some tests to be excluded");
  }

  if (info.exclusiveDescribes.isNotEmpty) {
    print("WARN: ddescribe caused some tests to be excluded");
  }
}
