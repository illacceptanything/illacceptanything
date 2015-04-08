# Installation

This document guides you through the installation process of Dgeni on different platforms.

## Prerequisites

To install Dgeni on your local machine, there are a few tools that have to be present first.

- **Node.js**

  You'll need to install [Node.js](http://nodejs.org). Just download the binary from the website and
  install it on your machine.

- **npm**

  *npm* is the "Node Package Manager". It usually comes with the Node.js binary.

You can also install these tools manually by taking a look at this [gist](https://gist.github.com/isaacs/579814).

## Installing Dgeni via npm

Installing Dgeni is as easy as running the following command on your command line (assuming you're
located in a project folder):

```js
npm install dgeni
```

To add Dgeni as development dependency to your project, run this command with the `--save-dev` option.
This will add Dgeni to the list of `devDependencies` in your `package.json` file.

```js
npm install --save-dev dgeni
```

Both commands will install Dgeni in your projects folder under `node_modules/`. Dgeni is now locally
installed and ready to use.
