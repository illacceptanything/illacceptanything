/**
 * RightJS UI Internationalization: German module
 *
 * Copyright (C) Nikolay Nemshilov
 */
RightJS.Object.each({

  Calendar: {
    Done:           'Erledigt',
    Now:            'Jetzt',
    NextMonth:      'Nächster Monat',
    PrevMonth:      'Vorhergehender Monat',
    NextYear:       'Nächstes Jahr',
    PrevYear:       'Vorhergehendes Jahr',

    dayNames:        'Sonntag Montag Dienstag Mittwoch Donnerstag Freitag Samstag'.split(' '),
    dayNamesShort:   'So Mo Di Mi Do Fr Sa'.split(' '),
    dayNamesMin:     'So Mo Di Mi Do Fr Sa'.split(' '),
    monthNames:      'Januar Februar März April Mai Juni Juli August September Oktober November Dezember'.split(' '),
    monthNamesShort: 'Jan Feb Mär Apr Mai Jun Jul Aug Sep Okt Nov Dez'.split(' ')
  },

  Lightbox: {
    Close: 'Schließen',
    Prev:  'Vorhergehendes Bild',
    Next:  'Nächstes Bild'
  },

  InEdit: {
    Save:   "Speichern",
    Cancel: "Abbruch"
  },

  Colorpicker: {
    Done: 'Erledigt'
  },

  Dialog: {
    Ok:       'Ok',
    Close:    'Schließen',
    Cancel:   'Abbrechen',
    Help:     'Hilfe',
    Expand:   'Aufklappen',
    Collapse: 'Zuklappen',

    Alert:    'Warnung!',
    Confirm:  'Bestätigen',
    Prompt:   'Eingabe'
  },

  Rte: {
    Clear:       'Clear',
    Save:        'Specihern',
    Source:      'Quelle',
    Bold:        'Fett',
    Italic:      'Kursiv',
    Underline:   'Unterstrichen',
    Strike:      'Durchgestrichen',
    Ttext:       'Typetext',
    Header:      'Kopfzeile',
    Cut:         'Ausschneiden',
    Copy:        'Kopieren',
    Paste:       'Einfügen',
    Pastetext:   'Paste as text',
    Left:        'Links',
    Center:      'Zentriert',
    Right:       'Rechts',
    Justify:     'Blocksatz',
    Undo:        'Rückgängig',
    Redo:        'Wiederholen',
    Code:        'Code block',
    Quote:       'Block quote',
    Link:        'Link einfügen',
    Image:       'Bild einfügen',
    Video:       'Video einfügen',
    Dotlist:     'Aufzählung',
    Numlist:     'Nummerierung',
    Indent:      'Einrücken',
    Outdent:     'Ausrücken',
    Forecolor:   'Textfarbe',
    Backcolor:   'Hintergrundfarbe',
    Select:      'Auswählen',
    Remove:      'Entfernen',
    Format:      'Formatieren',
    Fontname:    'Schriftname',
    Fontsize:    'Schriftgröße',
    Subscript:   'Tiefgestellt',
    Superscript: 'Hochgestellt',
    UrlAddress:  'URL Adresse'
  }

}, function(module, i18n) {
  if (self[module]) {
    RightJS.$ext(self[module].i18n, i18n);
  }
});