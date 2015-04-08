/**
 * RightJS UI Internationalization: Ukrainian module
 *
 * Copyright (C) Maxim Golubev
 * Copyright (C) Nikolay Nemshilov
 */
if (self.Calendar) {
  Calendar.Options.firstDay = 0;
}

RightJS.Object.each({

  Calendar: {
    Done:            'Гаразд',
    Now:             'Зараз',
    NextMonth:       'Наступный мiсяць',
    PrevMonth:       'Попереднiй мiсяць',
    NextYear:        'Наступний рiк',
    PrevYear:        'Попереднiй рiк',

    dayNames:        'Неділя Понеділок Вівторок Середа Четвер П\'ятниця Субота'.split(' '),
    dayNamesShort:   'Ндл Пнд Втр Срд Чтв Птн Сбт'.split(' '),
    dayNamesMin:     'Нд Пн Вт Ср Чт Пт Сб'.split(' '),
    monthNames:      'Січень Лютий Березень Квітень Травень Червень Липень Серпень Вересень Жовтень Листопад Грудень'.split(' '),
    monthNamesShort: 'Січ Лют Бер Квіт Трав Черв Лип Серп Вер Жовт Лист Груд'.split(' ')
  },

  Lightbox: {
    Close: 'Сховати',
    Prev:  'Попереднє зображення',
    Next:  'Наступне зображення'
  },

  InEdit: {
    Save:   "Зберегти",
    Cancel: "Скасувати"
  },

  Colorpicker: {
    Done: 'Гаразд'
  },

  Dialog: {
    Ok:       'Гаразд',
    Close:    'Сховати',
    Cancel:   'Сховати',
    Help:     'Помощь',
    Expand:   'Во все окно',
    Collapse: 'Обычный размер',
    Alert:    'Внимание!',
    Confirm:  'Подтверждение',
    Prompt:   'Ввод данных'
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
    Pastetext:   'Вставить как текст',
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