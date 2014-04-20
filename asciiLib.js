mergeInto(LibraryManager.library, {
	js_ascii_changeConsoleText: function(textPtr,len) {
	  var text=intArrayToString(HEAPU8.subarray(textPtr, textPtr+len));
	  setTimeout (function () {document.getElementById('console').innerHTML=text;},20);
	},
	js_ascii_changeConsoleColors: function(backColor,foreColor) {
	  var console=document.getElementById('console');
	  console.style.backgroundColor='#'+Pointer_stringify(backColor);
	  console.style.color='#'+Pointer_stringify(foreColor);
	},
	js_ascii_setTimeout: function (ms,id) {
	  fireTimeout = Module.cwrap('_onjs_fireTimeout', 'number', ['number']);
	  setTimeout (function () {fireTimeout(id);},ms);
	},
	js_ascii_onMouseMoveEvent: function (functionName,el,ev) {
	  if (functionName!=0&&(typeof lockMouseEvent === "undefined" || lockMouseEvent==null)) {
	    var fontWidthTest=document.getElementById('fontWidthTest');
	    var fontWidth=fontWidthTest.clientWidth; //put this in some precalculated data
	    var fontHeight=fontWidthTest.clientHeight;

	    var rect = el.getBoundingClientRect();
	    var X = ev.clientX - rect.left - el.clientLeft + el.scrollLeft;
	    var Y = ev.clientY - rect.top - el.clientTop + el.scrollTop;
	    X=Math.floor(X/fontWidth);
	    Y=Math.floor(Y/fontHeight);
	    
	    if (X>=0&&Y>=0&&(typeof lastMouseX === "undefined" || lastMouseX!=X || lastMouseY!=Y)) {
	      Module.ccall(functionName,'number',['number','number'],[X,Y]);
	      lockMouseEvent=1;
	      setTimeout (function () {lockMouseEvent=null;},10);
	      lastMouseX=X;
	      lastMouseY=Y;
	    }
	  }
	},
	js_ascii_onMouseKeyEvent: function (functionName,el,ev,isDown) {
	  if (functionName!=0) {
	    var state;
		if (ev.button==0)
		  state = 1<<0;
		else if (ev.button==2)
		  state = 1<<1;
		  console.log ("mouse key "+ev.button+" = "+isDown);
		Module.ccall(functionName,'number',['number','number'],[state,isDown]);
	  }
	},
	js_ascii_setConsoleSize: function (cols,lines) {
	  var console=document.getElementById('console');
	  var fontWidthTest=document.getElementById('fontWidthTest');
	  var fontWidth=fontWidthTest.clientWidth;
	  var fontHeight=fontWidthTest.clientHeight;
	  console.style.width=(cols*fontWidth).toString()+'px';
	  console.style.height=(lines*fontHeight).toString()+'px';
	  console.style.maxWidth=console.style.width;
	},
	js_ascii_toggleEvents : function (toggle) {
	  var console = document.getElementById('console');
	  var mouseDown=function (ev) {_js_ascii_onMouseKeyEvent('_onjs_fireMouseKey',this,ev,1);return false};
	  var mouseUp=function (ev) {_js_ascii_onMouseKeyEvent('_onjs_fireMouseKey',this,ev,0);return false;};
	  var mouseMove=function (ev) {_js_ascii_onMouseMoveEvent('_onjs_fireMouseMove',this,ev);return false;};
	  if (toggle>0) {
	    console.addEventListener('mousedown',mouseDown,true); //Opera needs a special reminder to not select text
	    console.addEventListener('mouseup',mouseUp,true);
	    console.addEventListener('mousemove',mouseMove,true);
	  }
	  else {
	    console.removeEventListener('mousedown',mouseDown,true);
	    console.removeEventListener('mouseup',mouseUp,true);
	    console.removeEventListener('mousemove',mouseMove,true);
	  }
	},
});
