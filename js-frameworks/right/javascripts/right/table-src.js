/**
 * Tables Specific Wrappers v2.2.0
 * http://rightjs.org/plugins/table
 *
 * Copyright (C) 2009-2011 Nikolay Nemshilov
 */
var Table = RightJS.Table = (function(RightJS) {
/**
 * Table plugin initialization script
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
var R = RightJS,
    $ = RightJS.$,
    $E = RightJS.$E,
    isHash  = RightJS.isHash,
    isElement = RightJS.isElement,
    Class   = RightJS.Class,
    Object  = RightJS.Object,
    Element = RightJS.Element;




/**
 * Tables specific dom-wrapper
 *
 * Copyright (C) 2010-2011 Nikolay Nemshilov
 */
var Table = Element.Wrappers.TABLE = new Class(Element, {

  extend: {
    version: '2.2.0',

    Options: {
      ascMarker:  '&#x25BC;',  // asc marker content
      descMarker: '&#x25B2;',  // desc marker content
      algorithm:  'text',      // default sorting algorithm 'text' or 'numeric'
      order:      'asc',        // default order
      sortedClass: 'sorted'
    }
  },

  /**
   * Basic constructor
   *
   * @param mixed raw dom-element or an options list
   * @return void
   */
  initialize: function(arg) {
    var options = arg || {}, element = options;

    if (isHash(options) && !isElement(options)) {
      element = 'table';
    }

    this.$super(element, options);

    this.options = Object.merge(
      Table.Options, new Function('return '+ this.get('data-table'))
    );
  },

  /**
   * Sorts the table by the given index
   *
   * @param mixed Number column index or TH element
   * @param String order direction 'asc' or 'desc'
   * @param String optional algorythm 'string', 'numeric'
   * @return Table this
   */
  sort: function(index, direction, algorithm) {
    var th = index instanceof Element ? index : this.header().last().children('th')[index];
    if (!th) { return this; } // in case something goes wrong

    // reading data from the TH cell
    index     = th.parent().children('th').indexOf(th);
    direction = direction || (this.marker && this.marker.parent() === th ? this.marker.asc ? 'desc' : 'asc' : null);
    algorithm = algorithm || (th.hasClass('numeric') ? 'numeric' : null);

    // handling the fallback from the options
    direction = direction || this.options.order;
    algorithm = algorithm || this.options.algorithm;
    sortedClass =  this.options.sortedClass;

    // collecting the list of sortable rows
    var rows  = this.rows().map(function(row) {
      var cell = row.children('td')[index], text = cell ? cell.text() : '';

      if (algorithm === 'numeric') {
        text = R(text).toFloat();
      }

      return { row: row, text: text };
    });

    // creating an anchor where to insert the rows
    var anchor = rows[0] ?
      $E('tr').insertTo(rows[0].row, 'before') :
      $E('tr').insertTo(this.first('tbody') || this);

    // finding the exact sorting algorithm
    if (typeof(algorithm) !== 'function') {
      algorithm = direction === 'asc' ? function(a, b) {
        return a.text > b.text ? 1 : a.text < b.text ? -1 : 0;
      } : function(a, b) {
        return a.text > b.text ? -1 : a.text < b.text ? 1 : 0;
      };
    }

    // sorting the rows and reinsert them in the correct order
    rows.sort(algorithm).reverse().each(function(hash) {
      anchor.insert(hash.row, 'after');
    });

    anchor.remove();

    // putting the sorting marker
    this.marker = (
      this.marker || th.first('span.sort-marker') || $E('span', {'class': 'sort-marker'})
    ).update(
      this.options[direction === 'asc' ? 'ascMarker' : 'descMarker']
    ).insertTo(th, 'bottom');
    this.marker.asc = direction === 'asc';
    this.find('th').each(function(th) {
      th.removeClass(sortedClass);
    });
    this.marker.parent().toggleClass(sortedClass);

    return this;
  },

  /**
   * Returns the table data-rows
   *
   * @return Array table data rows
   */
  rows: function() {
    return this.find('tr').reject(function(row) {
      return row.first('th') || row.parent('tfoot');
    });
  },

  /**
   * Returns the table header rows
   *
   * @return Array table header rows
   */
  header: function() {
    return this.find('tr').filter('first', 'th');
  },

  /**
   * Returns the table footer rows
   *
   * @return Array table footer rows
   */
  footer: function() {
    return this.find('tfoot > tr');
  }
});

/**
 * Document level hooks for the table plugin
 *
 * Copyright (C) 2010 Nikolay Nemshilov
 */
$(document).onClick(function(event) {
  var th = event.find('th.sortable');

  if (th) {
    th.parent('table').sort(th);
  }
});
return Table;
})(RightJS);