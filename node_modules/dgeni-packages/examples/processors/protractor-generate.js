"use strict";

var _ = require('lodash');

/**
 * dgProcessor generateProtractorTestsProcessor
 * @description
 * Generate a protractor test files from the e2e tests in the examples
 */
module.exports = function generateProtractorTestsProcessor(exampleMap) {
  return {
    deployments: [],
    basePath: '',
    $validate: {
      deployments: { presence: true },
    },
    $runAfter: ['adding-extra-docs'],
    $runBefore: ['extra-docs-added'],
    $process: function(docs) {

      var deployments = this.deployments;
      var basePath = this.basePath;

      exampleMap.forEach(function(example) {
        _.forEach(example.files, function(file) {

          // Check if it's a Protractor test.
          if (file.type === 'protractor') {

            _.forEach(deployments, function(deployment) {
              docs.push(createProtractorDoc(example, deployment, file, basePath));
            });
          }

        });
      });
    }
  };
};

function createProtractorDoc(example, deployment, file, basePath) {
  return {
    docType: 'e2e-test',
    id: 'protractorTest' + '-' + example.id + '-' + deployment.name,
    example: example,
    deployment: deployment,
    template: 'protractorTests.template.js',
    innerTest: file.fileContents,
    'ng-app-included': example['ng-app-included'],
    basePath: basePath
  };
}