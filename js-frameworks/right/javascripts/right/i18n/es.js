/**
 * RightJS UI Internationalization: Spanish module
 *
 * Copyright (C) Nikolay Nemshilov
 */
RightJS.Object.each({

  Calendar: {
    Done:           'Hecho',
    Now:            'Ahora',
    NextMonth:      'Mes siguiente',
    PrevMonth:      'Mes precedente',
    NextYear:       'Año siguiente',
    PrevYear:       'Año precedente',

    dayNames:        'Domingo Lunes Martes Miércoles Jueves Viernes Sábado'.split(' '),
    dayNamesShort:   'Dom Lun Mar Mié Jue Vie Sab'.split(' '),
    dayNamesMin:     'Do Lu Ma Mi Ju Vi Sa'.split(' '),
    monthNames:      'Enero Febrero Marzo Abril Mayo Junio Julio Agosto Septiembre Octubre Noviembre Diciembre'.split(' '),
    monthNamesShort: 'Ene Feb Mar Abr May Jun Jul Ago Sep Oct Nov Dic'.split(' ')
  },

  Lightbox: {
    Close: 'Cerrar',
    Prev:  'Imagen precedente',
    Next:  'Imagen siguiente'
  },

  InEdit: {
    Save:   "Guardar",
    Cancel: "Borrar"
  },

  Colorpicker: {
    Done: 'Hecho'
  },

  Dialog: {
    Ok:       'Ok',
    Close:    'Cerrar',
    Cancel:   'Cancelar',
    Help:     'Ayuda',
    Expand:   'Expandir',
    Collapse: 'Plegar',

    Alert:    'Aviso!',
    Confirm:  'Confirmar',
    Prompt:   'Entrar'
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