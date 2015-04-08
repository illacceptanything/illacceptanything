<?xml version="1.0" encoding="UTF-8"?>
<phpunit bootstrap="./vendor/autoload.php">
  <testsuites>
    <testsuite name="unit">
      <directory>./tests/Unit</directory>
    </testsuite>
    <testsuite name="integ">
      <directory>./tests/Integ</directory>
    </testsuite>
  </testsuites>
  <filter>
    <whitelist processUncoveredFilesFromWhitelist="true">
      <directory suffix=".php">./src</directory>
      <exclude>
        <file>src/hash_equals.php</file>
      </exclude>
    </whitelist>
  </filter>
</phpunit>
