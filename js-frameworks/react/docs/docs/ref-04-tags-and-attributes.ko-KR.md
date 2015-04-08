---
id: tags-and-attributes-ko-KR
title: 태그와 어트리뷰트
permalink: tags-and-attributes-ko-KR.html
prev: component-specs-ko-KR.html
next: events-ko-KR.html
---

## 지원되는 태그

React는 모든 공통 엘리먼트를 지원하려 합니다. 필요한 엘리먼트가 목록에 없다면, 이슈로 등록해 주세요.

### HTML 엘리먼트

다음의 HTML 엘리먼트가 지원됩니다.

```
a abbr address area article aside audio b base bdi bdo big blockquote body br
button canvas caption cite code col colgroup data datalist dd del details dfn
dialog div dl dt em embed fieldset figcaption figure footer form h1 h2 h3 h4 h5
h6 head header hr html i iframe img input ins kbd keygen label legend li link
main map mark menu menuitem meta meter nav noscript object ol optgroup option
output p param picture pre progress q rp rt ruby s samp script section select
small source span strong style sub summary sup table tbody td textarea tfoot th
thead time title tr track u ul var video wbr
```

### SVG 엘리먼트

다음의 SVG 엘리먼트가 지원됩니다.

```
circle defs ellipse g line linearGradient mask path pattern polygon polyline
radialGradient rect stop svg text tspan
```

아마 Canvas, SVG, VML(IE8 전용)에 렌더할 때 쓰는 React의 드로잉 라이브러리인 [react-art](https://github.com/facebook/react-art)도 흥미 있으실 수 있습니다.


## 지원되는 어트리뷰트

React는 모든 `data-*`, `aria-*` 어트리뷰트와 밑에 있는 모든 어트리뷰트를 지원합니다.

> 주의:
>
> 모든 어트리뷰트는 카멜케이스이고, `class` `for` 어트리뷰트는 각각  DOM API의 사양에 맞춰서 `className` `htmlFor` 가 됩니다.

이벤트의 목록을 보시려면 [지원되는 이벤트](/react/docs/events-ko-KR.html)를 확인하세요.

### HTML 어트리뷰트

이런 표준 어트리뷰트가 지원됩니다.

```
accept acceptCharset accessKey action allowFullScreen allowTransparency alt
async autoComplete autoFocus autoPlay cellPadding cellSpacing charSet checked classID
className cols colSpan content contentEditable contextMenu controls coords
crossOrigin data dateTime defer dir disabled download draggable encType form
formAction formEncType formMethod formNoValidate formTarget frameBorder height
hidden href hrefLang htmlFor httpEquiv icon id label lang list loop manifest
marginHeight marginWidth max maxLength media mediaGroup method min multiple
muted name noValidate open pattern placeholder poster preload radioGroup
readOnly rel required role rows rowSpan sandbox scope scoped scrolling seamless
selected shape size sizes span spellCheck src srcDoc srcSet start step style
tabIndex target title type useMap value width wmode
```

덧붙여, 이런 비표준 어트리뷰트도 지원됩니다.

- 모바일 사파리를 위한 `autoCapitalize autoCorrect`.
- [오픈 그래프](http://ogp.me/) 메타 태그를 위한 `property`.
- [HTML5 마이크로데이터](http://schema.org/docs/gs.html)를 위한 `itemProp itemScope itemType itemRef itemId`.

컴포넌트에 직접 HTML 문자열을 넣을 때 사용하는, React 전용 어트리뷰트 `dangerouslySetInnerHTML`([자세한 정보는 여기](/react/docs/special-non-dom-attributes-ko-KR.html))도 있습니다.

### SVG 어트리뷰트

```
cx cy d dx dy fill fillOpacity fontFamily fontSize fx fy gradientTransform
gradientUnits markerEnd markerMid markerStart offset opacity
patternContentUnits patternUnits points preserveAspectRatio r rx ry
spreadMethod stopColor stopOpacity stroke strokeDasharray strokeLinecap
strokeOpacity strokeWidth textAnchor transform version viewBox x1 x2 x y1 y2 y
```
