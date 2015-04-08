/**
* RightJS UI Internationalization: Lithuanian module
*
* Copyright (C) Nikolay Nemshilov
*/
RightJS.Object.each({

  Calendar: {
    Done: 'Atlikta',
    Now: 'Dabar',
    NextMonth: 'Sekantis mėnuo',
    PrevMonth: 'Ankstesnis mėnuo',
    NextYear: 'Sekantys metai',
    PrevYear: 'Ankstesni metai',

    dayNames: 'Sekmadienis Pirmadienis Antradienis Trečiadienis Ketvirtadienis Penktadienis Šeštadienis'.split(' '),
    dayNamesShort: 'Sek Pir Ant Tre Ket Pen Šeš'.split(' '),
    dayNamesMin: 'Sk Pr An Tr Kt Pt Št'.split(' '),
    monthNames: 'Sausis Vasaris Kovas Balandis Gegužė Birželis Liepa Rugpjūtis Rugsėjis Spalis Lapkritis Gruodis'.split(' '),
    monthNamesShort: 'Sau Vas Kov Bal Geg Bir Lie Rgp Rgs Spa Lap Grd'.split(' ')
  },

  Lightbox: {
    Close: 'Uždaryti',
    Prev: 'Ankstesnis paveikslėlis',
    Next: 'Sekantis paveikslėlis'
  },

  InEdit: {
    Save: "Išsaugoti",
    Cancel: "Atšaukti"
  },

  Colorpicker: {
    Done: 'Atlikta'
  },

  Dialog: {
    Ok: 'Atlikta',
    Close: 'Uždaryti',
    Cancel: 'Atšaukti',
    Help: 'Pagalba',
    Expand: 'Per visą langą',
    Collapse: 'Suskleisti',

    Alert: 'Dėmesio!',
    Confirm: 'Patvirtinimas',
    Prompt: 'Duomenų įvedimas'
  },

  Rte: {
    Clear: 'Išvalyti',
    Save: 'Išsaugoti',
    Source: 'Pirminis tekstas',
    Bold: 'Pusjuodis',
    Italic: 'Kursyvas',
    Underline: 'Pabrauktas',
    Strike: 'Užbrauktas',
    Ttext: 'Lygiaplotis',
    Header: 'Antraštė',
    Cut: 'Iškirpti',
    Copy: 'Kopijuoti',
    Paste: 'Įdėti',
    Pastetext: 'Įterpti kaip tekstą',
    Left: 'Lygiuoti į kairę',
    Center: 'Centruoti',
    Right: 'Lygiuoti į dešinę',
    Justify: 'Per visą plotį',
    Undo: 'Atšaukti',
    Redo: 'Grąžinti',
    Code: 'Kodas',
    Quote: 'Citata',
    Link: 'Įterpti nuorodą',
    Image: 'Įterpti paveikslėlį',
    Video: 'Įterpti video',
    Dotlist: 'Paprastas sąrašas',
    Numlist: 'Skaitinis sąrašas',
    Indent: 'Pridėti įtrauką',
    Outdent: 'Pašalinti įtrauką',
    Forecolor: 'Teksto spalva',
    Backcolor: 'Fono spalva',
    Select: 'Pažymėti',
    Remove: 'Pašalinti',
    Format: 'Formatas',
    Fontname: 'Šriftas',
    Fontsize: 'Dydis',
    Subscript: 'Apatinis indeksas',
    Superscript: 'Viršutinis indeksas',
    UrlAddress: 'URL adresas'
  }

}, function(module, i18n) {
  if (self[module]) {
    RightJS.$ext(self[module].i18n, i18n);
  }
});