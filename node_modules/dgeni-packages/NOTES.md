# Implementation Notes

## Code Ids and URL Paths

Currently there is lots of inconsistency in how code is referenced by name.  This
leads to complex and difficult to maintain code.  Instead we should agree on a consistent naming
scheme as follows:

* Each code component should have a well defined `id` based on their module, name and type.
* Everything exists in a module (even globals since they are loaded along with a module such as ng
  or ngMock).
* Special kinds of component should be highlighted - modules, directives, filters and globals -
  using a scoping syntax, the type of the component followed by a colon prefixed to the component
  name.
* Services are just objects (or functions) and would not need to be prefixed
* Types are just constructor functions defined either globally or as a service in a module. They
  should be identifiable through starting with a upper case letter so they should not need a special
  component type.
* Input directives have sub-types, such as `checkbox` or `text`.  These should be incorporated into
  the component name using square brackets `input[checkbox]`.
* Methods on components should be defined by # fragments, which will be simply appended to the URL
  and allow the browser to navigate directly to the documentation for that item in the page:

This scheme makes is consistent to convert back and forth between ids and paths.

### Examples

```
module:ngSanitize
module:ng.filter:currency
module:ng.directive:input[checkbox]
module:ngRoute.directive:ngView
module:ngMock.global:angular.mock.dump
module:ngResource.$resource
module:ng.global:form.FormController
module:ng.$http#methods_head
```

### Path Mapping

The ids above would map to the following URL paths, also making a nice path hierarchy:

```
ngSanitize
ng/filter/currency
ng/directive/input[checkbox]
ngRoute/directive/ngView
ngMock/global/angular.mock.dump
ngResource/service/$resource
ng/global/form.FormController
ng/$http#methods_head
```

### Links to Code
Since we have all the documents in a single collection during processing, we can do clever matching
of links against code names. This means that you only have to provide enough of a components id to
identify it unambiguously relative to other components in the collection.
For example, in the AngularJS code we can refer to many components simply by their "name":

```
ngShow
$route
ngSanitize
```

But were there is ambiguity we need to add progressively more information:

```
ngMock:$log
ngMockE2E:$httpBackend
```


## @ngdoc tags
Currently the `@ngdoc` tag can contain one of the following values:

* `error` - only used for minerr documentation
* `function` - generally used for global functions but sometimes "misused" for a service or method
* `property` - generally used for properties on services but also used for `angular.version`
* `overview` - generally used for modules and ngdocs but also used for `ng.$rootElement` and `angular.mock` (should be objects?)
* `object` - generally used for services that are not straight functions
* `method` - used for methods on services and types (such as `angular.Module`, etc)
* `interface` - only used for `angular.Module` in `angular-load.js`
* `service` - used only occasionally for some angular services
* `directive` - used for angular directives
* `inputType` - used for input element specific directives (such as `input[checkbox]`)
* `event` - used for events on objects (mostly services)
* `filter` - used for angular filters (although there may be one or two that use function)

We ought to consolidate to:

* `function` - for global functions
* `object` - for global objects
* `interface` - for global interfaces
* `type` - for constructors
* `module` - instead of `overview` for modules
* `service` - instead of object/function for angular services
* `serviceProvider` - instead of function/object for angular service providers
* `directive` - as-is (but also include inputTypes, e.g input[checkbox])
* `filter` - as-is
* `method` - as-is
* `property` - as-is (but change angular.version to object)
* `event` - as-is
* Anything else is just a descriptive name for the content, such as `error`, `guide`, `tutorial`,
  etc.


## Errors

Currently the source folder structure for errors doesn't match the urls and names of the errors
because we are using things like `compile` instead of `$compile`. We should change this to make it
more consistent.  (Not immediately essential as we currently generate the `@id` from the `@name` in
any case).

## Guide (and content in general)
We should lose the flat structure and create folder hierarchies for the source files that will map
to the URL paths to the guide documents

E.g. `content/guide/dev_guide.services.injecting_controllers.ngdoc` should become
`content/guide/services/injecting_controllers.ngdoc`


## Legacy Implementation Issues

### Annotations
This could be done as an angular directive rather than the custom //!annotate solution

For example, in ngShow:

```
 * If you wish to change the hide behavior with ngShow/ngHide then this can be achieved by
 * restating the styles for the .ng-hide class in CSS:
 * <pre>
 * .ng-hide {
 *   <div annotation title="CSS Specificity" content="Not to worry, this will override the AngularJS default...">
 *   display:block!important;
 *   </div>
 *
 *   //this is just another form of hiding an element
 *   position:absolute;
 *   top:-9999px;
 *   left:-9999px;
 * }
 * </pre>
```

Or in ngAnimate

```
<pre class="prettyprint linenums">
var ngModule = angular.module(<div annotation title="Your AngularJS Module" content="Replace this or ngModule with the module that you used to define your application.">'YourApp'</div>, ['ngAnimate']);
ngModule.animation('.my-crazy-animation', function() {
  return {
    enter: function(element, done) {
      //run the animation here and call done when the animation is complete
      return function(cancelled) {
```

