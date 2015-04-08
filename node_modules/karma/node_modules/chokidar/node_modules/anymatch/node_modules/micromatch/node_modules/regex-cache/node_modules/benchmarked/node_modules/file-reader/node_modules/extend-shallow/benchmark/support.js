/**
 * Sources:
 *   - http://jsperf.com/array-unique2/15
 *
 */

function randoms(n) {
  var values = [];
  var rand;
  while (n--) {
    rand = Math.random() * n * 2 | 0;
    if (rand < values.length)
      values.push(values[rand]);
    else {
      switch (Math.random() * 3 | 0) {
      case 0:
        values.push(Math.random() * 1e10 | 0);
        break;
      case 1:
        values.push((Math.random() * 1e32).toString(36));
        break;
      case 2:
        values.push({});
        break;
      }
    }
  }
  return values;
}

var vals = randoms(10000);
