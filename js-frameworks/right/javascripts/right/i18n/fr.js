/**
 * RightJS UI Internationalization: French module
 *
 * Copyright (C) Nikolay Nemshilov
 */
RightJS.Object.each({

  Calendar: {
    Done:            "Fait",
    Now:             "Maint.",
    NextMonth:       "Mois prochain",
    PrevMonth:       "Mois précédent",
    NextYear:        "L'année prochain",
    PrevYear:        "L'année précédente",

    dayNames:        'Dimanche Lundi Mardi Mercredi Jeudi Vendredi Samedi'.split(' '),
    dayNamesShort:   'Dim Lun Mar Mer Jeu Ven Sam'.split(' '),
    dayNamesMin:     'Di Lu Ma Me Je Ve Sa'.split(' '),
    monthNames:      'Janvier Février Mars Avril Mai Juin Juillet Août Septembre Octobre Novembre Décembre'.split(' '),
    monthNamesShort: 'Jan Fév Mar Avr Mai Juin Juil Août Sept Oct Nov Déc'.split(' ')
  },

  Lightbox: {
    Close: 'Fermer',
    Prev:  'Image précédente',
    Next:  'Image suivante'
  },

  InEdit: {
    Save:   "Enregistrer",
    Cancel: "Annuler"
  },

  Colorpicker: {
    Done: 'Fait'
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