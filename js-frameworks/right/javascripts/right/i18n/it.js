/**
 * RightJS UI Internationalization: Italian module
 *
 * Copyright (C) Nikolay Nemshilov
 */
RightJS.Object.each({

  Calendar: {
    Done:           'Fatto',
    Now:            'Oggi',
    NextMonth:      'Mese successivo',
    PrevMonth:      'Mese precedente',
    NextYear:       'Anno seguente',
    PrevYear:       'Anno precedente',

    dayNames:        'Domenica Lunedi Martedi Mercoledi Giovedi Venerdi Sabato'.split(' '),
    dayNamesShort:   'Dom Lun Mar Mer Gio Ven Sab'.split(' '),
    dayNamesMin:     'Do Lu Ma Me Gi Ve Sa'.split(' '),
    monthNames:      'Gennaio Febbraio Marzo Aprile Maggio Giugno Luglio Agosto Settembre Ottobre Novembre Dicembre'.split(' '),
    monthNamesShort: 'Gen Feb Mar Apr Mag Giu Lug Ago Set Ott Nov Dic'.split(' ')
  },

  Lightbox: {
    Close: 'Chiudi',
    Prev:  'Immagine precedente',
    Next:  'Immagine seguente'
  },

  InEdit: {
    Save:   "Salva",
    Cancel: "Abbandona"
  },

  Colorpicker: {
    Done: 'Fatto'
  },

  Dialog: {
    Ok:       'Ok',
    Close:    'Close',
    Cancel:   'Cancel',
    Help:     'Help',
    Expand:   'Expand',
    Collapse: 'Collapse',

    Alert:    'Warning!',
    Confirm:  'Confirm',
    Prompt:   'Enter'
  },

  Rte: {
    Clear:       'Clear',
    Save:        'Save',
    Source:      'Source',
    Bold:        'Bold',
    Italic:      'Italic',
    Underline:   'Underline',
    Strike:      'Strike through',
    Ttext:       'Typetext',
    Header:      'Header',
    Cut:         'Cut',
    Copy:        'Copy',
    Paste:       'Paste',
    Pastetext:   'Paste as text',
    Left:        'Left',
    Center:      'Center',
    Right:       'Right',
    Justify:     'Justify',
    Undo:        'Undo',
    Redo:        'Redo',
    Code:        'Code block',
    Quote:       'Block quote',
    Link:        'Add link',
    Image:       'Insert image',
    Video:       'Insert video',
    Dotlist:     'List with dots',
    Numlist:     'List with numbers',
    Indent:      'Indent',
    Outdent:     'Outdent',
    Forecolor:   'Text color',
    Backcolor:   'Background color',
    Select:      'Select',
    Remove:      'Remove',
    Format:      'Format',
    Fontname:    'Font name',
    Fontsize:    'Size',
    Subscript:   'Subscript',
    Superscript: 'Superscript',
    UrlAddress:  'URL Address'
  }

}, function(module, i18n) {
  if (self[module]) {
    RightJS.$ext(self[module].i18n, i18n);
  }
});