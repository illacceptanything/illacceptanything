/**
 * RightJS UI Internationalization: Russian module
 *
 * Copyright (C) Nikolay Nemshilov
 */
RightJS.Object.each({

  Calendar: {
    Done:            'Готово',
    Now:             'Сейчас',
    NextMonth:       'Следующий месяц',
    PrevMonth:       'Предыдущий месяц',
    NextYear:        'Следующий год',
    PrevYear:        'Предыдущий год',

    dayNames:        'Воскресенье Понедельник Вторник Среда Четверг Пятница Суббота'.split(' '),
    dayNamesShort:   'Вск Пнд Втр Срд Чтв Птн Сбт'.split(' '),
    dayNamesMin:     'Вс Пн Вт Ср Чт Пт Сб'.split(' '),
    monthNames:      'Январь Февраль Март Апрель Май Июнь Июль Август Сентябрь Октябрь Ноябрь Декабрь'.split(' '),
    monthNamesShort: 'Янв Фев Мар Апр Май Инь Иль Авг Сен Окт Ноя Дек'.split(' ')
  },

  Lightbox: {
    Close: 'Закрыть',
    Prev:  'Предыдущее изображение',
    Next:  'Следующее изображение'
  },

  InEdit: {
    Save:   "Сохранить",
    Cancel: "Отмена"
  },

  Colorpicker: {
    Done: 'Готово'
  },

  Dialog: {
    Ok:       'Готово',
    Close:    'Закрыть',
    Cancel:   'Отмена',
    Help:     'Помощь',
    Expand:   'Во все окно',
    Collapse: 'Обычный размер',

    Alert:    'Внимание!',
    Confirm:  'Подтверждение',
    Prompt:   'Ввод данных'
  },

  Rte: {
    Clear:       'Очистить',
    Save:        'Сохранить',
    Source:      'Исходный код',
    Bold:        'Жирный',
    Italic:      'Наклонный',
    Underline:   'Подчеркнутый',
    Strike:      'Зачеркнутый',
    Ttext:       'Моноширинный',
    Header:      'Заголовок',
    Cut:         'Вырезать',
    Copy:        'Копировать',
    Paste:       'Вставить',
    Pastetext:   'Вставить как текст',
    Left:        'Ровнять по левому краю',
    Center:      'Ровнять по центру',
    Right:       'Ровнять по правому краю',
    Justify:     'Ровнять по ширине',
    Undo:        'Отменить',
    Redo:        'Повторить',
    Code:        'Блок кода',
    Quote:       'Цитата',
    Link:        'Добавить ссылку',
    Image:       'Вставить картинку',
    Video:       'Вставить видео',
    Dotlist:     'Обычный список',
    Numlist:     'Нумерованый список',
    Indent:      'Добавить уровень',
    Outdent:     'Убрать уровень',
    Forecolor:   'Цвет текста',
    Backcolor:   'Цвет подсветки',
    Select:      'Выделить',
    Remove:      'Удалить',
    Format:      'Формат',
    Fontname:    'Шрифт',
    Fontsize:    'Размер',
    Subscript:   'Нижний индекс',
    Superscript: 'Верхний индекс',
    UrlAddress:  'URL адрес'
  }

}, function(module, i18n) {
  if (self[module]) {
    RightJS.$ext(self[module].i18n, i18n);
  }
});