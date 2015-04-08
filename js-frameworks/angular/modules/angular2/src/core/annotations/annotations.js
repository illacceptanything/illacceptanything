import {ABSTRACT, CONST, normalizeBlank, isPresent} from 'angular2/src/facade/lang';
import {ListWrapper, List} from 'angular2/src/facade/collection';
import {Injectable} from 'angular2/di';

// type StringMap = {[idx: string]: string};

/**
 * Directives allow you to attach behavior to elements in the DOM.
 *
 * Directive is an abstract concept, instead use concrete directives: [Component], [DynamicComponent], [Decorator]
 * or [Viewport].
 *
 * A directive consists of a single directive annotation and a controller class. When the directive's [selector] matches
 * elements in the DOM, the following steps occur:
 *
 * 1. For each directive, the [ElementInjector] attempts to resolve the directive's constructor arguments.
 * 2. Angular instantiates directives for each matched element using [ElementInjector] in a depth-first order,
 *    as declared in the HTML.
 *
 * ## Understanding How Injection Works
 *
 * There are three stages of injection resolution.
 * - *Pre-existing Injectors*:
 *   - The terminal [Injector] cannot resolve dependencies. It either throws an error or, if the dependency was
 *     specified as `@Optional`, returns `null`.
 *   - The primordial injector resolves browser singleton resources, such as: cookies, title, location, and others.
 * - *Component Injectors*: Each `@Component` has its own [Injector], and they follow the same parent-child hierarchy
 *     as the components in the DOM.
 * - *Element Injectors*: Each component has a Shadow DOM. Within the Shadow DOM each element has an [ElementInjector]
 *     which follow the same parent-child hierarchy as the DOM elements themselves.
 *
 * When a template is instantiated, it also must instantiate the corresponding directives in a depth-first order. The
 * current [ElementInjector] resolves the constructor dependencies for each directive.
 *
 * Angular then resolves dependencies as follows, according to the order in which they appear in the [View]:
 *
 * 1. Dependencies on the current element
 * 2. Dependencies on element injectors and their parents until it encounters a Shadow DOM boundary
 * 3. Dependencies on component injectors and their parents until it encounters the root component
 * 4. Dependencies on pre-existing injectors
 *
 *
 * The [ElementInjector] can inject other directives, element-specific special objects, or it can delegate to the parent
 * injector.
 *
 * To inject other directives, declare the constructor parameter as:
 * - `directive:DirectiveType`: a directive on the current element only
 * - `@Ancestor() directive:DirectiveType`: any directive that matches the type between the current element and the
 *    Shadow DOM root. Current element is not included in the resolution, therefor even if it could resolve it, it will
 *    be ignored.
 * - `@Parent() directive:DirectiveType`: any directive that matches the type on a direct parent element only.
 * - `@Children query:Query<DirectiveType>`: A live collection of direct child directives [TO BE IMPLEMENTED].
 * - `@Descendants query:Query<DirectiveType>`: A live collection of any child directives [TO BE IMPLEMENTED].
 *
 * To inject element-specific special objects, declare the constructor parameter as:
 * - `element: NgElement` to obtain a DOM element (DEPRECATED: replacement coming)
 * - `viewContainer: ViewContainer` to control child template instantiation, for [Viewport] directives only
 * - `bindingPropagation: BindingPropagation` to control change detection in a more granular way.
 *
 * ## Example
 *
 * The following example demonstrates how dependency injection resolves constructor arguments in practice.
 *
 *
 * Assume this HTML template:
 *
 * ```
 * <div dependency="1">
 *   <div dependency="2">
 *     <div dependency="3" my-directive>
 *       <div dependency="4">
 *         <div dependency="5"></div>
 *       </div>
 *       <div dependency="6"></div>
 *     </div>
 *   </div>
 * </div>
 * ```
 *
 * With the following `dependency` decorator and `SomeService` injectable class.
 *
 * ```
 * @Injectable()
 * class SomeService {
 * }
 *
 * @Decorator({
 *   selector: '[dependency]',
 *   bind: {
 *     'id':'dependency'
 *   }
 * })
 * class Dependency {
 *   id:string;
 * }
 * ```
 *
 * Let's step through the different ways in which `MyDirective` could be declared...
 *
 *
 * ### No injection
 *
 * Here the constructor is declared with no arguments, therefore nothing is injected into `MyDirective`.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor() {
 *   }
 * }
 * ```
 *
 * This directive would be instantiated with no dependencies.
 *
 *
 * ### Component-level injection
 *
 * Directives can inject any injectable instance from the closest component injector or any of its parents.
 *
 * Here, the constructor declares a parameter, `someService`, and injects the `SomeService` type from the parent
 * component's injector.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor(someService: SomeService) {
 *   }
 * }
 * ```
 *
 * This directive would be instantiated with a dependency on `SomeService`.
 *
 *
 * ### Injecting a directive from the current element
 *
 * Directives can inject other directives declared on the current element.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor(dependency: Dependency) {
 *     expect(dependency.id).toEqual(3);
 *   }
 * }
 * ```
 * This directive would be instantiated with `Dependency` declared at the same element, in this case `dependency="3"`.
 *
 *
 * ### Injecting a directive from a direct parent element
 *
 * Directives can inject other directives declared on a direct parent element. By definition, a directive with a
 * `@Parent` annotation does not attempt to resolve dependencies for the current element, even if this would satisfy
 * the dependency.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor(@Parent() dependency: Dependency) {
 *     expect(dependency.id).toEqual(2);
 *   }
 * }
 * ```
 * This directive would be instantiated with `Dependency` declared at the parent element, in this case `dependency="2"`.
 *
 *
 * ### Injecting a directive from any ancestor elements
 *
 * Directives can inject other directives declared on any ancestor element (in the current Shadow DOM), i.e. on the
 * parent element and its parents. By definition, a directive with an `@Ancestor` annotation does not attempt to
 * resolve dependencies for the current element, even if this would satisfy the dependency.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor(@Ancestor() dependency: Dependency) {
 *     expect(dependency.id).toEqual(2);
 *   }
 * }
 * ```
 *
 * Unlike the `@Parent` which only checks the parent, `@Ancestor` checks the parent, as well as its
 * parents recursively. If `dependency="2"` didn't exist on the direct parent, this injection would have returned
 * `dependency="1"`.
 *
 *
 * ### Injecting a live collection of direct child directives [PENDING IMPLEMENTATION]
 *
 * A directive can also query for other child directives. Since parent directives are instantiated before child
 * directives, a directive can't simply inject the list of child directives. Instead, the directive asynchronously
 * injects a [Query], which updates as children are added, removed, or moved by any [ViewPort] directive such as a
 * `for`, an `if`, or a `switch`.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor(@Children() dependencies:Query<Maker>) {
 *   }
 * }
 * ```
 *
 * This directive would be instantiated with a [Query] which contains `Dependency` 4 and 6. Here, `Dependency` 5 would
 * not be included, because it is not a direct child.
 *
 * ### Injecting a live collection of direct descendant directives [PENDING IMPLEMENTATION]
 *
 * Similar to `@Children` above, but also includes the children of the child elements.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor(@Children() dependencies:Query<Maker>) {
 *   }
 * }
 * ```
 *
 * This directive would be instantiated with a Query which would contain `Dependency` 4, 5 and 6.
 *
 * ### Optional injection
 *
 * The normal behavior of directives is to return an error when a specified dependency cannot be resolved. If you
 * would like to inject `null` on unresolved dependency instead, you can annotate that dependency with `@Optional()`.
 * This explicitly permits the author of a template to treat some of the surrounding directives as optional.
 *
 * ```
 * @Decorator({ selector: '[my-directive]' })
 * class MyDirective {
 *   constructor(@Optional() dependency:Dependency) {
 *   }
 * }
 * ```
 *
 * This directive would be instantiated with a `Dependency` directive found on the current element. If none can be
 * found, the injector supplies `null` instead of throwing an error.
 *
 * @publicModule angular2/annotations
 */
@ABSTRACT()
export class Directive extends Injectable {
  /**
   * The CSS selector that triggers the instantiation of a directive.
   *
   * Angular only allows directives to trigger on CSS selectors that do not cross element boundaries.
   *
   * `selector` may be declared as one of the following:
   *
   * - `element-name`: select by element name.
   * - `.class`: select by class name.
   * - `[attribute]`: select by attribute name.
   * - `[attribute=value]`: select by attribute name and value.
   * - `:not(sub_selector)`: select only if the element does not match the `sub_selector`.
   * - `selector1, selector2`: select if either `selector1` or `selector2` matches.
   *
   *
   * ## Example
   *
   * Suppose we have a directive with an `input[type=text]` selector.
   *
   * And the following HTML:
   *
   * ```html
   * <form>
   *   <input type="text">
   *   <input type="radio">
   * <form>
   * ```
   *
   * The directive would only be instantiated on the `<input type="text">` element.
   *
   */
  selector:string;

  /**
   * Enumerates the set of properties that accept data binding for a directive.
   *
   * The `bind` property defines a set of `directiveProperty` to `bindingProperty` key-value pairs:
   *
   * - `directiveProperty` specifies the component property where the value is written.
   * - `bindingProperty` specifies the DOM property where the value is read from.
   *
   * You can include [Pipes] when specifying a `bindingProperty` to allow for data transformation and structural
   * change detection of the value. These pipes will be evaluated in the context of this component.
   *
   *
   * ## Syntax
   *
   * ```
   * @Directive({
   *   bind: {
   *     'directiveProperty1': 'bindingProperty1',
   *     'directiveProperty2': 'bindingProperty2 | pipe1 | ...',
   *     ...
   *   }
   * }
   * ```
   *
   *
   * ## Basic Property Binding
   *
   * We can easily build a simple `Tooltip` directive that exposes a `tooltip` property, which can be used in templates
   * with standard Angular syntax. For example:
   *
   * ```
   * @Decorator({
   *   selector: '[tooltip]',
   *   bind: {
   *     'text': 'tooltip'
   *   }
   * })
   * class Tooltip {
   *   set text(text) {
   *     // This will get called every time the 'tooltip' binding changes with the new value.
   *   }
   * }
   * ```
   *
   * We can then bind to the `tooltip' property as either an expression (`someExpression`) or as a string literal, as
   * shown in the HTML template below:
   *
   * ```html
   * <div [tooltip]="someExpression">...</div>
   * <div tooltip="Some Text">...</div>
   * ```
   *
   * Whenever the `someExpression` expression changes, the `bind` declaration instructs Angular to update the
   * `Tooltip`'s `text` property.
   *
   *
   *
   * ## Bindings With Pipes
   *
   * You can also use pipes when writing binding definitions for a directive.
   *
   * For example, we could write a binding that updates the directive on structural changes, rather than on reference
   * changes, as normally occurs in change detection. (See: [Pipe] and [keyValueDiff] documentation for more details.)
   *
   * ```
   * @Decorator({
   *   selector: '[class-set]',
   *   bind: {
   *     'classChanges': 'classSet | keyValDiff'
   *   }
   * })
   * class ClassSet {
   *   set classChanges(changes:KeyValueChanges) {
   *     // This will get called every time the `class-set` expressions changes its structure.
   *   }
   * }
   * ```
   *
   * The template that this directive is used in may also contain its own pipes. For example:
   *
   * ```html
   * <div [class-set]="someExpression | somePipe">
   * ```
   *
   * In this case, the two pipes compose as if they were inlined: `someExpression | somePipe | keyValDiff`.
   *
   */
  bind:any; //  StringMap

  /**
   * Specifies which DOM events a directive listens to.
   *
   * The `events` property defines a set of `event` to `method` key-value pairs:
   *
   * - `event1`: the DOM event that the directive listens to.
   * - `statement`: the statement to execute when the event occurs.
   *
   *
   * When writing a directive event binding, you can also refer to the following local variables:
   * - `$event`: Current event object which triggered the event.
   * - `$target`: The source of the event. This will be either a DOM element or an Angular directive.
   *    [TO BE IMPLEMENTED]
   *
   *
   * ## Syntax
   *
   * ```
   * @Directive({
   *   events: {
   *     'event1': 'onMethod1(arguments)',
   *     ...
   *   }
   * }
   * ```
   *
   * ## Basic Event Binding:
   *
   * Suppose you want to write a directive that triggers on `change` events in the DOM. You would define the event
   * binding as follows:
   *
   * ```
   * @Decorator({
   *   selector: 'input',
   *   events: {
   *     'change': 'onChange($event)'
   *   }
   * })
   * class InputDecorator {
   *   onChange(event:Event) {
   *   }
   * }
   * ```
   *
   * Here the `onChange` method of `InputDecorator` is invoked whenever the DOM element fires the 'change' event.
   *
   */
  events:any; //  StringMap

  /**
   * Specifies a set of lifecycle events in which the directive participates.
   *
   * See: [onChange], [onDestroy], [onAllChangesDone] for details.
   */
  lifecycle:List; //List<LifecycleEvent>

  @CONST()
  constructor({
      selector,
      bind,
      events,
      lifecycle
    }:{
      selector:string,
      bind:any,
      events: any,
      lifecycle:List
    }={})
  {
    super();
    this.selector = selector;
    this.bind = bind;
    this.events = events;
    this.lifecycle = lifecycle;
  }

  /**
   * Returns true if a directive participates in a given [LifecycleEvent].
   *
   * See: [onChange], [onDestroy], [onAllChangesDone] for details.
   */
  hasLifecycleHook(hook:string):boolean {
    return isPresent(this.lifecycle) ? ListWrapper.contains(this.lifecycle, hook) : false;
  }
}

/**
 * Declare reusable UI building blocks for an application.
 *
 * Each Angular component requires a single `@Component` and at least one `@Template` annotation. The `@Component`
 * annotation specifies when a component is instantiated, and which properties and events it binds to.
 *
 * When a component is instantiated, Angular
 * - creates a shadow DOM for the component.
 * - loads the selected template into the shadow DOM.
 * - creates a child [Injector] which is configured with the [Component.services].
 *
 * All template expressions and statements are then evaluated against the component instance.
 *
 * For details on the `@Template` annotation, see [Template].
 *
 * ## Example
 *
 * ```
 * @Component({
 *   selector: 'greet'
 * })
 * @Template({
 *   inline: 'Hello {{name}}!'
 * })
 * class Greet {
 *   name: string;
 *
 *   constructor() {
 *     this.name = 'World';
 *   }
 * }
 * ```
 *
 * @publicModule angular2/annotations
 */
export class Component extends Directive {
  /**
   * Defines the used change detection strategy.
   *
   * When a component is instantiated, Angular creates a change detector, which is responsible for propagating
   * the component's bindings.
   *
   * The `changeDetection` property defines, whether the change detection will be checked every time or only when the component
   * tells it to do so.
   */
  changeDetection:string;

  /**
   * Defines the set of injectable objects that are visible to a Component and its children.
   *
   * The [services] defined in the Component annotation allow you to configure a set of bindings for the component's
   * injector.
   *
   * When a component is instantiated, Angular creates a new child Injector, which is configured with the bindings in
   * the Component [services] annotation. The injectable objects then become available for injection to the component
   * itself and any of the directives in the component's template, i.e. they are not available to the directives which
   * are children in the component's light DOM.
   *
   *
   * The syntax for configuring the [services] injectable is identical to [Injector] injectable configuration. See
   * [Injector] for additional detail.
   *
   *
   * ## Simple Example
   *
   * Here is an example of a class that can be injected:
   *
   * ```
   * class Greeter {
   *    greet(name:string) {
   *      return 'Hello ' + name + '!';
   *    }
   * }
   *
   * @Component({
   *   selector: 'greet',
   *   services: [
   *     Greeter
   *   ]
   * })
   * @Template({
   *   inline: `{{greeter.greet('world')}}!`,
   *   directives: Child
   * })
   * class HelloWorld {
   *   greeter:Greeter;
   *
   *   constructor(greeter:Greeter) {
   *     this.greeter = greeter;
   *   }
   * }
   * ```
   */
  services:List;

@CONST()
  constructor({
    selector,
    bind,
    events,
    services,
    lifecycle,
    changeDetection
    }:{
      selector:string,
      bind:Object,
      events:Object,
      services:List,
      lifecycle:List,
      changeDetection:string
    }={})
  {
    super({
      selector: selector,
      bind: bind,
      events: events,
      lifecycle: lifecycle
    });

    this.changeDetection = changeDetection;
    this.services = services;
  }
}

/**
 * Directive used for dynamically loading components.
 *
 * Regular Angular components are statically resolved. DynamicComponent allows to you resolve a component at runtime
 * instead by providing a placeholder into which a regular Angular component can be dynamically loaded. Once loaded,
 * the dynamically-loaded component becomes permanent and cannot be changed.
 *
 *
 * ## Example
 *
 * Here we have `DynamicComp` which acts as the placeholder for `HelloCmp`. At runtime, the dynamic component
 * `DynamicComp` requests loading of the `HelloCmp` component.
 *
 * There is nothing special about `HelloCmp`, which is a regular Angular component. It can also be used in other static
 * locations.
 *
 * ```
 * @DynamicComponent({
 *   selector: 'dynamic-comp'
 * })
 * class DynamicComp {
 *   helloCmp:HelloCmp;
 *   constructor(loader:PrivateComponentLoader, location:PrivateComponentLocation) {
 *     loader.load(HelloCmp, location).then((helloCmp) => {
 *       this.helloCmp = helloCmp;
 *     });
 *   }
 * }
 *
 * @Component({
 *   selector: 'hello-cmp'
 * })
 * @Template({
 *   inline: "{{greeting}}"
 * })
 * class HelloCmp {
 *   greeting:string;
 *   constructor() {
 *     this.greeting = "hello";
 *   }
 * }
 * ```
 *
 *
 *
 * @publicModule angular2/annotations
 */
export class DynamicComponent extends Directive {
  /**
   * Same as [Component.services].
   */
  // TODO(vsankin): Please extract into AbstractComponent
  services:any; //List;

  @CONST()
  constructor({
    selector,
    bind,
    events,
    services,
    lifecycle
    }:{
      selector:string,
      bind:Object,
      events:Object,
      services:List,
      lifecycle:List
    }={}) {
    super({
      selector: selector,
      bind: bind,
      events: events,
      lifecycle: lifecycle
    });

    this.services = services;
  }
}

/**
 * Directive that attaches behavior to DOM elements.
 *
 * A decorator directive attaches behavior to a DOM element in a composable manner.
 * (see: http://en.wikipedia.org/wiki/Composition_over_inheritance)
 *
 * Decorators:
 * - are simplest form of [Directive]s.
 * - are best used as a composition pattern ()
 *
 * Decorators differ from [Component]s in that they:
 * - can have multiple decorators per element
 * - do not create their own evaluation context
 * - do not have a template (and therefor do not create Shadow DOM)
 *
 *
 * ## Example
 *
 * Here we use a decorator directive to simply define basic tool-tip behavior.
 *
 * ```
 * @Decorator({
 *   selector: '[tooltip]',
 *   bind: {
 *     'text': 'tooltip'
 *   },
 *   events: {
 *     'onmouseenter': 'onMouseEnter()',
 *     'onmouseleave': 'onMouseLeave()'
 *   }
 * })
 * class Tooltip{
 *   text:string;
 *   overlay:Overlay; // NOT YET IMPLEMENTED
 *   overlayManager:OverlayManager; // NOT YET IMPLEMENTED
 *
 *   constructor(overlayManager:OverlayManager) {
 *     this.overlay = overlay;
 *   }
 *
 *   onMouseEnter() {
 *     // exact signature to be determined
 *     this.overlay = this.overlayManager.open(text, ...);
 *   }
 *
 *   onMouseLeave() {
 *     this.overlay.close();
 *     this.overlay = null;
 *   }
 * }
 * ```
 * In our HTML template, we can then add this behavior to a `<div>` or any other element with the `tooltip` selector,
 * like so:
 *
 *  ```
 * <div tooltip="some text here"></div>
 * ```
 *
 * @publicModule angular2/annotations
 */
export class Decorator extends Directive {

  /**
   * If set to true the compiler does not compile the children of this directive.
   */
  //TODO(vsavkin): This would better fall under the Macro directive concept.
  compileChildren: boolean;

  @CONST()
  constructor({
      selector,
      bind,
      events,
      lifecycle,
      compileChildren = true,
    }:{
      selector:string,
      bind:any,
      events:any,
      lifecycle:List,
      compileChildren:boolean
    }={})
  {
    super({
        selector: selector,
        bind: bind,
        events: events,
        lifecycle: lifecycle
    });
    this.compileChildren = compileChildren;
  }
}

/**
 * Directive that controls the instantiation, destruction, and positioning of inline template elements.
 *
 * A viewport directive uses a [ViewContainer] to instantiate, insert, move, and destroy views at runtime.
 * The [ViewContainer] is created as a result of `<template>` element, and represents a location in the current view
 * where these actions are performed.
 *
 * Views are always created as children of the current [View], and as siblings of the `<template>` element. Thus a
 * directive in a child view cannot inject the viewport directive that created it.
 *
 * Since viewport directives are common in Angular, and using the full `<template>` element syntax is wordy, Angular
 * also supports a shorthand notation: `<li *foo="bar">` and `<li template="foo: bar">` are equivalent.
 *
 * Thus,
 *
 * ```
 * <ul>
 *   <li *foo="bar" title="text"></li>
 * </ul>
 * ```
 *
 * Expands in use to:
 *
 * ```
 * <ul>
 *   <template [foo]="bar">
 *     <li title="text"></li>
 *   </template>
 * </ul>
 * ```
 *
 * Notice that although the shorthand places `*foo="bar"` within the `<li>` element, the binding for the `Viewport`
 * controller is correctly instantiated on the `<template>` element rather than the `<li>` element.
 *
 *
 * ## Example
 *
 * Let's suppose we want to implement the `unless` behavior, to conditionally include a template.
 *
 * Here is a simple viewport directive that triggers on an `unless` selector:
 *
 * ```
 * @Viewport({
 *   selector: '[unless]',
 *   bind: {
 *     'condition': 'unless'
 *   }
 * })
 * export class Unless {
 *   viewContainer: ViewContainer;
 *   prevCondition: boolean;
 *
 *   constructor(viewContainer: ViewContainer) {
 *     this.viewContainer = viewContainer;
 *     this.prevCondition = null;
 *   }
 *
 *   set condition(newCondition) {
 *     if (newCondition && (isBlank(this.prevCondition) || !this.prevCondition)) {
 *       this.prevCondition = true;
 *       this.viewContainer.clear();
 *     } else if (!newCondition && (isBlank(this.prevCondition) || this.prevCondition)) {
 *       this.prevCondition = false;
 *       this.viewContainer.create();
 *     }
 *   }
 * }
 * ```
 *
 * We can then use this `unless` selector in a template:
 * ```
 * <ul>
 *   <li *unless="expr"></li>
 * </ul>
 * ```
 *
 * Once the viewport instantiates the child view, the shorthand notation for the template expands and the result is:
 *
 * ```
 * <ul>
 *   <template [unless]="exp">
 *     <li></li>
 *   </template>
 *   <li></li>
 * </ul>
 * ```
 *
 * Note also that although the `<li></li>` template still exists inside the `<template></template>`, the instantiated
 * view occurs on the second `<li></li>` which is a sibling to the `<template>` element.
 *
 *
 * @publicModule angular2/annotations
 */
export class Viewport extends Directive {
  @CONST()
  constructor({
      selector,
      bind,
      events,
      lifecycle
    }:{
      selector:string,
      bind:any,
      lifecycle:List
    }={})
  {
    super({
        selector: selector,
        bind: bind,
        events: events,
        lifecycle: lifecycle
    });
  }
}

//TODO(misko): turn into LifecycleEvent class once we switch to TypeScript;

/**
 * Notify a directive whenever a [View] that contains it is destroyed.
 *
 * ## Example
 *
 * ```
 * @Decorator({
 *   ...,
 *   lifecycle: [onDestroy]
 * })
 * class ClassSet {
 *   onDestroy() {
 *     // invoked to notify directive of the containing view destruction.
 *   }
 * }
 * ```
 * @publicModule angular2/annotations
 */
export const onDestroy = "onDestroy";


/**
 * Notify a directive when any of its bindings have changed.
 *
 * This method is called right after the directive's bindings have been checked,
 * and before any of its children's bindings have been checked.
 *
 * It is invoked only if at least one of the directive's bindings has changed.
 *
 * ## Example:
 *
 * ```
 * @Decorator({
 *   selector: '[class-set]',
 *   bind: {
 *     'propA': 'propA'
 *     'propB': 'propB'
 *   },
 *   lifecycle: [onChange]
 * })
 * class ClassSet {
 *   propA;
 *   propB;
 *   onChange(changes:{[idx: string, PropertyUpdate]}) {
 *     // This will get called after any of the properties have been updated.
 *     if (changes['propA']) {
 *       // if propA was updated
 *     }
 *     if (changes['propA']) {
 *       // if propB was updated
 *     }
 *   }
 * }
 *  ```
 * @publicModule angular2/annotations
 */
export const onChange = "onChange";

/**
 * Notify a directive when the bindings of all its children have been changed.
 *
 * ## Example:
 *
 * ```
 * @Decorator({
 *   selector: '[class-set]',
 *   lifecycle: [onAllChangesDone]
 * })
 * class ClassSet {
 *
 *   onAllChangesDone() {
 *   }
 *
 * }
 *  ```
 * @publicModule angular2/annotations
 */
export const onAllChangesDone = "onAllChangesDone";
