// Run some python
var execSync = require('child_process').execSync;
process.stdout.write(execSync('python -c "print(\'hello world\')"').toString());
