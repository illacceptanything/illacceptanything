/**
 * RightJS UI Internationalization: Hungarian module
 *
 * Copyright (C) Arnold Mészáros
 */
if (self.Calendar) {
  Calendar.Options.format   = 'HU';
  Calendar.Formats.HU       = '%Y.%m.%d';
}

RightJS.Object.each({

  Calendar: {
    Done:            'Kész',
    Now:             'Most',
    NextMonth:       'Következő hónap',
    PrevMonth:       'Előző hónap',
    NextYear:        'Következő év',
    PrevYear:        'Előző év',

    dayNames:        'Vasárnap Hétfő Kedd Szerda Csütörtök Péntek Szombat'.split(' '),
    dayNamesShort:   'Va Hé Ke Sze Csü Pé Szo'.split(' '),
    dayNamesMin:     'V H K Sz Cs P Sz'.split(' '),
    monthNames:      'Január Február Március Április Május Június Július Augusztus Szeptember Október November December'.split(' '),
    monthNamesShort: 'Jan Feb Már Ápr Máj Jún Júl Aug Szep Okt Nov Dec'.split(' ')
  },

  Lightbox: {
    Close: 'Bezár',
    Prev:  'Előző kép',
    Next:  'Következő kép'
  },

  InEdit: {
    Save:   "Save",
    Cancel: "Cancel"
  },

  Colorpicker: {
    Done: 'Kész'
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