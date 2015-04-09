#!/bin/bash

set -e

brew install portaudio

gem install bundler
bundle
