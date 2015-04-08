var disallowed = [
  '<<<<<<<',
  '=======',
  '>>>>>>>'
];

// returns undefined || an array of problem line numbers
module.exports = function (fileContents) {
  var lines = fileContents.split('\n');
  var errs;

  lines.forEach(function (line, num) {
    if (disallowed.indexOf(line.substr(0, 7)) !== -1) {
      errs = errs || [];
      errs.push(num + 1);
    }
  });

  return errs;
};
