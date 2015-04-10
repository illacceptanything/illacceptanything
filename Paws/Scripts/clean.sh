#!/usr/bin/env sh

mv node_modules{,-PRECLEAN}/
mv .coveralls.yml{,-PRECLEAN}
git clean -Xdf
mv .coveralls.yml{-PRECLEAN,}
mv node_modules{-PRECLEAN,}/
