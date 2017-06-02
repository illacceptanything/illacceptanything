---
id: forms-zh-CN
title: 表单组件
permalink: forms-zh-CN.html
prev: transferring-props-zh-CN.html
next: working-with-the-browser-zh-CN.html
---

诸如 `<input>`、`<textarea>`、`<option>` 这样的表单组件不同于其他组件，因为他们可以通过用户交互发生变化。这些组件提供的界面使响应用户交互的表单数据处理更加容易。
  
关于 `<form>` 事件详情请查看 [表单事件](/react/docs/events-zh-CN.html#form-events)。

## 交互属性    
    
表单组件支持几个受用户交互影响的属性：
    
* `value`，用于 `<input>`、`<textarea>` 组件。
* `checked`，用于类型为 `checkbox` 或者 `radio` 的 `<input>` 组件。
* `selected`，用于 `<option>` 组件。

在 HTML 中，`<textarea>` 的值通过子节点设置；在 React 中则应该使用 `value` 代替。        

表单组件可以通过 `onChange` 回调函数来监听组件变化。当用户做出以下交互时，`onChange` 执行并通过浏览器做出响应：
          
* `<input>` 或 `<textarea>` 的 `value` 发生变化时。
* `<input>` 的 `checked` 状态改变时。
* `<option>` 的 `selected` 状态改变时。
              
和所有 DOM 事件一样，所有的 HTML 原生组件都支持 `onChange` 属性，而且可以用来监听冒泡的 `change` 事件。               
              
              
## 受限组件              

设置了 `value` 的 `<input>` 是一个*受限*组件。 对于受限的 `<input>`，渲染出来的 HTML 元素始终保持 `value` 属性的值。例如：

```javascript
  render: function() {
    return <input type="text" value="Hello!" />;
  }
```
              
上面的代码将渲染出一个值为 `Hello!` 的 input 元素。用户在渲染出来的元素里输入任何值都不起作用，因为 React 已经赋值为     `Hello!`。如果想响应更新用户输入的值，就得使用 `onChange` 事件：

```javascript
  getInitialState: function() {
    return {value: 'Hello!'};
  },
  handleChange: function(event) {
    this.setState({value: event.target.value});
  },
  render: function() {
    var value = this.state.value;
    return <input type="text" value={value} onChange={this.handleChange} />;
  }
```

上面的代码中，React 将用户输入的值更新到 `<input>` 组件的 `value` 属性。这样实现响应或者验证用户输入的界面就很容易了。例如：

```javascript
  handleChange: function(event) {
    this.setState({value: event.target.value.substr(0, 140)});
  }
```

上面的代码接受用户输入，并截取前 140 个字符。

              
## 不受限组件

没有设置 `value`(或者设为 `null`) 的 `<input>` 组件是一个*不受限*组件。对于不受限的 `<input>` 组件，渲染出来的元素直接反应用户输入。例如：

```javascript
  render: function() {
    return <input type="text" />;
  }
```

上面的代码将渲染出一个空值的输入框，用户输入将立即反应到元素上。和受限元素一样，使用 `onChange` 事件可以监听值的变化。

如果想给组件设置一个非空的初始值，可以使用 `defaultValue` 属性。例如：

```javascript
  render: function() {
    return <input type="text" defaultValue="Hello!" />;
  }
```

上面的代码渲染出来的元素和**受限组件**一样有一个初始值，但这个值用户可以改变并会反应到界面上。

同样地， 类型为 `radio`、`checkbox` 的`<input>` 支持 `defaultChecked` 属性， `<select>` 支持 `defaultValue` 属性。

```javascript
  render: function() {
      return (
          <div>
            <input type="radio" name="opt" defaultChecked /> Option 1
            <input type="radio" name="opt" /> Option 2
            <select defaultValue="C">
              <option value="A">Apple</option>
              <option value="B">Banana</option>
              <option value="C">Cranberry</option>
            </select>
          </div>
      );
    }
```                


## 高级主题
                
### 为什么使用受限组件？

在 React 中使用诸如 `<input>` 的表单组件时，遇到了一个在传统 HTML 中没有的挑战。
                
比如下面的代码：                

```html
  <input type="text" name="title" value="Untitled" />
```

在 HTML 中将渲染初始值为 `Untitled` 的输入框。用户改变输入框的值时，节点的 `value` 属性（*property*）将随之变化，但是 `node.getAttribute('value')` 还是会返回初始设置的值 `Untitled`.

与 HTML 不同，React 组件必须在任何时间点描绘视图的状态，而不仅仅是在初始化时。比如在 React 中：

```javascript
  render: function() {
    return <input type="text" name="title" value="Untitled" />;
  }
```

该方法在任何时间点渲染组件以后，输入框的值就应该*始终*为 `Untitled`。


### 为什么 `<textarea>` 使用 `value` 属性？

在 HTML 中， `<textarea>` 的值通常使用子节点设置：

```html
  <!-- 反例：在 React 中不要这样使用！ -->
  <textarea name="description">This is the description.</textarea>
```
                  
对 HTML 而言，让开发者设置多行的值很容易。但是，React 是 JavaScript，没有字符限制，可以使用 `\n` 实现换行。简言之，React 已经有 `value`、`defaultValue` 属性，`</textarea>` 组件的子节点扮演什么角色就有点模棱两可了。基于此， 设置 `<textarea>` 值时不应该使用子节点：

```javascript
  <textarea name="description" value="This is a description." />
```

如果*非要**使用子节点，效果和使用 `defaultValue` 一样。


### 为什么 `<select>` 使用 `value` 属性                  

HTML 中 `<select>` 通常使用 `<option>` 的 `selected` 属性设置选中状态；React 为了更方面的控制组件，采用以下方式代替：

```javascript
  <select value="B">
    <option value="A">Apple</option>
    <option value="B">Banana</option>
    <option value="C">Cranberry</option>
  </select>
```

如果是不受限组件，则使用 `defaultValue`。

> 注意：
>
> 给 `value` 属性传递一个数组，可以选中多个选项：`<select multiple={true} value={['B', 'C']}>`。
