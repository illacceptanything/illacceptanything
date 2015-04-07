<img src="http://elliottcable.s3.amazonaws.com/p/paws.js-cathode-3.png">
<a href="https://travis-ci.org/ELLIOTTCABLE/Paws.js"><img alt='Build status' src="https://img.shields.io/travis/ELLIOTTCABLE/Paws.js/Current.svg"></a><img src="http://elliottcable.s3.amazonaws.com/p/8x8.png"><a href="https://coveralls.io/r/ELLIOTTCABLE/Paws.js"><img alt='Coverage status' src="https://img.shields.io/coveralls/ELLIOTTCABLE/Paws.js/Current.svg"></a><img src="http://elliottcable.s3.amazonaws.com/p/8x8.png"><a href="httks://gemnasium.com/ELLIOTTCABLE/Paws.js"><img alt='Dependency status' src="https://img.shields.io/gemnasium/ELLIOTTCABLE/Paws.js.svg"></a><img src="http://elliottcable.s3.amazonaws.com/p/8x8.png"><a href="https://github.com/ELLIOTTCABLE/Paws.js/releases"><img alt='Latest tag' src="https://img.shields.io/github/tag/ELLIOTTCABLE/Paws.js.svg"></a><img src="http://elliottcable.s3.amazonaws.com/p/8x8.png"><a href="https://npmjs.com/package/paws.js"><img alt='Latest NPM release' src="https://img.shields.io/npm/v/paws.js.svg"></a>

**Hello, friend.** This is a JavaScript implementation of the Paws machine, intended both to be included
into client-side code executed by browsers, and to be embedded into [Node.js][] projects.

**“What's a Paws,”** you ask? [Paws][] could be seen either as a *type* of programming language, or
as a design for a VM *on which* languages of that type can be run. Paws is a project sitting
somewhere between a pure VM for language development (think: the JVM), and a family of languages
(think: the LISPs.)

Paws lends itself well to highly *asynchronous* programming, meaning it's designed for things
involving the network requests (by design, web applications), and other tasks where concurrency is
desirable. In addition, things built on top of Paws can *distribute* themselves across multiple
environments and machines (this means your database, and your user's browsers, can all talk amongst
one-another.) Finally, Paws is designed from the ground-up to be *concurrency*-aware, ensuring tasks
can parallelize when they won't affect eachother negatively.

**“Cool! Can I use it?”** Probably not, I'm afraid, although it's adorable that you ask.
Unfortunately, this project is basically just a VM, with some excruciatingly-primative primatives
with which one can construct abstractions. (Writing code that will run on this machine is
approximately analogous to writing raw assembler.) Before this project will be useful to you,
somebody'll need to write some abstractions (basically, a language) on top of it!

To boot, the Paws system as a whole is still under heavy design and development; lots of things are
still likely to change. Although there's a [specification for the current version,][spec] lots of
relatively fundamental aspects of the machine's semantics are still subject to evolution. In fact,
some of the neatest features of the design aren't nailed down into the specification yet (nor are
they implemented in this codebase); so anybody trying to write those abstractions for you is
probably going to have some of their work invalidated in the future. **tl;dr: the Paws design isn't
stable, yet!**

**“Okay, well, I like language design. Can I write stuff on top of this machine?”** I'm so glad you
asked! You're my favourite kind of person! Assuming you understand the caveat mentioned above (that
this project is in flux), you can *absolutely* start experimenting with abstractions on top of the
Paws machine.

If you want to learn more, you should definitely [grab yourself an IRC client][irc] (I suggest
[IRCCloud][]), or just [click here][webchat], to join the chatroom where we discuss the Paws project
as a whole: [`#ELLIOTTCABLE`][webchat] on Freenode. All newcomers are welcome, and contribution is
hugely appreciated!

   [Node.js]: <http://nodejs.org> "A server-side JavaScript platform"
   [Paws]: <http://paws.mu> "An asynch-heavy distributed platform for concurrent programming"
   [spec]: <http://ell.io/spec> "Specification for the 10th iteration of the Paws design"
   [irc]: <http://freenode.net/using_the_network.shtml> "freenode: using the network"
   [IRCCloud]: <http://irccloud.com> "IRCCloud, the IRC client of the future"
   [webchat]: <http://ell.io/IRC> "Freenode's webchat, for #ELLIOTTCABLE"

Using
-----
This implementation of a Paws machine can be used in two ways: interactively, at the command-line;
or directly, via its embedding API. More information about command-line usage can be acquired by
querying the executable at the command-line:

    npm install                     # (Must be run before the executable can be used)
    ./Executables/paws.js --help
    ./Executables/paws.js interact  # Example, opens an interactive ‘REPL’ to play with

As for embedding the `Paws.js` API, you'll have to dive into the code and poke around a bit, for the
moment. I'm also happy to give you a quick overview, if you join [our channel][webchat] and ask!

*(I swear, API documentation is coming soon! `:P` )*

Contributing
------------
I consistently put a lot of effort into ensuring that this codebase is easy to spelunk. Hell, I
reduced myself to using [CoffeeScript][], to make everything easier to read! `(=`

Any and all [issues / pull-requests][issues] are welcome. There's a rather comprehensive test-suite
for the reactor itself; it's appreciated if any changes are contributed with passing test-cases, of
course.

After `git clone`'ing the codebase, you should immediately `npm run-script hi`; this will help you
set up your local copy for hacking.

More specific information ~~can be found in [CONTRIBUTING](./blob/Master/CONTRIBUTING.markdown)~~.
(Well, eventually. `>,>`)

   [CoffeeScript]: <http://coffeescript.org> "A little language that transpiles into JavaScript"
   [issues]: <https://github.com/ELLIOTTCABLE/Paws.js/issues> "Issue-tracker for Paws.js"

<br>
----
<div align='center' id='npm-and-browser-support'>
   <a href="https://npmjs.org/package/paws.js">
      <img alt="npm downloads" src="https://nodei.co/npm-dl/paws.js.png?months=9"></a>
<!--
   <h4>Browser support:</h4>
   <a href="https://ci.testling.com/ELLIOTTCABLE/Paws.js">
      <img alt="Current browser-support status on HEAD (generated by Testling-CI)" src="https://ci.testling.com/ELLIOTTCABLE/Paws.js.png"> </a>
-->
</div>
