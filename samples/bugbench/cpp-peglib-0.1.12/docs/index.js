// Setup editros
const grammar = ace.edit("grammar-editor");
grammar.setShowPrintMargin(false);
grammar.setValue(localStorage.getItem('grammarText') || '');
grammar.moveCursorTo(0, 0);

const code = ace.edit("code-editor");
code.setShowPrintMargin(false);
code.setValue(localStorage.getItem('codeText') || '');
code.moveCursorTo(0, 0);

const codeAst = ace.edit("code-ast");
codeAst.setShowPrintMargin(false);
codeAst.setOptions({
  readOnly: true,
  highlightActiveLine: false,
  highlightGutterLine: false
})
codeAst.renderer.$cursorLayer.element.style.opacity=0;

const codeAstOptimized = ace.edit("code-ast-optimized");
codeAstOptimized.setShowPrintMargin(false);
codeAstOptimized.setOptions({
  readOnly: true,
  highlightActiveLine: false,
  highlightGutterLine: false
})
codeAstOptimized.renderer.$cursorLayer.element.style.opacity=0;

$('#opt_mode').val(localStorage.getItem('optimazationMode') || 'all');
$('#opt_rules').val(localStorage.getItem('optimazationRules'));

function generateErrorListHTML(errors) {
  let html = '<ul>';

  html += $.map(errors, function (x) {
    return '<li data-ln="' + x.ln + '" data-col="' + x.col + '"><span>' + x.ln + ':' + x.col + '</span> <span>' + x.msg + '</span></li>';
  }).join('');

  html += '<ul>';

  return html;
}

function parse() {
  const $grammarValidation = $('#grammar-validation');
  const $grammarInfo = $('#grammar-info');
  const grammarText = grammar.getValue();

  const $codeValidation = $('#code-validation');
  const $codeInfo = $('#code-info');
  const codeText = code.getValue();

  const optimazationMode = $('#opt_mode').val();
  const optimazationRules = $('#opt_rules').val();

  localStorage.setItem('grammarText', grammarText);
  localStorage.setItem('codeText', codeText);
  localStorage.setItem('optimazationMode', optimazationMode);
  localStorage.setItem('optimazationRules', optimazationRules);

  $grammarInfo.html('');
  $grammarValidation.hide();
  $codeInfo.html('');
  $codeValidation.hide();
  codeAst.setValue('');
  codeAstOptimized.setValue('');

  if (grammarText.length === 0) {
   return;
  }

  const mode = optimazationMode == 'all';
  const data = JSON.parse(Module.lint(grammarText, codeText, mode, optimazationRules));

  const isValid = data.grammar.length === 0;
  if (isValid) {
    $grammarValidation.removeClass('editor-validation-invalid').text('Valid').show();

    const isValid = data.code.length === 0;
    if (isValid) {
      codeAst.insert(data.ast);
      codeAstOptimized.insert(data.astOptimized);
      $codeValidation.removeClass('editor-validation-invalid').text('Valid').show();
    } else {
      const html = generateErrorListHTML(data.code);
      $codeInfo.html(html);
      $codeValidation.addClass('editor-validation-invalid').text('Invalid').show();
    }
  } else {
    const html = generateErrorListHTML(data.grammar);
    $grammarInfo.html(html);
    $grammarValidation.addClass('editor-validation-invalid').text('Invalid').show();
  }
}

// Event handing for text editiing
let timer;
function setupTimer() {
  clearTimeout(timer);
  timer = setTimeout(parse, 750);
};
grammar.getSession().on('change', setupTimer);
code.getSession().on('change', setupTimer);

// Event handing in the info area
function makeOnClickInInfo(editor) {
  return function () {
    const el = $(this);
    editor.navigateTo(el.data('ln') - 1, el.data('col') - 1);
    editor.focus();
  }
};
$('#grammar-info').on('click', 'li', makeOnClickInInfo(grammar));
$('#code-info').on('click', 'li', makeOnClickInInfo(code));

// Event handing in the AST optimazation
$('#opt_mode').on('change', setupTimer);
$('#opt_rules').on('keyup', setupTimer);

// Show page
$('#main').css({
  'display': 'flex',
});

// WebAssembly
var Module = {
  onRuntimeInitialized: function() {
    // Initial parse
    parse();
  }
};
