# Why Dgeni

Now, why do one want to use Dgeni as documentation generator? There are plenty other tools out there
right? One could just use [JSDoc](http://usejsdoc.org/), or [YUIDoc](http://yui.github.io/yuidoc/),
both do a very good job. Well, yes that's right. However there are a few reasons why one probably
wants to use Dgeni instead.

- **Your code has AngularJS syntax**

  Ever wanted to annotate your code with proper annotations for things like services, filters or
  directives? How about modules and controllers? Can you annotate your code with tools like JSDoc or
  YUIDoc? Exactly. These document generators don't support AngularJS specific syntax. Dgeni however,
  can be extended with an NGDoc package which gives you exactly **that** functionality. So in other
  words, if you work on AngularJS related code and want to document it, Dgeni is probably the only
  tool that makes it possible.

- **You want custom annotation**

  Similar to the fact that AngularJS brings is own kind of syntactic sugar for specific types of
  components, you probably want to have your very own annotation types. That could have different
  reasons. For example, imagine the case you work on a library or framework that should be capable
  of dealing with AngularJS specific annotations as well as VanillaJS annotations. In such case, you
  probably don't want to spread `@ngdoc` annotations all over the place, but rather have your own
  custom annotation tag that kind of declares your specific namespace.

- **Flexibility**

  Dgeni is more or less just a pipe that executes a set of different processors to process the given
  documentation data. Now what has that to do with flexibility? Being able to add, remove or change
  the order of processors, gives you full control on how your documentation is being processed and
  also generated. **You** can decide what documentation data is relevant to you. **You** can decide
  what features should be enabled in your generated documentation. **You** can decide how your
  documentation gets rendered.

- **Modularity**

  This goes hand in hand with **flexibility**. But the real benefit of being modular, is the fact
  that it's insanely easy to add additional functionality. Functionality that doesn't even exist yet.
  If you want to, you can build your own custom packages and come up with features that nobody else
  has in his documentation. Of course, you can also just remove functionality if something doesn't
  fit to your needs.

These are just a few arguments, why one probably want to use Dgeni as a documentation generator.
Evaluate for yourself if it's the right tool or not.
