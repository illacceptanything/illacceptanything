# Matreshka v1.0.4

[![Matreshka Website](http://matreshka.io/img/mk5-logo_full-vert.svg)](http://matreshka.io)

### [Download](https://github.com/finom/matreshka/releases)

[![Join the chat at https://gitter.im/finom/matreshka](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/finom/matreshka?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) 

```html
<script src="matreshka.min.js"></script>
<input type="text" class="my-input">
<script>
var app = new Matreshka;
app.bindNode( 'x', '.my-input' );
app.x = 'Two-way data binding in JS? O rly?';
</script>
``` 

Matreshka - is a small and powerful client-side JavaScript framework that allows you to build single page applications as simply as possible.

* Two-way data-bindings in JavaScript files. No more {{weird.syntax}} inside HTML.
* It's simple. Really. You don't need to learn mass of articles to get started.
* Custom architecture. You can choose any way how you build an application.

## [Release History](https://github.com/finom/matreshka/releases)

## Todo
* Fire ``remove`` and ``modify`` when data key is removed from ``Matreshka.Object`` instance (``remove``, ``removeDataKeys``)
* ``force`` flag for ``Matreshka.Array`` events
* ``Matreshka#off`` for delegated DOM events with specified selector
* Refactor ``Matreshka#_on`` and ``Matreshka#_off`` methods
* Remove event after handler call in ``Matreshka#once``
* ``binder.destroy`` member that calls when ``Matreshka#unbindNode`` is called
* ``private`` global event flag that makes event to be silent for delegated handlers
* Fire ``remove`` event on data remove for ``Matreshka.Object``
* Export Matreshka using ES6 syntax
* Automated testing
* Comment every part of code
* Matreshka.Array.of
* Matreshka.Array.from


**Maybe will be added in next versions**
* ``Matreshka.toMatreshka`` - converts JS object to Matreshka instance
* Some way to get parents from delegated Matreshka events
* ``toJSONString`` method
* ``bindSandbox`` method which accepts single argument + event options
* ``this.bound.key`` instead of ``this.bound('key')`` as alternative

------------------------------------

## Resources
[**The website**](http://matreshka.io)

[**Docs in Russian**](http://ru.matreshka.io/)

[**Gitter chat in Russian**](https://gitter.im/finom/matreshka/ru)

[**JSDoc files for IDE (ENG, RUS)**](https://github.com/finom/matreshka_docs)

[**TodoMVC**](https://github.com/finom/matreshka_todomvc)

[**Issues**](https://github.com/finom/matreshka/issues)

[**Twitter**](https://twitter.com/matreshkajs)

**Author:** Andrey Gubanov <a@odessite.com.ua>

**License:** [MIT License](https://raw.github.com/finom/matreshka/master/LICENSE)



