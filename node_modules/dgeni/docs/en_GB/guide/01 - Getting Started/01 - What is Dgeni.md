# What is Dgeni?

Dgeni is a documentation generator built by the AngularJS Team. It was originally created to help
build the documentation for the AngularJS project.

The project consists of a Node.js package, which can be run directly as a command line tool or
easily integrated into build tools such as Grunt or Gulp.

## Project Goals

The main goal of the Dgeni project is to help software developers to generate documentation from
their code.

To achieve this the project provides a tool that aims to be technology agnostic, you could use it to
document any kind of software project, from AngularJS JavaScript applications, through to OS kernel
code, written in C.

The tool should provide a simple preconfigured solution for the most common cases but be completely
configurable and customizable for any documentation situation.


## How It Works

The flexibility of Dgeni is achieved by delegating the actual documentation generation to a
pipeline of "document processors", which are defined in packages as part of the configuration of
Dgeni.

Once configured Dgeni will simply run each processor in series. Each processor receives an
collection of document objects returned from the previous processor, the processor then modifies
this collection or the properties of the document objects in the collection and then returns the
collection for the next processor.

Each processor should have a single well defined responsibility, which makes maintaining and reusing
the processors easier.

Some of the most essential processors would have responsibility for:

- loading up document source from files
- parsing the document source for meta data (such as jsdoc tags)
- computing additional properties based on those that have been parsed, such as the name of the
  document
- rendering the documents into a readable form, such as HTML, usually driven by templates
- writing the rendered documents to output files

## Dgeni and AngularJS

The AngularJS project, uses its own kind of annotation when it comes to documenting its code,
known as "ngdoc" tags. These tags are similar but not officially supported by typical documentation
generators like JSDoc. Therefore, the AngularJS Team built their own documentation generator that
works like jsdoc but was hard-coded to work specifically with ngdoc tags and the particular type of
structure that AngularJS uses in its code.

However, it turned out that the NGDoc generator implementation was hard to maintain and change,
because things like templates weren't separated from the actual code base. Also adding new features
wasn't easy for new contributors. In addition, the entire generator was built into the AngularJS
code base which made it difficult for other development teams to reuse the library to help document
their own AngularJS apps and libraries.

The AngularJS solved this problem by writing and using Dgeni.  They now provide a set of packages
that support the documenting AngularJS but since these are modular and separated from the AngularJS
code-base it is easier to maintain the processors and also possible for other teams to reuse them.

## Should I Use Dgeni?

Dgeni can be used for documenting any kind of code, both client-side, such as AngularJS apps, and
server-side, such as a Node.js RESTful server.

It is actualy agnostic when it comes to the technology you are using in your project. Since you can
build your own custom packages, you could use it to document anything from a .NET Windows app to a
PHP server.  It just needs you or someone else who is willing to write the necessary document
processors.

That being said, Dgeni is written in Node.js by the AngularJS team, so it is likely that there will
be more document processors around that provide support for documenting a JavaScript project.
