@ECHO OFF
SET BIN_TARGET=%~dp0/../phpspec/phpspec/bin/phpspec
php "%BIN_TARGET%" %*
