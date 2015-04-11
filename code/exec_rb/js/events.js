(function () {
  var historyList = [], historyIndex = 0;
  var lines = [], printed = false, webruby, load_string_func;
  var partial_src = "";

  if (localStorage) {
    historyList = JSON.parse(localStorage.historyList || '[]');
    historyIndex = historyList.length;
  }

  var INPUT_HEIGHT = 21;
  var ENTER_KEY = 13;
  var UP_KEY    = 38;
  var DOWN_KEY  = 40;

  // Taken from http://stackoverflow.com/a/901144
  function getParameterByName(name) {
    name = name.replace(/[\[]/, "\\[").replace(/[\]]/, "\\]");
    var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
    results = regex.exec(location.search);
    return results == null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
  }

  function getQueryLevel() {
    var level = parseInt(getParameterByName('level')) || 2;
    level = Math.min(2, level);
    level = Math.max(0, level);
    return level;
  }

  window.Module = {};
  window.Module['print'] = function (x) {
    lines.push(x);
    printed = true;
  };

  $(document).ready(function() {
    $(window).resize(function() {
      var $window = $(window);

      $('#shell').height($window.height()-10);
      $('#editor').width($window.width()-60);
    });

    $('#shell').click(function() {
      focus_editor();
    });

    $(window).trigger('resize');

    $('#try-it-now').live('click', function(e) {
      e.preventDefault();
      $.modal.close();
    });

    var getUrlParameter = function(name) {
      return decodeURIComponent((new RegExp('[?|&]' + name + '=' + '([^&;]+?)(&|#|;|$)').exec(location.search)||[,""])[1].replace(/\+/g, '%20'))||null;
    };

    webruby = new WEBRUBY({print_level: getQueryLevel()});
    webruby.run();
    webruby.run_source($('script[type="text/ruby"]').text());

    var run_command = function(last_line) {
      lines = [];
      printed = false;

      if (last_line != historyList[historyList.length-1])
        historyList.push(last_line);

      historyIndex = historyList.length;

      var complete_src = partial_src ? (partial_src + "\n" + last_line) : (last_line);
      var ret = webruby.multiline_run_source(complete_src);
      if (ret === 2) {
        partial_src = complete_src;
      } else {
        partial_src = "";
        if (!printed) {
          window.Module['print']('nil');
        }
      }

      add_output(last_line, lines);
    };

    var add_output = function(source, lines) {
      var element = $("#output");
      var value   = lines.slice(-1)[0];

      var prompt = '<span class="prompt">' + $('#command .prompt').text() + '</span>';
      var session = element.append('<div class="session"><div class="command">' + prompt + '<div class="source editor"/></div><div class="response"></div></div>').find('.session:last');
      var response = session.find('.response');

      $(lines.slice(0, -1)).each(function(_, line) {
        response.append('<p>');
        response.find('p:last').text(line);
      });

      $('#command .prompt').text(partial_src ? "**" : ">>");
      var size = $('.session').size();
      var id = 'editor' + size;
      session.find('.command .source').attr('id', id).text(source);
      var viewer = configure_editor(id);
      viewer.setReadOnly(true);
      resize_textarea(viewer, '#' + id);
      viewer.resize();
      focus_editor();

      if (value) {
        response.append('<span class="value-prompt">=&gt;</span><span class="value" />');
        response.find('.value').text(value);
      }

      var source_height = $('#' + id).height();
      session.find('.command').height(source_height);

      if (value && value.match(/\S*Error: /))
        response.addClass('error');

      scroll_to_end();

      historyIndex = historyList.length;
    };

    var scroll_to_end = function() {
      $('#container').scrollTop($('#output').height() + $('#command').height());
    };

    var set_command = function(cmd) {
      editor.session.setValue(cmd);
      editor.getSelection().moveCursorFileEnd();
      resize_textarea(editor, '#editor');
    };

    var quick_command = function(cmd) {
      set_command(cmd);
      setTimeout(function() {
        run_command(cmd);
        set_command('');
      }, 200)
    }

    var resize_textarea = function(editor, selector) {
      var lines = editor.session.getLength();

      $(selector).height(lines * INPUT_HEIGHT);
      editor.resize();
      scroll_to_end();
    };

    $('#editor').keyup(function() {
      resize_textarea(editor, '#editor');
    });

    $('#editor').keydown(function(e) {
      var cmd, found = true;

      switch (e.which) {
        case UP_KEY:
          historyIndex--;
          cmd = historyList[historyIndex];

          if (historyIndex < 0)
            historyIndex = 0;
          else
            set_command(cmd);

          break;

        case DOWN_KEY:
          historyIndex++;
          cmd = historyList[historyIndex];

          if (historyIndex >= historyList.length) {
            historyIndex = historyList.length;
            set_command('');
          }
          else
            set_command(cmd);

          break;

        case ENTER_KEY:
          if (e.shiftKey) {
            editor.getSession().insert(editor.selection.getCursor(), "\n");
            resize_textarea(editor, '#editor');
            editor.resize();
          }
          else {
            var val = editor.getValue().trimRight();
            if (val)
              run_command(val);
            else {
              add_output('', []);
              scroll_to_end();
            }

            set_command('');
            $(this).height(INPUT_HEIGHT).focus();
          }
          break;

        default:
          found = false;
          break;
      }

      if (found) e.preventDefault();
    });

    window.onbeforeunload = function () {
      webruby.close();

      if (localStorage) {
        localStorage.historyList = JSON.stringify(historyList);
      }
    };

    var configure_editor = function(selector) {
      var editor = ace.edit(selector);
      editor.renderer.setShowGutter(false);
      editor.setTheme("ace/theme/monokai");
      editor.setHighlightActiveLine(false);
      editor.setShowPrintMargin(false);
      editor.getSession().setMode("ace/mode/ruby");
      editor.getSession().setUseSoftTabs(true);
      editor.getSession().setTabSize(2);
      return editor;
    };

    var editor = configure_editor('editor');
    window.ed = editor;
    editor.commands.addCommand({
      name: 'up',
      bindKey: 'Up',
      exec: function(editor) {
        return false;
      }
    });

    editor.commands.addCommand({
      name: 'down',
      bindKey: 'Down',
      exec: function(editor) {
        return false;
      }
    });

    var focus_editor = function() {
      editor.focus();
      $('textarea:last').focus();
    };

    focus_editor();

    if (startupCommand = getUrlParameter('cmd'))
      quick_command(startupCommand);

    if (gistID = getUrlParameter('gist')) {
      $.get("https://api.github.com/gists/" + gistID, function (data) {
        $.each(data.files, function(name, file) {
          if (file.language == "Ruby"){
            quick_command(file.content);
          }
        });
      });
    }

    if (!startupCommand && !gistID && localStorage.saw_welcome != 'yes') {
      set_command('puts "Hello World"');
      localStorage.saw_welcome = 'yes';
      $('#welcome').modal({onClose: function(dialog) {
        dialog.data.fadeOut('fast', function () {
          $.modal.close();
          focus_editor();
        });
      }});
    }

    var saveGist = function(description, content, callback) {
      var data = {"description": description, "public": true, "files": {"sample.rb": {"content": content}}};
      $.post('https://api.github.com/gists', JSON.stringify(data), function(response) {
        callback(response.id);
      });
    };
    //saveGist("example gist", "puts 1+1", function (gistID) { alert(gistID); });
  });
}());
