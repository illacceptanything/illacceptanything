/**
 * RightJS UI Internationalization: Romanian module
 *
 * Copyright (C) 2012 Andrei Tofan
 */
RightJS.Object.each({

  Calendar: {
    Done:            'Gata',
    Now:             'Acum',
    NextMonth:       'Luna Urmãtoare',
    PrevMonth:       'Luna Anterioarã',
    NextYear:        'Anul Urmãtor',
    PrevYear:        'Anul Anterior',

    dayNames:        'Luni Marþi Miercuri Joi Vineri Sâmbãtã Duminicã'.split(' '),
    dayNamesShort:   'Lu Ma Mi Jo Vi Sa Du'.split(' '),
    dayNamesMin:     'L M M J V S D'.split(' '),
    monthNames:      'Ianuarie Februarie Martie Aprilie Mai Iunie Iulie August Septembrie Octombrie Noiembrie Decembrie'.split(' '),
    monthNamesShort: 'Ian Feb Mar Apr Mai Iun Iul Aug Sep Oct Nov Dec'.split(' ')
  },

  Lightbox: {
    Close: 'Închide',
    Prev:  'Imaginea Anterioarã',
    Next:  'Imaginea Urmãtoare'
  },

  InEdit: {
    Save:   "Salveazã",
    Cancel: "Anuleazã"
  },

  Colorpicker: {
    Done: 'Gata'
  },

  Dialog: {
    Ok:       'Bine',
    Close:    'Închide',
    Cancel:   'Anuleazã',
    Help:     'Ajutor',
    Expand:   'Afiseazã',
    Collapse: 'Ascunde',

    Alert:    'Atenþie!',
    Confirm:  'Confirmã',
    Prompt:   'Intrebare'
  },

  Rte: {
    Clear:       'Goleºte',
    Save:        'Salveazã',
    Source:      'Sursã',
    Bold:        'Îngroºat',
    Italic:      'Înclinat',
    Underline:   'Subliniat',
    Strike:      'Tãiat',
    Ttext:       'Evidenþiazã',
    Header:      'Titlu',
    Cut:         'Decupeazã',
    Copy:        'Copiazã',
    Paste:       'Lipeste',
    Pastetext:   'Lipeste Text',
    Left:        'Stânga',
    Center:      'Centru',
    Right:       'Dreapta',
    Justify:     'In mod egal',
    Undo:        'Pas Inapoi',
    Redo:        'Pas Inainte',
    Code:        'Cod',
    Quote:       'Citat',
    Link:        'Adaugã Legaturã',
    Image:       'Adaugã Imagine',
    Video:       'Adaugã Videoclip',
    Dotlist:     'Lista neordonatã',
    Numlist:     'Lista ordonatã',
    Indent:      'Adaugã margine',
    Outdent:     'Scade marginea',
    Forecolor:   'Culoare text',
    Backcolor:   'Culoare fundal',
    Select:      'Selecteazã',
    Remove:      'ªterge',
    Format:      'Formateazã',
    Fontname:    'Nume Font',
    Fontsize:    'Mãrime Font',
    Subscript:   'Indice',
    Superscript: 'Exponent',
    UrlAddress:  'Adresa URL'
  }

}, function(module, i18n) {
  if (self[module]) {
    RightJS.$ext(self[module].i18n, i18n);
  }
});