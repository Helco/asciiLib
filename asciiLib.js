mergeInto(LibraryManager.library, {
	js_ascii_setConsoleSize: function (cols,lines) {
	  var console=document.getElementById('console');
	  var fontWidthTest=document.getElementById('fontWidthTest');
	  var fontWidth=fontWidthTest.clientWidth;
	  var fontHeight=fontWidthTest.clientHeight;
	  console.style.width=(cols*fontWidth).toString()+'px';
	  console.style.height=(lines*fontHeight).toString()+'px';
	},
	js_ascii_changeConsoleText: function(textPtr) {
	  var text=Pointer_stringify(textPtr);
	  setTimeout (function () {document.getElementById('console').innerHTML=text;},20);
	},
	js_ascii_changeConsoleColors: function(backColor,foreColor) {
	  var hui = document.styleSheets[0].rules || document.styleSheets[0].cssRules;
	  var styleBySelector = {};
	  for (var i=0; i<hui.length; i++)
	    styleBySelector[hui[i].selectorText] = hui[i].style;
	  var st=styleBySelector[".consoleClass"];
	  var console=document.getElementById('console');
	  console.style.background-color='#'+Pointer_stringify(backColor);
	  console.style.color='#'+Pointer_stringify(foreColor);
alert('Used');
	}
});
