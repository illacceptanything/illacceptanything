module.exports =
utilities =

   runInNewContext: do ->
      server = (source, context) ->
         `//@browserify-ignore`
         require('vm').createScript(source).runInNewContext(context)
      
      client = (source, context) ->
         semaphore =
            source: source
         
         $frame = window.document.createElement 'iframe'
         $frame.style.display = 'none'
         body(html(window.document)).insertBefore $frame
         $window = $frame.contentWindow
        #$window.document.close()
         
         $window.__fromSemaphore = semaphore
         $script = $window.document.createElement 'script'
         $script.text = "window.__fromSemaphore.result = eval(window.__fromSemaphore.source)"
         body(html($window.document)).insertBefore $script
         
         body(html(window.document)).removeChild $frame
         
         return semaphore.result
      
      nodeFor = (type) -> (node) ->
         node.getElementsByTagName(type)[0] or
         node.insertBefore (node.ownerDocument or node).createElement type
      
      html = nodeFor('html')
      head = nodeFor('head')
      body = nodeFor('body')

      return (source, context) ->
         (if process?.browser then client else server).apply this, arguments
