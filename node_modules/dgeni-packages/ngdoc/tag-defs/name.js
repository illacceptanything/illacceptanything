module.exports = function(createDocMessage) {
  return {
    name: 'name',
    required: true,
    transforms: function(doc, tag, value) {
      var INPUT_TYPE = /input\[(.+)\]/;
      if ( doc.docType === 'input' ) {
        var match = INPUT_TYPE.exec(value);
        if ( !match ) {
          throw new Error(createDocMessage('Invalid input directive name.  It should be of the form: "input[inputType]" but was "' + value + '"', doc));
        }
        doc.inputType = match[1];
      }
      return value;
    }
  };
};