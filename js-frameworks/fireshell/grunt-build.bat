@echo off
IF not exist node_modules (npm install)
grunt build
