/**
 * RightJS UI Internationalization: Finnish module
 *
 * Copyright (C) Juho Vepsäläinen
 * Source: http://www.csc.fi/sivut/kotoistus/suositukset/vahv_kalenterit_cldr1_4.htm
 */
RightJS.Object.each({

  Calendar: {
    Done:            "OK",
    Now:             "Tänään",
    NextMonth:       "Seuraava kuukausi",
    PrevMonth:       "Edellinen kuukausi",
    NextYear:        "Seuraava vuosi",
    PrevYear:        "Edellinen vuosi",

    dayNames:        'Sunnuntai Maanantai Tiistai Keskiviikko Torstai Perjantai Lauantai'.split(' '),
    dayNamesShort:   'Su Ma Ti Ke To Pe La'.split(' '),
    dayNamesMin:     'S M T K T P L'.split(' '),
    monthNames:      'Tammikuu Helmikuu Maaliskuu Huhtikuu Toukokuu Kesäkuu Heinäkuu Elokuu Syyskuu Lokakuu Marraskuu Joulukuu'.split(' '),
    monthNamesShort: 'Tammi Helmi Maalis Huhti Touko Kesä Heinä Elo Syys Loka Marras Joulu'.split(' ')
  },

  Lightbox: {
    Close: 'Sulje',
    Prev:  'Edellinen kuva',
    Next:  'Seuraava kuva'
  },

  InEdit: {
    Save:   "Tallenna",
    Cancel: "Peruuta"
  },

  Colorpicker: {
    Done: 'OK'
  },

  Dialog: {
    Ok:       'Ok',
    Close:    'Sulje',
    Cancel:   'Peruuta',
    Help:     'Ohje',
    Expand:   'Laajenna',
    Collapse: 'Luhista',

    Alert:    'Varoitus!',
    Confirm:  'Vahvista',
    Prompt:   'Syötä'
  },

  Rte: {
    Clear:       'Tyhjennä',
    Save:        'Tallenna',
    Source:      'Lähdekoodi',
    Bold:        'Lihavointi',
    Italic:      'Kursivointi',
    Underline:   'Alleviivaus',
    Strike:      'Yliviivaus',
    Ttext:       'Tasamittainen',
    Header:      'Otsikko',
    Cut:         'Leikkaa',
    Copy:        'Kopioi',
    Paste:       'Liitä',
    Pastetext:   'Paste as text',
    Left:        'Vasemmalle',
    Center:      'Keskelle',
    Right:       'Oikealle',
    Justify:     'Tasaa',
    Undo:        'Kumoa',
    Redo:        'Toista',
    Code:        'Koodilohko',
    Quote:       'Lainaus',
    Link:        'Lisää linkki',
    Image:       'Lisää kuva',
    Video:       'Lisää video',
    Dotlist:     'Tavallinen lista',
    Numlist:     'Numeroitu lista',
    Indent:      'Sisennä',
    Outdent:     'Ulonna',
    Forecolor:   'Tekstin väri',
    Backcolor:   'Taustan väri',
    Select:      'Valitse',
    Remove:      'Poista',
    Format:      'Muotoilu',
    Fontname:    'Fontin nimi',
    Fontsize:    'Koko',
    Subscript:   'Alaindeksi',
    Superscript: 'Yläindeksi',
    UrlAddress:  'www-osoite'
  }

}, function(module, i18n) {
  if (self[module]) {
    RightJS.$ext(self[module].i18n, i18n);
  }
});
