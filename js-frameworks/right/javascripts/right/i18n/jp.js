/**
 * RightJS UI Internationalization: Japanese module
 *
 * Copyright (C) Nikolay Nemshilov
 */
if (self.Calendar) {
  Calendar.Options.format = 'POSIX';
}

RightJS.Object.each({

  Calendar: {
    Done:           'Done',
    Now:            '今日',
    NextMonth:      '翌月',
    PrevMonth:      '前の月',
    NextYear:       '翌年',
    PrevYear:       '前年',

    dayNames:        '日曜日 月曜日 火曜日 水曜日 木曜日 金曜日 土曜日'.split(' '),
    dayNamesShort:   '日 月 火 水 木 金 土'.split(' '),
    dayNamesMin:     '日 月 火 水 木 金 土'.split(' '),
    monthNames:      '1月 2月 3月 4月 5月 6月 7月 8月 9月 10月 11月 12月'.split(' '),
    monthNamesShort: '1月 2月 3月 4月 5月 6月 7月 8月 9月 10月 11月 12月'.split(' ')
  },

  Lightbox: {
    Close: '閉じる',
    Prev:  '前の画像',
    Next:  '次の画像'
  },

  InEdit: {
    Save:   "保存",
    Cancel: "キャンセル"
  },

  Colorpicker: {
    Done: 'Done'
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