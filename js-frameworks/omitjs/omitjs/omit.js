/*!
 * Omit.js JavaScript Library v 0.8
 * The MIT License
 *
 * Copyright (c) 2013 Viktor Jansson - http://www.omitjs.com - viktor@omitjs.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * Object used for filling with pages
 * @type {Object}
 */
var page = {};

/**
 * Function to create a div, which is one of the most common things you
 * will use in the html function.
 * @param self
 * @param html
 * @param inside
 * @return {String}
 */
var div = function(self,html,inside) {
	return '<div'+self.obj+' '+(inside || '')+'>'+html+'</div>';
}

/**
 * Function to return a css string but allows you to write it in a
 * prettier way with indentation, like this:
 * css({
 * 		'property':'value',
 * });
 * @param properties
 * @return {String}
 */
var css = function(properties) {
	var output = '{';
	if (properties instanceof Array) {
		for (var property in properties) {
			output += properties[property]+';';
		}
	} else {
		for (var property in properties) {
			output += property+':'+properties[property]+';';
		}
	}

	output += '}';

	return output;
};

/**
 * Set interval to check if user has defined site, and if so, run
 * Omit.head with site variables. This is pretty ugly, but it's
 * more consistent with the code layout where all settings (objects,
 * pages, animations and the site) are defined as objects and not in
 * functions as site would have been otherwise.
 */
var checkSiteInterval = setInterval(function(){
	if (window.site) {
		Omit.initSite(site);
		clearInterval(checkSiteInterval);
	}
},2);

/**
 * Function to return a string with parameters used when linking to
 * different pages
 * @param params
 * @return {String}
 */
var params = function(params) {
	var output = '?';
	for(param in params) {
		output += param+'='+params[param]+'&';
	}
	output = output.substring(0,output.length-1);
	return output;
}

/**
 * The compiler object, this is where the magic happens. The code
 * in this object takes care of converting your omit-objects and
 * pages into html, css and javascript.
 * @type {Object}
 */
var Omit = {
	version: 0.8,
	/**
	* Constants, strings used by compiler.
	*/
	HTML: 'html',
	CSS: 'css',
	CSS_MEDIA: {
		TABLET: 'tablet',
		MOBILE: 'mobile',
		DESKTOP: 'desktop',
		PRINT: 'print'
	},

	JS: 'js',
	CHILDREN: 'children',
	FUNCTIONS: 'functions',

	PARENT: 'parent',
	NAME: 'objName',
	OBJ: 'obj',

	SITE: 'site',
	PAGE: 'page',
	PAGE_INDEX: 'index',
	HASHBANG: '#!',

	/**
	* Variables used when compiling css.
	*/
	css: {
		desktop: '',
		tablet: '',
		mobile: '',
		print: ''
	},

	/**
	 * Media queries, used when defining custom css for specific viewing modes.
	 */
	cssMediaQuery: {
		desktop: function(css) {
			return css;
		},
		mobile: function(css) {
			return this.MOBILE+'{'+css+'}';
		},
		tablet: function(css) {
			return this.TABLET+'{'+css+'}';
		},
		print: function(css) {
			return this.PRINT+'{'+css+'}';
		},
		TABLET: '@media only screen and (max-width: 641px)',
		MOBILE: '@media only screen and (max-width: 320px)',
		PRINT: '@media print'
	},
	/**
	 * This cssReset is automatically added to the beginning of the css for the website.
	 */
	cssReset: 'html, body, div, span, applet, object, iframe,h1, h2, h3, h4, h5, h6, p, blockquote, pre, a, abbr, acronym, address, big, cite, code, del, dfn, em, img, ins, kbd, q, s, samp, small, strike, strong, sub, sup, tt, var, b, u, i, center, dl, dt, dd, ol, ul, li, fieldset, form, label, legend, table, caption, tbody, tfoot, thead, tr, th, td, article, aside, canvas, details, embed,  figure, figcaption, footer, header, hgroup,  menu, nav, output, ruby, section, summary, time, mark, audio, video {m:0;p:0;b:0;f-s:100%;f:inherit;vertical-align:baseline;}body{l-h:1;}ol, ul{list-style:none;}blockquote:before, blockquote:after,q:before, q:after {content: \'\';content: none;}table {border-collapse: collapse;border-spacing: 0;}.clearfix:after {clear: both;content: " ";display: block;visibility: hidden;}',
	cssAnimations: '',
	genericCss: '',

	/**
	 * Variable used to output the content of the current page in.
	 */
	page: '<div id="page"></div>',

	/**
	 * Internal variables used by general compilation
	 */
	isCompiled: false,
	parent: '',
	parentObject: false,
	compiledJs: {},
	compiledJsFuncNum: 0,
	compiledO: {},
	refO: {},
	refPage: {},

	/**
	 * Site events, overwritable using the site object. It has some default functions
	 * but ideally you want to make something of your own.
	 */
	onReady: function() {},
	on404: function() {
		if (page.error404) {
			this.setPageToHash(page.error404);
		} else {
			alert('Error 404 - Page not found.');
		}
	},
	onPageChange: function(newHTML,callback) {
		$('#'+this.PAGE).html(newHTML);
		callback();
	},
	/**
	* Css replacements for shorthand css.
	*/
	shorthandCSSProperties: {
		/*Position & placement*/
		'l':'left',
		'r':'right',
		't':'top',
		'bt':'bottom',
		'w':'width',
		'h':'height',

		'v':'visibility',
		'z':'z-index',

		'min-h':'min-height',
		'min-w':'min-width',
		'max-h':'max-height',
		'max-w':'max-width',

		'dir':'direction',
		'pos':'position',
		'fl':'float',
		'cl':'clear',
		'd':'display',

		'l-s':'list-style',
		'l-s-img':'list-style-img',
		'l-s-pos':'list-style-position',
		'l-s-t':'list-style-type',
		'cnt-i':'counter-increment',
		'cnt-r':'counter-reset',
		'q':'quotes',

		/* Overflow */
		'o':'overflow',
		'o-x':'overflow-x',
		'o-y':'overflow-y',
		'o-s':'overflow-style',

		/*Padding*/
		'p':'padding',
		'p-l':'padding-left',
		'p-b':'padding-bottom',
		'p-r':'padding-right',
		'p-t':'padding-top',
		
		/*Margin*/
		'm':'margin',
		'm-l':'margin-left',
		'm-b':'margin-bottom',
		'm-r':'margin-right',
		'm-t':'margin-top',
		
		/*Background & colors*/
		'bg':'background',
		'bg-c':'background-color',
		'bg-a':'background-attachment',
		'bg-img':'background-image',
		'bg-pos':'background-position',
		'bg-r':'background-repeat',
		'bg-clip':'background-clip',
		'bg-o':'background-origin',
		'c':'color',

		'ct':'content',

		/*Border*/
		'b':'border', //border-bottom-color not supported, use border-color: red red black red
		'b-col':'border-collapse',
		'b-c':'border-color',
		'b-s':'border-style',
		'b-sp':'border-spacing',
		'b-w':'border-width',

		'b-t':'border-top',
		'b-b':'border-bottom',
		'b-l':'border-left',
		'b-rt':'border-right',

		'out':'outline',
		'out-c':'outline-color',
		'out-s':'outline-style',
		'out-w':'outline-width',
		'out-o':'outline-offset',

		'res':'resize',
		'cur':'cursor',

		/*Fonts & texts*/
		'f':'font',
		'f-f':'font-family',
		'f-s':'font-size',
		'f-s-a':'font-size-adjust',
		'f-st':'font-style',
		'f-w':'font-weight',
		'f-v':'font-variant',
		'l-h':'line-height',
		'l-sp':'letter-spacing',
		't-d':'text-decoration',
		't-t':'text-transform',
		't-a':'text-align',
		't-s':'text-shadow',
		't-i':'text-indent',
		't-j':'text-justify',
		't-o':'text-outline',
		't-out':'text-outline',
		't-w':'text-wrap',
		'w-b':'word-break',
		'w-s':'word-spacing',
		'wh-s':'white-space',

		'op':'opacity',

		'pg-b-a':'page-break-after',
		'pg-b-b':'page-break-before',
		'pg-b-i':'page-break-inside',

		'tbl-l':'table-layout',
		'c-s':'caption-side',
		'e-c':'empty-cells',
		'v-a':'vertical-align'
	},
	/**
	 * Shorthands for commonly used css attributes.
	 */
	shorthandCSSAttributes: {
		'abs':'absolute',
		'rel':'relative',
		'l':'left',
		'r':'right',
		'c':'center',
		'n':'none',
		'nl':'normal',
		'd':'default',
		'a':'auto',
		'h':'hidden',
		'v':'visible',
		'b':'block',
		'bt':'both',
		'bd':'bold',
		'i':'inline',
		'i-b':'inline-block'
	},
	/**
	* Settings for properties being replaced with browser-specific prefixes
	* First el in array is what to replace with, next are which prefixes to use.
	* w = -webkit-
	* m = -moz-
	* i = -ms- (i for internet explorer)
	* o = -o-
	* = no prefix
	**/
	shorthandCSS3Properties: {
		'b-r':['border-radius','w',''],
		'b-img':['border-image','w','m','o',''],
		'b-img-r':['border-image-repeat','w','m','o',''],
		'b-img-s':['border-image-slice','w','m','o',''],
		'b-img-src':['border-image-source','w','m','o',''],
		'b-img-w':['border-image-width','w','m','o',''],

		'col-cnt':['column-count','w','m','o',''],
		'col-f':['column-fill','w','m','o',''],
		'col-g':['column-gap','w','m','o',''],
		'col-r':['column-rule','w','m','o',''],
		'col-r-c':['column-rule-color','w','m','o',''],
		'col-r-w':['column-rule-width','w','m','o',''],
		'col-s':['column-span','w','m','o',''],
		'col-w':['column-width','w','m','o',''],
		'cols':['columns','w','m','o',''],

		'box-s':['box-shadow','w','m','o',''],
		'box-sz':['box-sizing','w','m','o',''],
		'box-a':['box-align','w','m',''],
		'box-dir':['box-direction','w','m',''],
		'box-f':['box-flex','w','m',''],
		'box-f-g':['box-flex-group','w','m',''],
		'box-l':['box-lines','w','m',''],
		'box-p':['box-pack','w','m',''],

		'bg-s':['background-size','w','m',''],

		'app':['appearance','w','m',''],

		'trf':['transform','w','m','i','o',''],
		'trf-s':['transform-style','w','m','i','o',''],
		'trf-o':['transform-origin','w','m','i','o',''],

		'trs':['transition','w','m','o',''],
		'trs-p':['transition-property','w','m','o',''],
		'trs-d':['transition-duration','w','m','o',''],
		'trs-t-f':['transition-timing-function','w','m','o',''],
		'trs-dl':['transition-delay','w','m','o',''],
		'pers':['perspective','w','m','i',''],
		'pers-o':['perspective-origin','w','m','i',''],
		'anim':['animation','w','m','o'] //animation-delay etc is not supported as they are generated with the animation compiler.
	},
	shorthandCSS3Prefixes: {
		'w':'webkit',
		'm':'moz',
		'i':'ms',
		'o':'o'
	},
	/**
	* These functions are run once the site object is declared. Structure is required.
	* @param structure
	*/
	init: function(structure) {
		this.setSite();
		this.compileCSS3Animations();
		this.compileStart();
		this.setCss();
		this.compilePageObjectStart();
		this.structure(structure());
		this.listenForHashChange();
		this.setPageToHash();
		this.isCompiled = true;
	},
	/**
	* Start compiling the objects
	*/
	compileStart: function() {
		this.refO = window.o;
		window.o = {};
		this.compile(this.refO, window.o);
	},
	/**
	 * Compiles o.object.html into o.object and takes care of all css, js, children and functions.
	 * @param obj
	 * @param into
	 * Recursive
	 */
	compile: function(obj, into) {
		if(obj && into) {
			for(var child in obj) {
				var referencedObj = obj[child];
				//console.log('Object',referencedObj[this.HTML]);

				//If has HTML, create a reference o.obj to o.obj.html
				if (referencedObj[this.HTML]) {
					//Set reference this.name
					referencedObj[this.NAME] = child;

					//Merge children and functions for a reference to the parent object.
					var parentObj = $.extend({}, this.parentObject[this.CHILDREN], this.parentObject[this.FUNCTIONS]);

					//Overwrite the object with it's html function to allow o.obj() instead of o.obj.html();
					into[child] = this.compileHtmlFuncTemplate(referencedObj, child, parentObj);

					//Set the this.obj & this.name for the object, used when referencing the object from outside of the object
					into[child][this.OBJ] = this.getObjScopedName(child);
					into[child][this.NAME] = this.getObjName(child);
					//console.log($.extend({}, into[child]));
				}

				//Add and convert the objects css
				if (referencedObj[this.CSS]) {
					var referencedObjCSS = referencedObj[this.CSS];
					//Set this.obj to .object
					referencedObj[this.OBJ] = this.getObjScopedName(child);
					referencedObj[this.NAME] = this.getObjName(child);

					//If css is function, add it to the css, otherwise it is an object
					//containing different css media.
					if (referencedObj[this.CSS] instanceof Function) {
						this.css.desktop += referencedObj[this.CSS]();
					} else {
						referencedObj[this.CSS][this.OBJ] = referencedObj[this.OBJ];
						referencedObj[this.CSS][this.NAME] = referencedObj[this.NAME];
						//console.log(child,'has media');
						var thisCss = this.css;
						for (var media in this.css) {
							if (referencedObjCSS[media]) {
								this.css[media] += referencedObjCSS[media]();
							}
						}
					}
				}

				//If this obj has functions,
				if (referencedObj[this.FUNCTIONS]) {
					//Cache the functions
					var referencedObjFunctions = referencedObj[this.FUNCTIONS];

					//Set this.name and this.obj (used for calling this.parent.)
					referencedObjFunctions[this.OBJ] = this.getObjScopedName(child);
					referencedObjFunctions[this.NAME] = this.getObjName(child);

					//Loop through all functions in the functions object.
					referencedObj = $.extend(referencedObj, referencedObj[this.FUNCTIONS]);

					into[child] = $.extend(into[child], referencedObj[this.FUNCTIONS]);

				}


				//Add the objects js function to a queue to be run at page load.
				if (referencedObj[this.JS]) {
					this.compiledJs[child] = this.compileJsFuncTemplate(referencedObj);
				}

				//If this has children, then compile them.
				if (referencedObj[this.CHILDREN]) {
					var children = referencedObj[this.CHILDREN];
					this.parent = this.parent + (this.parent != '' ? ' .' : '') + child;
					this.parentObject = referencedObj;
					this.compile(children, into[child]);
					this.parent = '';
					this.parentObject = false;

					//Create this references
					for (var grandchild in children) {
						if(into[child][grandchild]) {
							referencedObj[grandchild] = into[child][grandchild];
							referencedObj[grandchild][this.PARENT] = into[child];
						}

					}
				}

			}
		}
	},
	/**
	 * Scoping for the js function of an object
	 * @param referenceObj
	 * @return {Function}
	 */
	compileJsFuncTemplate: function(referenceObj) {
		return function() {
			return referenceObj[Omit.JS].apply(referenceObj);
		}
	},
	/**
	 * Scoping for the html function of an object
	 * @param referenceObj
	 * @param name
	 * @param parent
	 * @return {Function}
	 */
	compileHtmlFuncTemplate: function(referenceObj, name, parent) {
		return function() {
			referenceObj[Omit.OBJ] = ' class="'+name+'"';
			referenceObj[Omit.NAME] = name; //Fixes bug when functions are in object, name cannot be used by html function
			referenceObj[Omit.PARENT] = parent;
			return referenceObj[Omit.HTML].apply(referenceObj, arguments);
		}
	},
	/**
	 * Return .obj .child depending on parent
	 * @param child
	 * @return {String}
	 */
	getObjScopedName: function(child) {
		return (this.parent != '' ? '.' : '')+this.parent + (this.parent != '' ? ' .' : '.')+child;
	},
	/**
	 * Returns the name of the object. Kept in function if we want to manipulate it in the future.
	 * @param child
	 * @return {*}
	 */
	getObjName: function(child) {
		return child;
	},
	/**
	 * Creates references for js
	 * @param obj
	 * Recursive
	 */
	setJsObj: function(obj) {
		for (var child in obj) {
			obj[child][this.OBJ] = this.getObjScopedName(child);
			obj[child][this.NAME] = this.getObjName(child);
			if (obj[child][this.CHILDREN]) {
				this.parent = this.parent + (this.parent != '' ? ' .' : '') + child;
				this.setJsObj(obj[child][this.CHILDREN]);
				this.parent ='';
			}
		}
	},
	/**
	 * Run all js functions for all objects on the current page.
	 */
	setJs: function() {
		setTimeout(function(){
			var self = Omit;
			var compiledJs = self.compiledJs;
			self.parent = '';
			self.setJsObj(Omit.refO);

			var objects = $('[class]');
			var classesOnPage = {};
			objects.each(function() {
				classesOnPage[ $(this).attr('class') ] = true;
			});


			for (var className in classesOnPage) {
				if (className.indexOf(' ') !== -1) {
					var classNames = className.split(' ');
					for (var name in classNames) {
						if (compiledJs[ classNames[name] ]) {
							compiledJs[ classNames[name] ]();
						}
					}
				} else {
					if (compiledJs[ className ]) {
						compiledJs[ className ]();
					}
				}
			}
		},1);
	},
	/**
	 * Set Css for page
	 */
	setCss: function() {
		var css = this.cssReset + this.genericCss + this.cssAnimations;

		var thisCss = this.css;
		for (var media in thisCss) {
			if (thisCss[media] !== '') {
				css += this.cssMediaQuery[media]( thisCss[media] );
			}
		}

		css = this.convertShorthandCSS(css);
		var head = document.getElementsByTagName('head')[0];
		var style = document.createElement('style');

		style.type = 'text/css';
		if (style.styleSheet){
			style.styleSheet.cssText = css;
		} else {
			style.appendChild(document.createTextNode(css));
		}
		head.appendChild(style);
	},
	/**
	 * Starts compilation of the page object, from an object containing functions
	 * to strings that can be used in the window.location.hash
	 */
	compilePageObjectStart: function() {
		this.refPage = window.page;
		window.page = {};
		this.compilePageObject(this.refPage, window.page);
	},
	/**
	 * Create the page object, containing string names of the pages.
	 * @param obj
	 * @param into
	 * @param base
	 * Recursive
	 */
	folder: '',
	compilePageObject: function(obj, into, base) {
		for (var cPage in obj) {
			if (obj[cPage] instanceof Function) {
				into[cPage] = this.HASHBANG + this.folder + cPage;
			} else {
				into[cPage] = {};
				this.folder = this.folder + cPage + '/';
				this.compilePageObject(obj[cPage], into[cPage], base);
				this.folder = '';
			}
		}
	},
	/**
	 * Function to insert an external css document, for example if you
	 * are using plugins that's not omitified.
	 * @param url
	 */
	insertExternalCSS: function(url) {
		var head = document.getElementsByTagName("head")[0];
		var cssNode = document.createElement('link');
		cssNode.type = 'text/css';
		cssNode.rel = 'stylesheet';
		cssNode.href = url;
		cssNode.media = 'screen';
		head.appendChild(cssNode);
	},
	/**
	 * Function that activates google analytics, if set in the site variable.
	 */
	activateGoogleAnalytics: function(id) {
		window._gaq = window._gaq || [];
		window._gaq.push(['_setAccount', id]);
		window._gaq.push(['_trackPageview']);

		var ga = document.createElement('script');
		ga.type = 'text/javascript';
		ga.async = true;
		ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
		var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);

		this.googleAnalyticsActive = true;
	},
	googleAnalyticsActive: false,
	/**
	* Construct the page structure. It's put in a div to allow changing of the structure
	* after omit has been initialized
	*/
	structure: function(html) {
		document.getElementById(this.SITE).innerHTML = html;
	},
	/**
	* Init the site depending on the properties set in the site variable. This function calls
	* Omit.init and is the first function to be run.
	*/
	initSite: function(obj) {
		//We have to have a page structure.
		if (!obj.structure) {
			alert('Page structure not defined.');
		}
		//If we want a title
		if (obj.title) {
			document.title = obj.title;
		}
		//If we want some generic css.
		if(obj.css) {
			this.genericCss = obj.css;
		}
		//Include commonly used google services
		if (obj.google) {
			//Include google fonts
			if (obj.google.fonts) {
				var googleFonts = obj.google.fonts;
				for(var font in googleFonts) {
					this.insertExternalCSS('http://fonts.googleapis.com/css?family='+encodeURI(googleFonts[font]));
				}
			}
			//Include google analytics
			if (obj.google.analytics) {
				this.activateGoogleAnalytics(obj.google.analytics);
			}
		}

		//Change default media queries
		if (obj.mediaQuery) {
			if (obj.mediaQuery.tablet) {
				this.cssMediaQuery.TABLET = obj.mediaQuery.tablet;
			}
			if (obj.mediaQuery.mobile) {
				this.cssMediaQuery.MOBILE = obj.mediaQuery.mobile;
			}
			if (obj.mediaQuery.print) {
				this.cssMediaQuery.PRINT = obj.mediaQuery.print;
			}
		}

		//Insert external css stylesheets.
		if (obj.cssFiles) {
			var cssFiles = obj.cssFiles;
			for (var cssFile in cssFiles) {
				this.insertExternalCSS(cssFiles[cssFile]);
			}
		}

		//Special function for onPageChange, used for animations for example
		if(obj.onPageChange) {
			this.onPageChange = obj.onPageChange;
		}
		//Function to execute onReady, which is launched after a page is ready.
		if (obj.onReady) {
			this.onReady = function(){
				obj.onReady();
			}
		}

		//Insert external js files. Note! If the files contains errors, omit can't compile them and hence will spawn a blank page.
		if(obj.jsFiles) {
			var jsFiles = obj.jsFiles;
			var jsFilesLength = jsFiles.length;
			var jsFilesLoaded = 0;
			for (var i = 0; i < jsFilesLength; i++) {
				$.getScript(jsFiles[i], function() {
					jsFilesLoaded++;
				});
			}
			//Only init omit after all scripts has loaded.
			var allLoaded = setInterval(function(){
				if (jsFilesLength == jsFilesLoaded) {
					try {
						Omit.init(obj.structure);
						clearInterval(allLoaded);
					 } catch(e) {
						console.error('Error in the omit compiler when initializing Omit.', e);
						clearInterval(allLoaded);
					}
				}
			},1);
		} else {
			Omit.init(obj.structure);
		}
	},
	/**
	* Creates the #site div which is a container for the full site where the structure is set.
	*/
	setSite: function() {
		var div = document.createElement('div');
		div.id = this.SITE;
		if (document.body.firstChild) {
			document.body.insertBefore(div, document.body.firstChild);
		} else {
			document.body.appendChild(div);
		}
	},
	/**
	* Change page if hash changes.
	*/
	listenForHashChange: function() {
		$(window).on('hashchange', function() {
			Omit.setPageToHash();
		});
	},
	/**
	 * Set page HTML based on what page we have called.
	 */
	setPageHtml: function(cPage, args, anchor) {
		if (cPage) {
			try {
				var html = cPage.apply(this, args);
				//Trigger google analytics if google analytics is activated.
				if (this.googleAnalyticsActive) {
					window._gaq.push(['_trackPageview', window.location.hash]);
				}
				//Trigger the onPageChange event
				this.onPageChange(html, function(){
					setTimeout(function(){
						Omit.onReady();
						Omit.scrollToAnchor(anchor);
					},1);
				});
			} catch(e) {
				console.error('Error on page '+location.hash+': '+e.message);
				return;
			}
		} else {
			if (location.hash !== '') {
				this.on404();
			}
		}
	},
	/**
	 * Function to scroll to anchor if anchor is supplied.
	 */
	scrollToAnchor: function(anchor) {
		if (anchor) {
			var $anchor = $('a[name='+anchor+']');
			if ($anchor.length > 0) {
				var scrollTo = $anchor.offset().top;
				window.scrollTo(0, scrollTo);
			}
		}
	},
	/**
	 * Get which page to show based on location.hash
	 * @param forceHash string - if you want to render a page without changing the actual location.hash
	 */
	setPageToHash: function(forceHash) {
		var hash;
		if (!location.hash) {
			hash = this.PAGE_INDEX;
		} else {
			hash = location.hash.replace(this.HASHBANG,'');
		}
		if (forceHash) {
			hash = forceHash.replace(this.HASHBANG,'');
		}
		var originalHash = hash;
		var funcApplyArguments = [];
		var pageObj = this.refPage;
		var hasParams = false;

		//Remove anchor and save for later
		var splitAnchor = hash.split('#');
		var anchor = splitAnchor[1] ? splitAnchor[1] : false;

		hash = splitAnchor[0].toString();

		//Remove params if any
		hash.replace('/?','/'+this.PAGE_INDEX+'?');
		if (hash.indexOf('?') !=-1) {
			hash = hash.split('?')[0];
			hasParams = true;
		}

		//First check if hash has folders
		if (hash.indexOf('/') !== -1) {
			//Add index if ends with /
			if(hash.charAt(hash.length-1) == '/') {
				hash+=this.PAGE_INDEX;
			}
			var hashFolder = hash.split('/');
			for (i=0;i<hashFolder.length;i++) {
				//console.log(hashFolder[i]);
				pageObj = pageObj[hashFolder[i]];
			}
			hash = hashFolder[hashFolder.length-1];
		} else {
			pageObj = this.refPage[hash];
		}

		//If has hash params
		if (hasParams) {
			var hashParams = {};
			var hashParts = originalHash.split('?');
			hash = hashParts[0];

			var paramVals = hashParts[1];
			paramVals = paramVals.split('&');
			var funcParamNames = this.getParamNames(pageObj);

			//Format the supplied parameters into an object.
			for (var paramVal in paramVals) {
				var paramValSplit = paramVals[paramVal].split('=');
				var param = paramValSplit[0];
				var value = paramValSplit[1];
				//console.log('p:'+param+' v:'+value);
				hashParams[param] = value;
			}
			//Protect from xss attacks by encoding some special chars.
			for (var val in hashParams) {
				if (hashParams[val] !== undefined) {
					var safeVal = this.replaceAllMultiple(hashParams[val],{
						'&':'&amp;',
						'<':'&lt;',
						'>':'&gt;',
						'"':'&quot;',
						"'":'&#x27;',
						'/':'&#x2F'
					});
					hashParams[val] = safeVal;
				}

			}

			//Step through funcParamNames to see if we should add a value.
			for (var paramName in funcParamNames) {
				var parName = funcParamNames[paramName];
				if(hashParams[parName]) {
					funcApplyArguments.push(hashParams[parName]);
				} else {
					funcApplyArguments.push(undefined);
				}
			}
		}
		//console.log('setPageHtml', pageObj);
		Omit.setPageHtml(pageObj, funcApplyArguments, anchor);

	},
	/**
	 * Function to get param names
	 * @param func
	 * @return {Boolean}
	 */
	getParamNames: function(func) {
		var funStr = func.toString();
		return funStr.slice(funStr.indexOf('(')+1, funStr.indexOf(')')).match(/([^\s,]+)/g);
	},
	/**
	 * String replace but with replace /g
	 * @param string
	 * @param replaceObject
	 * @return {*}
	 */
	replaceAllMultiple: function(string, replaceObject) {
		var output = string;
		for(var replace in replaceObject) {
			output = output.replace(new RegExp(replace,'g'),replaceObject[replace]);
		}
		return output;
	},
	/**
	* Converts Shorthand CSS into regular CSS.
	*/
	convertShorthandCSS: function(css) {
		var beginnings = ['{',';','; '];
		var replaceProperties = this.shorthandCSSProperties;
		var replaceAttributes = this.shorthandCSSAttributes;
		var replaceCSS3Properties = this.shorthandCSS3Properties;
		var replaceCSS3Prefixes = this.shorthandCSS3Prefixes;

		//Replace shorthand properties
		for (var replace in replaceProperties) {
			for (var beginning in beginnings) {
				//console.console.log(replaceWith,replaceObj[replaceWith]);
				css = css.replace(new RegExp(beginnings[beginning] + replace + ':','g'), beginnings[beginning] + replaceProperties[replace] + ':');
			}
		}

		//Replace shorthand attributes
		for (var replace in replaceAttributes) {
			css = css.replace(new RegExp(':' + replace + ';','g'), ':' + replaceAttributes[replace] + ';');
		}

		//Special case for . converted to px.
		css = css.replace(/\.(?=;| |\))/g,'px');

		//Replace "" with url("");
		css = css.replace(new RegExp('"[.a-zA-Z0-9_\/]{1,100}\.[a-zA-Z0-9]{1,3}"','g'), 'url($&)');

		//Convert shorthand prefixed CSS3
		for (var replace in replaceCSS3Properties) {
			//console.log(replace);
			var replaceArray = replaceCSS3Properties[replace];
			for (var beginning in beginnings) {
				css = css.replace(new RegExp(
					beginnings[beginning] + replace + ':\[a-zA-Z0-9,._\)\( \-]{1,500};','g'
				),function(match){
					//Get the attribute entered
					var attribute = match.split(':')[1].replace(';','');
					var output = '';
					//Step through and add all prefixes, start on 1, cause 0 is the actual property.
					var replaceBeginning = beginnings[beginning]
					for (var i=1; i < replaceArray.length;i++) {
						var currentPrefix = replaceArray[i];
						if (currentPrefix !== '') {
							currentPrefix = '-'+replaceCSS3Prefixes[replaceArray[i]]+'-';
						}
						if (i>1) {
							replaceBeginning = '';
						}
						output+=replaceBeginning + currentPrefix + replaceArray[0]+':'+attribute+';';
					}
					return output;
				});
			}
		}
		return css;
	},
	/**
	 * Constants used by CSS3 animation compilator.
	 */
	FROM: 'from',
	TO: 'to',
	KEYFRAMES: 'keyframes',
	ANIMATION_END: 'oanimationend animationend webkitAnimationEnd',
	/**
	 * Compiles the global object animation into CSS for CSS3 animations.
	 */
	compileCSS3Animations: function() {
		var outputCSS = '';
		if (window.animation) {
			for (var name in animation) {
				var css = '';
				var prefixedCSS = '';
				var callCSS = '';

				var callProperty = {
					duration: '0s',
					timing: 'ease',
					delay: '0s',
					direction: 'normal',
					unknown: 'none',
					iteration: 'infinite',
					name: name
				};

				css += '@keyframes '+name+'{';

				var animationMode = animation[name];

				for (var mode in animationMode) {
					//If from or to
					if (mode == this.FROM || mode == this.TO) {
						css+= mode + '{'+animationMode[mode]+'}';
					//If is int
					} else if(mode % 1 === 0) {
						css+= mode +'%'+ '{'+animationMode[mode]+'}';
					//If is 10,20,30,...
					} else if(mode.indexOf(',') !== -1) {
						var modes = mode.split(',');
						for (var m in modes) {
							modes[m] = modes[m]+'%';
						}
						modes = modes.join();
						css += modes + '{'+animationMode[mode]+'}';
					//If it is one of the other properties, such as direction
					} else {
						for (var property in callProperty) {
							var thisProperty = callProperty[property];
							if (animationMode[property]) {
								if (property == 'duration' || property == 'delay') {
									callProperty[property] = (parseInt(animationMode[property])/1000)+'s';
								} else {
									callProperty[property] = animationMode[property];
								}
							}

						}
					}
				}

				css+='}';

				//Add all keyframe prefixes
				var cssPrefixes = this.shorthandCSS3Prefixes;
				prefixedCSS += css;
				for (var prefix in cssPrefixes) {
					//No IE specific for keyframes.
					if (prefix !== 'i') {
						prefixedCSS += css.replace(this.KEYFRAMES,'-'+cssPrefixes[prefix]+'-'+this.KEYFRAMES);
					}
				}

				//Build the call-css out of callproperty object
				for (var property in callProperty) {
					callCSS += callProperty[property]+' ';
				}

				animation[name] = callCSS;
				animation[name].apa = 'test';
				outputCSS += prefixedCSS;
			}

			animation.end = this.ANIMATION_END;
		}
		this.cssAnimations = outputCSS;
	}
};

/**
 * Make changes to jQuery functions:
 * -monkeyPatching all manipulation functions to run js when page has been manipulated
 * -add event animationEnd
 * -enable animations from var animation in .animate
 * -create offOn function that unbinds and then binds a supplied event
 */
(function($){
	//jQuery monkeyPatch manager
	$.monkeyPatch = function(map) {
		var original = {};
		var processor = function(key) {
			$.fn[key] = function() {
				original[key].apply(this, arguments);
				map[key].apply(this, arguments);
			}
		};
		for (var key in map) {
			original[key] = $.fn[key];
			processor(key);
		}
	};

	var monkeyPatchFuncArray = [
		'append',
		'appendTo',
		'before',
		'html',
		'insertAfter',
		'insertBefore',
		'prepend',
		'prependTo',
		'replaceAll',
		'replaceWith',
		'wrap',
		'wrapAll',
		'wrapInner'
	];
	var monkeyPatchFunctions = {};

	for (var func in monkeyPatchFuncArray) {
		monkeyPatchFunctions[ monkeyPatchFuncArray[func] ] = function() {
			Omit.setJs();
		}
	}

	$.monkeyPatch(monkeyPatchFunctions);

	//Rewrite event binding to first unbind before binding. This is due to the fact that all js-functions are run at each page load. If hashchange is unbound, then we shall bind it again with Omit.listenForHashChange();
	$.fn.offOn = function() {
		this.off(arguments[0]);
		this.on.apply(this, arguments);
		if (arguments[0].indexOf('hashchange') !== -1) {
			Omit.listenForHashChange();
		}
		return this;
	}
	//Add function for css3 animation end
	$.fn.animationEnd = function(func) {
		this.on(window.Omit.ANIMATION_END, func);
	}

	//Enable our css animations with the animate jquery function
	var oldAnimate = $.fn.animate;
	$.fn.animate = function() {
		var firstArgument = arguments[0];
		if (typeof firstArgument === 'string') {
			//console.log(firstArgument);
			this.css('animation',firstArgument);
			if (arguments[1]) {
				var self = this;
				var callback = arguments[1];
				var animationCSS = arguments[0];
				this.animationEnd(function(){
					callback();
					var styleWithAnimation = self.attr('style');
					//console.log('animation: '+animationCSS.slice(0, -1)+';');
					//Remove animation css from style attribute. Added christmas eve 2012-12-24.
					self.css({
						'animation':'',
						'-o-animation':'',
						'-moz-animation':'',
						'-webkit-animation':''
					});
				}
				);
			}
			return this;
		} else {
			return oldAnimate.apply(this,arguments);
		}
	}

})(jQuery);