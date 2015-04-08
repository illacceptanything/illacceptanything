/**
 * RightJS UI Internationalization: Chinese module
 *
 * Copyright (C) Ym Xiao
 */
RightJS.Object.each({

  Calendar: {
    Done:           '完成',
    Now:            '现在',
    NextMonth:      '下一月',
    PrevMonth:      '上一页',
    NextYear:       '下一年',
    PrevYear:       '上一年',

    dayNames:        '星期日 星期一 星期二 星期三 星期四 星期五 星期六'.split(' '),
    dayNamesShort:   '日 一 二 三 四 五 六'.split(' '),
    dayNamesMin:     '日 一 二 三 四 五 六'.split(' '),
    monthNames:      '1月 2月 3月 4月 5月 6月 7月 8月 9月 10月 11月 12月'.split(' '),
    monthNamesShort: '1 2 3 4 5 6 7 8 9 10 11 12'.split(' ')
  },

  Lightbox: {
    Close: '关闭',
    Prev:  '前一张',
    Next:  '后一张'
  },

  InEdit: {
    Save:   "保存",
    Cancel: "取消"
  },

  Colorpicker: {
    Done: '完成'
  },

  Dialog: {
    Ok:       '确定',
    Close:    '关闭',
    Cancel:   '取消',
    Help:     '帮助',
    Expand:   '展开',
    Collapse: '关闭',
    Alert:    '警告!',
    Confirm:  '确认',
    Prompt:   '请输入'
  },

  Rte: {
    Clear:       '清除',
    Save:        '保存',
    Source:      '源代码',
    Bold:        '粗体',
    Italic:      '斜体',
    Underline:   '下滑线',
    Strike:      '删除线',
    Ttext:       '输入文本',
    Header:      '图',
    Cut:         '剪切',
    Copy:        '复制',
    Paste:       '粘贴',
    Pastetext:   '粘贴为文本',
    Left:        '左对齐',
    Center:      '居中对齐',
    Right:       '右对齐',
    Justify:     '自适应',
    Undo:        '撤销',
    Redo:        '重做',
    Code:        '代码',
    Quote:       '引用',
    Link:        '链接',
    Image:       '图片',
    Video:       '视频',
    Dotlist:     '无序列表',
    Numlist:     '有序列表',
    Indent:      '缩进',
    Outdent:     '减少缩进',
    Forecolor:   '字体颜色',
    Backcolor:   '背景颜色',
    Select:      '选项',
    Remove:      '删除',
    Format:      '格式化',
    Fontname:    '字体',
    Fontsize:    '字号',
    Subscript:   '下标',
    Superscript: '上标',
    UrlAddress:  'URL地址'
  }

}, function(module, i18n) {
  if (self[module]) {
    RightJS.$ext(self[module].i18n, i18n);
  }
});
